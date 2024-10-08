/*
 * Copyright (C) 2011-2024 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "stdinc.h"

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/filelist/DirectoryListingManager.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>

namespace dcpp {

using ranges::for_each;
using ranges::find_if;

#define DIRECTORY_DOWNLOAD_REMOVAL_SECONDS 120


atomic<DirectoryDownloadId> directoryDownloadIdCounter { 0 };

DirectoryDownload::DirectoryDownload(const FilelistAddData& aListData, const string& aBundleName, const string& aTarget, Priority p, ErrorMethod aErrorMethod) :
	id(directoryDownloadIdCounter++), priority(p), target(aTarget), bundleName(aBundleName), created(GET_TIME()), listData(aListData), errorMethod(aErrorMethod) { }

bool DirectoryDownload::HasOwner::operator()(const DirectoryDownloadPtr& ddi) const noexcept {
	return owner == ddi->getOwner() && Util::stricmp(a, ddi->getBundleName()) != 0;
}

DirectoryListingManager::DirectoryListingManager() noexcept {
	QueueManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
}

DirectoryListingManager::~DirectoryListingManager() noexcept {
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
}


DirectoryDownloadPtr DirectoryListingManager::getPendingDirectoryDownloadUnsafe(const UserPtr& aUser, const string& aPath) const noexcept {
	auto ddlIter = ranges::find_if(dlDirectories, [&](const DirectoryDownloadPtr& ddi) {
		return ddi->getUser() == aUser &&
			ddi->getState() == DirectoryDownload::State::PENDING &&
			Util::stricmp(aPath.c_str(), ddi->getListPath().c_str()) == 0;
	});

	if (ddlIter == dlDirectories.end()) {
		dcassert(0);
		return nullptr;
	}

	return *ddlIter;
}

bool DirectoryListingManager::cancelDirectoryDownload(DirectoryDownloadId aId) noexcept {
	auto download = getDirectoryDownload(aId);
	if (!download) {
		return false;
	}

	if (download->getQueueItem()) {
		// Directory download removal will be handled through QueueManagerListener::ItemRemoved
		QueueManager::getInstance()->removeQI(download->getQueueItem());
	} else {
		// Completed download? Remove instantly
		removeDirectoryDownload(download);
	}

	return true;
}


void DirectoryListingManager::removeDirectoryDownload(const DirectoryDownloadPtr& aDownloadInfo) noexcept {
	{
		WLock l(cs);
		dlDirectories.erase(remove_if(dlDirectories.begin(), dlDirectories.end(), [=](const DirectoryDownloadPtr& aDownload) {
			return aDownload->getId() == aDownloadInfo->getId();
		}), dlDirectories.end());
	}

	fire(DirectoryListingManagerListener::DirectoryDownloadRemoved(), aDownloadInfo);
}

void DirectoryListingManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
	DirectoryDownloadList toRemove;

	{
		RLock l(cs);
		ranges::copy_if(dlDirectories, back_inserter(toRemove), [=](const DirectoryDownloadPtr& aDownload) {
			return aDownload->getProcessedTick() > 0 && aDownload->getProcessedTick() + DIRECTORY_DOWNLOAD_REMOVAL_SECONDS * 1000 < aTick;
		});
	}

	for (const auto& ddl : toRemove) {
		removeDirectoryDownload(ddl);
	}
}

bool DirectoryListingManager::hasDirectoryDownload(const string& aBundleName, void* aOwner) const noexcept {
	RLock l(cs);
	return ranges::find_if(dlDirectories, DirectoryDownload::HasOwner(aOwner, aBundleName)) != dlDirectories.end();
}

DirectoryDownloadList DirectoryListingManager::getDirectoryDownloads() const noexcept {
	DirectoryDownloadList ret;

	{
		RLock l(cs);
		ranges::copy(dlDirectories, back_inserter(ret));
	}

	return ret;
}


DirectoryDownloadList DirectoryListingManager::getPendingDirectoryDownloadsUnsafe(const UserPtr& aUser) const noexcept {
	DirectoryDownloadList ret;
	ranges::copy_if(dlDirectories, back_inserter(ret), [&](const DirectoryDownloadPtr& aDownload) {
		return aDownload->getUser() == aUser && aDownload->getState() == DirectoryDownload::State::PENDING;
	});

	return ret;
}

DirectoryDownloadPtr DirectoryListingManager::getDirectoryDownload(DirectoryDownloadId aId) const noexcept {
	RLock l(cs);
	auto i = ranges::find_if(dlDirectories, [&](const DirectoryDownloadPtr& aDownload) {
		return aDownload->getId() == aId;
	});

	return i == dlDirectories.end() ? nullptr : *i;
}

DirectoryDownloadPtr DirectoryListingManager::addDirectoryDownloadHookedThrow(const FilelistAddData& aListData, const string& aBundleName, const string& aTarget, Priority p, DirectoryDownload::ErrorMethod aErrorMethod) {
	dcassert(!aTarget.empty() && !aListData.listPath.empty() && !aBundleName.empty());
	auto downloadInfo = make_shared<DirectoryDownload>(aListData, PathUtil::cleanPathSeparators(aBundleName), aTarget, p, aErrorMethod);
	
	DirectoryListingPtr dl;
	{
		RLock l(cs);
		auto vl = viewedLists.find(aListData.user);
		if (vl != viewedLists.end()) {
			dl = vl->second;
		}
	}

	bool needList = false;
	{
		WLock l(cs);

		// Download already pending for this item?
		auto downloads = getPendingDirectoryDownloadsUnsafe(aListData.user);
		for (const auto& download: downloads) {
			if (Util::stricmp(aListData.listPath.c_str(), download->getListPath().c_str()) == 0) {
				return download;
			}
		}

		// Unique directory, fine...
		dlDirectories.push_back(downloadInfo);
		needList = aListData.user.user->isSet(User::NMDC) ? downloads.empty() : true;
	}

	fire(DirectoryListingManagerListener::DirectoryDownloadAdded(), downloadInfo);

	if (!dl && needList) {
		queueListHookedThrow(downloadInfo);
	} else if (dl) {
		dl->addAsyncTask([=, this] { handleDownloadHooked(downloadInfo, dl, false); });
	}

	return downloadInfo;
}

void DirectoryListingManager::queueListHookedThrow(const DirectoryDownloadPtr& aDownloadInfo) {
	auto user = aDownloadInfo->getUser();

	Flags flags = QueueItem::FLAG_DIRECTORY_DOWNLOAD;
	if (!user.user->isSet(User::NMDC)) {
		flags.setFlag(QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_RECURSIVE_LIST);
	}

	try {
		auto qi = QueueManager::getInstance()->addListHooked(aDownloadInfo->getListData(), flags.getFlags());
		aDownloadInfo->setQueueItem(qi);
	} catch (const DupeException&) {
		// We have a list queued already
	} catch (const Exception& e) {
		// Failed
		failDirectoryDownload(aDownloadInfo, e.getError());
		throw;
	}
}

DirectoryListingManager::DirectoryListingMap DirectoryListingManager::getLists() const noexcept {
	RLock l(cs);
	return viewedLists;
}

void DirectoryListingManager::processListHooked(const string& aFileName, const string& aXml, const HintedUser& aUser, const string& aRemotePath, int aFlags) noexcept {
	auto isPartialList = (aFlags & QueueItem::FLAG_PARTIAL_LIST) > 0;

	{
		RLock l(cs);
		auto p = viewedLists.find(aUser.user);
		if (p != viewedLists.end()) {
			if (p->second->getPartialList() && isPartialList) {
				//we don't want multiple threads to load those simultaneously. load in the list thread and return here after that
				p->second->addPartialListLoadTask(aXml, aRemotePath, true, [=, this] { processListActionHooked(p->second, aRemotePath, aFlags); });
				return;
			}
		}
	}

	auto hooks = aFlags & QueueItem::FLAG_MATCH_QUEUE ? nullptr : &loadHooks;
	auto dl = make_shared<DirectoryListing>(aUser, isPartialList, aFileName, false, hooks, false);
	try {
		if (isPartialList) {
			dl->loadPartialXml(aXml, aRemotePath);
		} else {
			dl->loadFile();
		}
	} catch (const Exception& e) {
		log(STRING_F(LIST_LOAD_FAILED, aFileName % e.getError()), LogMessage::SEV_ERROR);
		return;
	}

	processListActionHooked(dl, aRemotePath, aFlags);
}

void DirectoryListingManager::log(const string& aMsg, LogMessage::Severity aSeverity) noexcept {
	LogManager::getInstance()->message(aMsg, aSeverity, STRING(FILE_LISTS));
}

void DirectoryListingManager::maybeReportDownloadError(const DirectoryDownloadPtr& aDownloadInfo, const string& aError, LogMessage::Severity aSeverity) noexcept {
	if (aDownloadInfo->getErrorMethod() == DirectoryDownload::ErrorMethod::LOG && !aError.empty()) {
		auto nick = ClientManager::getInstance()->getFormattedNicks(aDownloadInfo->getUser());
		auto fullTarget = PathUtil::joinDirectory(aDownloadInfo->getTarget(), aDownloadInfo->getBundleName());
		log(STRING_F(ADD_BUNDLE_ERRORS_OCC, fullTarget % nick % aError), aSeverity);
	}
}

void DirectoryListingManager::handleDownloadHooked(const DirectoryDownloadPtr& aDownloadInfo, const DirectoryListingPtr& aList, bool aListDownloaded/* = true*/) noexcept {
	auto dir = aList->findDirectoryUnsafe(aDownloadInfo->getListPath());

	// Check the content
	{
		if (!dir || (aList->getPartialList() && dir->findIncomplete())) {
			if (!aListDownloaded) {
				// Downloading directory from an open list and it's missing/incomplete, queue with the wanted path
				try {
					queueListHookedThrow(aDownloadInfo);
				} catch (const Exception& e) {
					// Failure handled in queueListHookedThrow
					maybeReportDownloadError(aDownloadInfo, e.getError());
				}
			} else {
				// List downloaded but the directory is still missing/incomplete, abort to avoid an infinite downloading loop
				string error = !dir ? "Filelist directory not found" : "Invalid filelist directory content";
				failDirectoryDownload(aDownloadInfo, error);
				maybeReportDownloadError(aDownloadInfo, error);
			}

			return;
		}
	}

	// Queue the directory

	string errorMsg;
	auto queueInfo = aList->createBundleHooked(dir, aDownloadInfo->getTarget(), aDownloadInfo->getBundleName(), aDownloadInfo->getPriority(), errorMsg);
	if (queueInfo) {
		aDownloadInfo->setError(errorMsg);
		aDownloadInfo->setQueueInfo(queueInfo);
		aDownloadInfo->setQueueItem(nullptr);
		aDownloadInfo->setProcessedTick(GET_TICK());
		aDownloadInfo->setState(DirectoryDownload::State::QUEUED);
		fire(DirectoryListingManagerListener::DirectoryDownloadProcessed(), aDownloadInfo, *queueInfo, errorMsg);

		maybeReportDownloadError(aDownloadInfo, errorMsg, LogMessage::Severity::SEV_WARNING);
	} else {
		failDirectoryDownload(aDownloadInfo, errorMsg);
		maybeReportDownloadError(aDownloadInfo, errorMsg);
	}
}

void DirectoryListingManager::processListActionHooked(DirectoryListingPtr aList, const string& aPath, int aFlags) noexcept {
	if (aFlags & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
		DirectoryDownloadList downloadItems;

		{
			RLock l(cs);
			if (aFlags & QueueItem::FLAG_PARTIAL_LIST) {
				// Partial list
				auto ddl = getPendingDirectoryDownloadUnsafe(aList->getHintedUser(), aPath);
				if (ddl) {
					downloadItems.push_back(ddl);
				}
			} else {
				// Full filelist
				downloadItems = getPendingDirectoryDownloadsUnsafe(aList->getHintedUser());
			}
		}

		for (const auto& ddl: downloadItems) {
			handleDownloadHooked(ddl, aList);
		}
	}

	if(aFlags & QueueItem::FLAG_MATCH_QUEUE) {
		auto results = QueueManager::getInstance()->matchListing(*aList);
		if ((aFlags & QueueItem::FLAG_PARTIAL_LIST) && (!SETTING(REPORT_ADDED_SOURCES) || results.newFiles == 0 || results.bundles.empty())) {
			return;
		}

		log(aList->getNick(false) + ": " + results.format(), LogMessage::SEV_INFO);
	}
}

void DirectoryListingManager::on(QueueManagerListener::ItemFinished, const QueueItemPtr& qi, const string& aAdcDirectoryPath, const HintedUser& aUser, int64_t /*aSpeed*/) noexcept {
	if (!qi->isSet(QueueItem::FLAG_CLIENT_VIEW) || !qi->isSet(QueueItem::FLAG_USER_LIST))
		return;

	DirectoryListingPtr dl;
	{
		RLock l(cs);
		auto p = viewedLists.find(aUser.user);
		if (p == viewedLists.end()) {
			return;
		}

		dl = p->second;
	}

	if (dl) {
		dl->setFileName(qi->getListName());
		if (dl->hasCompletedDownloads()) {
			dl->addFullListTask(aAdcDirectoryPath);
		} else {
			fire(DirectoryListingManagerListener::OpenListing(), dl, aAdcDirectoryPath, Util::emptyString);
		}
	}
}

void DirectoryListingManager::on(QueueManagerListener::PartialListFinished, const HintedUser& aUser, const string& aXML, const string& aBase) noexcept {
	if (aXML.empty())
		return;

	DirectoryListingPtr dl;
	{
		RLock l(cs);
		auto p = viewedLists.find(aUser.user);
		if (p != viewedLists.end() && p->second->getPartialList()) {
			dl = p->second;
		} else {
			return;
		}
	}

	dl->addHubUrlChangeTask(aUser.hint);

	if (dl->hasCompletedDownloads()) {
		dl->addPartialListLoadTask(aXML, aBase);
	} else {
		fire(DirectoryListingManagerListener::OpenListing(), dl, aBase, aXML);
	}
}

void DirectoryListingManager::on(QueueManagerListener::ItemRemoved, const QueueItemPtr& qi, bool aFinished) noexcept {
	if (!qi->isSet(QueueItem::FLAG_USER_LIST))
		return;

	auto u = qi->getSources()[0].getUser();
	if (qi->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && !aFinished) {
		DirectoryDownloadPtr ddl;

		{
			RLock l(cs);
			ddl = getPendingDirectoryDownloadUnsafe(u, qi->getListDirectoryPath());
		}

		if (ddl) {
			auto error = QueueItem::Source::formatError(qi->getSources()[0]);
			if (!error.empty()) {
				failDirectoryDownload(ddl, error);
				maybeReportDownloadError(ddl, error);
			} else {
				removeDirectoryDownload(ddl);
			}
		}
	}

	if (qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		DirectoryListingPtr dl = nullptr;

		{
			RLock l(cs);
			auto p = viewedLists.find(u);
			if (p == viewedLists.end()) {
				dcassert(0);
				return;
			}

			dl = p->second;
		}

		dl->onListRemovedQueue(qi->getTarget(), qi->getListDirectoryPath(), aFinished);

		bool closing = (dl->getClosing() || !dl->hasCompletedDownloads());
		if (!aFinished && !dl->hasDownloads() && closing) {
			removeList(u);
		}
	}
}


void DirectoryListingManager::failDirectoryDownload(const DirectoryDownloadPtr& aDownloadInfo, const string& aError) noexcept {
	dcassert(!aError.empty());

	aDownloadInfo->setState(DirectoryDownload::State::FAILED);
	aDownloadInfo->setError(aError);
	aDownloadInfo->setProcessedTick(GET_TICK());
	aDownloadInfo->setQueueItem(nullptr);
	fire(DirectoryListingManagerListener::DirectoryDownloadFailed(), aDownloadInfo, aError);
}

DirectoryListingPtr DirectoryListingManager::openOwnList(ProfileToken aProfile, const string& aDir /* = Util::emptyString*/) noexcept {
	auto me = HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString);

	{
		auto dl = findList(me.user);
		if (dl) {
			// REMOVE
			dl->addShareProfileChangeTask(aProfile);
			return dl;
		}
	}

	auto dl = createList(me, true, Util::toString(aProfile), true);
	// dl->setMatchADL(useADL);

	fire(DirectoryListingManagerListener::OpenListing(), dl, aDir, Util::emptyString);
	return dl;
}

DirectoryListingPtr DirectoryListingManager::openLocalFileList(const HintedUser& aUser, const string& aFile, const string& aDir/* = Util::emptyString*/, bool aPartial/* = false*/) noexcept {
	{
		auto dl = findList(aUser.user);
		if (dl) {
			return nullptr;
		}
	}

	dcassert(aPartial || PathUtil::fileExists(aFile));

	auto dl = createList(aUser, aPartial, aFile, false);
	fire(DirectoryListingManagerListener::OpenListing(), dl, aDir, Util::emptyString);
	return dl;
}

DirectoryListingPtr DirectoryListingManager::createList(const HintedUser& aUser, bool aPartial, const string& aFileName, bool aIsOwnList) noexcept {
	auto dl = make_shared<DirectoryListing>(aUser, aPartial, aFileName, true, &loadHooks, aIsOwnList);

	{
		WLock l(cs);
		viewedLists[dl->getHintedUser()] = dl;
	}

	fire(DirectoryListingManagerListener::ListingCreated(), dl);
	return dl;
}

void DirectoryListingManager::on(QueueManagerListener::ItemAdded, const QueueItemPtr& aQI) noexcept {
	if (!aQI->isSet(QueueItem::FLAG_CLIENT_VIEW) || !aQI->isSet(QueueItem::FLAG_USER_LIST)) {
		return;
	}

	auto user = aQI->getSources()[0].getUser();
	auto dl = findList(user);
	if (dl) {
		dl->onAddedQueue(aQI->getTarget());
	}
}

DirectoryListingPtr DirectoryListingManager::findList(const UserPtr& aUser) noexcept {
	RLock l (cs);
	if (auto p = viewedLists.find(aUser); p != viewedLists.end()) {
		return p->second;
	}

	return nullptr;
}

HintedUser DirectoryListingManager::checkDownloadUrl(const HintedUser& aUser) const noexcept {
	auto userInfoList = ClientManager::getInstance()->getUserInfoList(aUser);
	if (!userInfoList.empty() && ranges::find(userInfoList, aUser.hint, &User::UserHubInfo::hubUrl) == userInfoList.end()) {
		sort(userInfoList.begin(), userInfoList.end(), User::UserHubInfo::ShareSort());

		return { aUser.user, userInfoList.back().hubUrl };
	}

	return aUser;
}

DirectoryListingPtr DirectoryListingManager::openRemoteFileListHookedThrow(const FilelistAddData& aListData, Flags::MaskType aFlags) {
	{
		auto dl = findList(aListData.user);
		if (dl) {
			return nullptr;
		}
	}

	auto user = checkDownloadUrl(aListData.user);
	auto qi = QueueManager::getInstance()->addListHooked(aListData, aFlags);
	if (!qi) {
		return nullptr;
	}

	DirectoryListingPtr dl;
	if (!qi->isSet(QueueItem::FLAG_PARTIAL_LIST)) {
		dl = createList(user, false, qi->getListName(), false);
	} else {
		dl = createList(user, true, Util::emptyString, false);
	}

	dl->onAddedQueue(qi->getTarget());
	return dl;
}

bool DirectoryListingManager::removeList(const UserPtr& aUser) noexcept {
	DirectoryListingPtr dl;

	{
		RLock l(cs);
		auto p = viewedLists.find(aUser);
		if (p == viewedLists.end()) {
			return false;
		}

		dl = p->second;
	}

	auto downloads = dl->getDownloads();
	if (!downloads.empty()) {
		dl->setClosing(true);

		// It will come back here after being removed from the queue
		for (const auto& p : downloads) {
			QueueManager::getInstance()->removeFile(p);
		}
	} else {
		{
			WLock l(cs);
			viewedLists.erase(aUser);
		}

		dl->close();
		fire(DirectoryListingManagerListener::ListingClosed(), dl);
	}

	return true;
}

} //dcpp