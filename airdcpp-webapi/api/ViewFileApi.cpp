/*
* Copyright (C) 2011-2016 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <web-server/stdinc.h>
#include <web-server/FileServer.h>
#include <web-server/JsonUtil.h>

#include <api/ViewFileApi.h>

#include <api/common/Serializer.h>
#include <api/common/Deserializer.h>

#include <airdcpp/File.h>
#include <airdcpp/QueueManager.h>
#include <airdcpp/ShareManager.h>
#include <airdcpp/ViewFileManager.h>

#include <boost/range/algorithm/copy.hpp>

namespace webserver {
	ViewFileApi::ViewFileApi(Session* aSession) : SubscribableApiModule(aSession, Access::VIEW_FILES_VIEW) {

		ViewFileManager::getInstance()->addListener(this);

		createSubscription("view_file_added");
		createSubscription("view_file_removed");
		createSubscription("view_file_updated");
		createSubscription("view_file_finished");

		METHOD_HANDLER(Access::VIEW_FILES_VIEW, METHOD_GET,		(EXACT_PARAM("sessions")),									ViewFileApi::handleGetFiles);
		METHOD_HANDLER(Access::VIEW_FILES_EDIT, METHOD_POST,	(EXACT_PARAM("sessions")),									ViewFileApi::handleAddFile);
		METHOD_HANDLER(Access::VIEW_FILES_VIEW, METHOD_GET,		(EXACT_PARAM("sessions"), TTH_PARAM),						ViewFileApi::handleGetFile);
		METHOD_HANDLER(Access::VIEW_FILES_EDIT, METHOD_POST,	(EXACT_PARAM("sessions"), TTH_PARAM),						ViewFileApi::handleAddLocalFile);
		METHOD_HANDLER(Access::VIEW_FILES_EDIT, METHOD_DELETE,	(EXACT_PARAM("sessions"), TTH_PARAM),						ViewFileApi::handleRemoveFile);

		METHOD_HANDLER(Access::VIEW_FILES_VIEW, METHOD_GET,		(EXACT_PARAM("sessions"), TTH_PARAM, EXACT_PARAM("text")),	ViewFileApi::handleGetText);
		METHOD_HANDLER(Access::VIEW_FILES_VIEW, METHOD_POST,	(EXACT_PARAM("sessions"), TTH_PARAM, EXACT_PARAM("read")),	ViewFileApi::handleSetRead);
	}

	ViewFileApi::~ViewFileApi() {
		ViewFileManager::getInstance()->removeListener(this);
	}

	json ViewFileApi::serializeDownloadState(const ViewFilePtr& aFile) noexcept {
		if (aFile->isLocalFile()) {
			return nullptr;
		}

		return Serializer::serializeDownloadState(*aFile.get());
	}

	json ViewFileApi::serializeFile(const ViewFilePtr& aFile) noexcept {
		auto mimeType = FileServer::getMimeType(aFile->getPath());
		return{
			{ "id", aFile->getTTH().toBase32() },
			{ "tth", aFile->getTTH().toBase32() },
			{ "text", aFile->isText() },
			{ "read", aFile->getRead() },
			{ "name", aFile->getDisplayName() },
			{ "download_state", serializeDownloadState(aFile) },
			{ "type", Serializer::serializeFileType(aFile->getPath()) },
			{ "time_opened", aFile->getTimeCreated() },
			{ "content_ready", aFile->isLocalFile() || aFile->isDownloaded() },
			{ "mime_type", mimeType ? mimeType : Util::emptyString },
		};
	}

	api_return ViewFileApi::handleGetFiles(ApiRequest& aRequest) {
		auto files = ViewFileManager::getInstance()->getFiles();
		aRequest.setResponseBody(Serializer::serializeList(files, serializeFile));
		return websocketpp::http::status_code::ok;
	}

	api_return ViewFileApi::handleAddFile(ApiRequest& aRequest) {
		const auto& j = aRequest.getRequestBody();
		auto tth = Deserializer::deserializeTTH(j);

		auto name = JsonUtil::getField<string>("name", j, false);
		auto size = JsonUtil::getField<int64_t>("size", j);
		auto user = Deserializer::deserializeHintedUser(j, true);
		auto isText = JsonUtil::getOptionalFieldDefault<bool>("text", j, false);

		ViewFilePtr file = nullptr;
		try {
			file = ViewFileManager::getInstance()->addUserFileThrow(name, size, tth, user, isText);
		} catch (const Exception& e) {
			aRequest.setResponseErrorStr(e.getError());
			return websocketpp::http::status_code::internal_server_error;
		}

		if (!file) {
			aRequest.setResponseErrorStr("File with the same TTH is open already");
			return websocketpp::http::status_code::bad_request;
		}

		aRequest.setResponseBody(serializeFile(file));
		return websocketpp::http::status_code::ok;
	}

	api_return ViewFileApi::handleAddLocalFile(ApiRequest& aRequest) {
		auto tth = aRequest.getTTHParam();
		if (!ShareManager::getInstance()->isFileShared(tth)) {
			aRequest.setResponseErrorStr("TTH not shared");
			return websocketpp::http::status_code::bad_request;
		}

		auto file = ViewFileManager::getInstance()->addLocalFile(tth, JsonUtil::getOptionalFieldDefault<bool>("text", aRequest.getRequestBody(), false));
		if (!file) {
			aRequest.setResponseErrorStr("File with the same TTH is open already");
			return websocketpp::http::status_code::bad_request;
		}

		aRequest.setResponseBody(serializeFile(file));
		return websocketpp::http::status_code::ok;
	}

	api_return ViewFileApi::handleGetText(ApiRequest& aRequest) {
		auto file = ViewFileManager::getInstance()->getFile(aRequest.getTTHParam());
		if (!file) {
			aRequest.setResponseErrorStr("File not found");
			return websocketpp::http::status_code::not_found;
		}

		if (!file->isText()) {
			aRequest.setResponseErrorStr("This method can't be used for non-text files");
			return websocketpp::http::status_code::bad_request;
		}

		if (File::getSize(file->getPath()) > Util::convertSize(1, Util::MB)) {
			aRequest.setResponseErrorStr("File is too big to be viewed as text");
			return websocketpp::http::status_code::bad_request;
		}

		string content;
		try {
			File f(file->getPath(), File::READ, File::OPEN);

			auto encoding = Util::emptyString;
			bool nfo = Util::getFileExt(file->getPath()) == ".nfo";
			if (nfo) {
				// Platform-independent encoding conversion function could be added if there is more use for it
#ifdef _WIN32
				encoding = "CP.437";
#else
				encoding = "cp437";
#endif
			}

			content = Text::toUtf8(f.read(), encoding);
		} catch (const FileException& e) {
			aRequest.setResponseErrorStr("Failed to open the file: " + e.getError() + "(" + file->getPath() + ")");
			return websocketpp::http::status_code::internal_server_error;
		}

		aRequest.setResponseBody({
			{ "text", content },
		});
		return websocketpp::http::status_code::ok;
	}

	api_return ViewFileApi::handleGetFile(ApiRequest& aRequest) {
		auto file = ViewFileManager::getInstance()->getFile(aRequest.getTTHParam());
		if (!file) {
			aRequest.setResponseErrorStr("File not found");
			return websocketpp::http::status_code::not_found;
		}

		aRequest.setResponseBody(serializeFile(file));
		return websocketpp::http::status_code::ok;
	}

	api_return ViewFileApi::handleRemoveFile(ApiRequest& aRequest) {
		auto success = ViewFileManager::getInstance()->removeFile(aRequest.getTTHParam());
		if (!success) {
			aRequest.setResponseErrorStr("File not found");
			return websocketpp::http::status_code::not_found;
		}

		return websocketpp::http::status_code::no_content;
	}

	api_return ViewFileApi::handleSetRead(ApiRequest& aRequest) {
		auto success = ViewFileManager::getInstance()->setRead(aRequest.getTTHParam());
		if (!success) {
			aRequest.setResponseErrorStr("File not found");
			return websocketpp::http::status_code::not_found;
		}

		return websocketpp::http::status_code::no_content;
	}

	void ViewFileApi::on(ViewFileManagerListener::FileAdded, const ViewFilePtr& aFile) noexcept {
		maybeSend("view_file_added", [&] { return serializeFile(aFile); });
	}

	void ViewFileApi::on(ViewFileManagerListener::FileClosed, const ViewFilePtr& aFile) noexcept {
		maybeSend("view_file_removed", [&] { return serializeFile(aFile); });
	}

	void ViewFileApi::on(ViewFileManagerListener::FileStateUpdated, const ViewFilePtr& aFile) noexcept {
		maybeSend("view_file_updated", [&] { 
			return json({
				{ "id", aFile->getTTH().toBase32() },
				{ "download_state", Serializer::serializeDownloadState(*aFile.get()) }
			});
		});
	}

	void ViewFileApi::on(ViewFileManagerListener::FileFinished, const ViewFilePtr& aFile) noexcept {
		onViewFileUpdated(aFile);

		maybeSend("view_file_finished", [&] { return serializeFile(aFile); });
	}

	void ViewFileApi::on(ViewFileManagerListener::FileRead, const ViewFilePtr& aFile) noexcept {
		onViewFileUpdated(aFile);
	}

	void ViewFileApi::onViewFileUpdated(const ViewFilePtr& aFile) noexcept {
		maybeSend("view_file_updated", [&] { return serializeFile(aFile); });
	}
}