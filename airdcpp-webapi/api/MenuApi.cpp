/*
* Copyright (C) 2011-2019 AirDC++ Project
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

#include "stdinc.h"

#include <api/MenuApi.h>

#include <web-server/JsonUtil.h>
#include <api/common/Deserializer.h>
#include <api/common/Serializer.h>

#include <api/QueueBundleUtils.h>

#include <airdcpp/Bundle.h>

#include <airdcpp/DirectoryListingManager.h>
#include <airdcpp/SearchManager.h>


/*#define CONTEXT_MENU_HANDLER(menuId, hook, hookHandler, clickHandler, listHandler, access) \
	createHook(menuId, [this](const string& aId, const string& aName) { \
		return ContextMenuManager::getInstance()->hook.addSubscriber(aId, aName, HOOK_HANDLER(hookHandler)); \
	}, [this](const string& aId) { \
		ContextMenuManager::getInstance()->hook.removeSubscriber(aId); \
	}); \
	METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("select")), clickHandler); \
	METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("list")), listHandler);
*/

#define INLINE_MODULE_METHOD_HANDLER(access, method, params, func) (this->getRequestHandlers().push_back(ApiModule::RequestHandler(access, method, BRACED_INIT_LIST params, func)))


#define CONTEXT_MENU_HANDLER(menuId, hook, hook2, idType, idDeserializerFunc, idSerializerFunc, access) \
	createHook(menuId, [this](const string& aId, const string& aName) { \
		return ContextMenuManager::getInstance()->hook##MenuHook.addSubscriber( \
			aId, \
			aName, \
			[this](const vector<idType>& aSelections, const ActionHookResultGetter<ContextMenuItemList>& aResultGetter) { \
				return MenuApi::menuListHookHandler<idType>(aSelections, aResultGetter, menuId, idSerializerFunc); \
			} \
		); \
	}, [this](const string& aId) { \
		ContextMenuManager::getInstance()->hook##MenuHook.removeSubscriber(aId); \
	}); \
	INLINE_MODULE_METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("select")), [=](ApiRequest& aRequest) { \
		return handleClickItem<idType>( \
			aRequest, \
			menuId, \
			std::bind(&ContextMenuManager::onClick##hook2##Item, ContextMenuManager::getInstance(), placeholders::_1, placeholders::_2, placeholders::_3), \
			idDeserializerFunc \
		); \
	}); \
	INLINE_MODULE_METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("list")), [=](ApiRequest& aRequest) { \
		return handleListItems<idType>( \
			aRequest, \
			std::bind(&ContextMenuManager::get##hook2##Menu, ContextMenuManager::getInstance(), placeholders::_1), \
			idDeserializerFunc \
		); \
	});

/*
#define ENTITY_CONTEXT_MENU_HANDLER(menuId, hook, hook2, idType, idDeserializerFunc, idSerializerFunc, entityType, entityDeserializerFunc, access) \
	createHook(menuId, [this](const string& aId, const string& aName) { \
		return ContextMenuManager::getInstance()->hook##MenuHook.addSubscriber( \
			aId, \
			aName, \
			[this](const vector<idType>& aSelections, const entityType& aEntityType, const ActionHookResultGetter<ContextMenuItemList>& aResultGetter) { \
				return MenuApi::menuListHookHandler(aSelections, aResultGetter, menuId, idSerializerFunc, aEntityType); \
			} \
		); \
	}, [this](const string& aId) { \
		ContextMenuManager::getInstance()->hook##MenuHook.removeSubscriber(aId); \
	}); \
	INLINE_MODULE_METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("select")), [=](ApiRequest& aRequest) { \
		const auto entityId = JsonUtil::getField<string>("entity_id", aRequest.getRequestBody(), false); \
		return handleClickItem<idType>( \
			aRequest,  \
			menuId, \
			std::bind(&ContextMenuManager::onClick##hook2##Item, ContextMenuManager::getInstance(), placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4), \
			idDeserializerFunc,  \
			entityId \
		); \
	}); \
	INLINE_MODULE_METHOD_HANDLER(access, METHOD_POST, (EXACT_PARAM(menuId), EXACT_PARAM("list")), [=](ApiRequest& aRequest) { \
		const auto entityId = JsonUtil::getRawField<string>("entity_id", aRequest.getRequestBody()); \
		auto entity = entityDeserializerFunc(entityId, "entity_id"); \
		return handleListItems<idType>( \
			aRequest, \
			std::bind(&ContextMenuManager::get##hook2##Menu, ContextMenuManager::getInstance(), placeholders::_1, placeholders::_2), \
			idDeserializerFunc, \
			entity \
		); \
	});
*/

namespace webserver {
	MenuApi::MenuApi(Session* aSession) : 
		HookApiModule(
			aSession,
			Access::ANY,
			{
				// "menu_item_selected",
				"queue_bundle_menuitem_selected",
				"queue_file_menuitem_selected",
				"transfer_menuitem_selected",
				"favorite_hub_menuitem_selected",
				"share_root_menuitem_selected",
				"user_menuitem_selected",
				"hinted_user_menuitem_selected",
				"hub_user_menuitem_selected",
			},
			Access::ANY
		) {

		ContextMenuManager::getInstance()->addListener(this);

		CONTEXT_MENU_HANDLER("queue_bundle", queueBundle, QueueBundle, uint32_t, defaultArrayValueParser<uint32_t>, defaultArrayValueSerializer<uint32_t>, Access::ANY);
		CONTEXT_MENU_HANDLER("queue_file", queueFile, QueueFile, uint32_t, defaultArrayValueParser<uint32_t>, defaultArrayValueSerializer<uint32_t>, Access::ANY);
		CONTEXT_MENU_HANDLER("transfer", transfer, Transfer, uint32_t, defaultArrayValueParser<uint32_t>, defaultArrayValueSerializer<uint32_t>, Access::ANY);
		CONTEXT_MENU_HANDLER("favorite_hub", favoriteHub, FavoriteHub, uint32_t, defaultArrayValueParser<uint32_t>, defaultArrayValueSerializer<uint32_t>, Access::ANY);

		/*CONTEXT_MENU_HANDLER2("share_root", shareRoot, ShareRoot, TTHValue, [](const json& aJson, const string& aFieldName) {
			auto tthStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
			return Deserializer::parseTTH(tthStr);
		}, Access::ANY);

		CONTEXT_MENU_HANDLER2("user", user, User, CID, [](const json& aJson, const string& aFieldName) {
			auto cidStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
			return Deserializer::getUser(cidStr, true)->getCID();
		}, Access::ANY);*/

		CONTEXT_MENU_HANDLER("share_root", shareRoot, ShareRoot, TTHValue, tthArrayValueParser, defaultArrayValueSerializer<TTHValue>, Access::ANY);
		CONTEXT_MENU_HANDLER("user", user, User, CID, cidArrayValueParser, defaultArrayValueSerializer<CID>, Access::ANY);
		CONTEXT_MENU_HANDLER("hinted_user", hintedUser, HintedUser, HintedUser, hintedUserArrayValueParser, Serializer::serializeHintedUser, Access::ANY);
		CONTEXT_MENU_HANDLER("hub_user", hubUser, HubUser, HintedUser, hintedUserArrayValueParser, Serializer::serializeHintedUser, Access::ANY);

		/*const auto parseFilelist = [](const json& aJson, const string& aFieldName) {
			auto cidStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
			auto user = Deserializer::getUser(cidStr, true);

			auto filelist = DirectoryListingManager::getInstance()->findList(user);
			if (!filelist) {
				JsonUtil::throwError("");
			}

			return filelist;
		};

		const auto parseSearchInstance = [](const json& aJson, const string& aFieldName) {
			auto tthStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
			auto instanceId = Deserializer::parseTTH(tthStr);
		};*/

		// ENTITY_CONTEXT_MENU_HANDLER("filelist_item", filelistItem, FilelistItem, TTHValue, tthArrayValueParser, DirectoryListingPtr, parseFilelist, Access::ANY);
		// ENTITY_CONTEXT_MENU_HANDLER("grouped_search_result", groupedSearchResult, GroupedSearchResult, TTHValue, tthArrayValueParser, SearchInstancePtr, parseSearchInstance, Access::ANY);
	}

	MenuApi::~MenuApi() {
		ContextMenuManager::getInstance()->removeListener(this);
	}

	json MenuApi::toHookData(const json& aSelectedIds, const json& aData) {
		return json({
			{ "ids", aSelectedIds },
			{ "data", aData },
		});
	}

	/*ActionHookResult<ContextMenuItemList> MenuApi::bundleMenuItemHook(const vector<uint32_t>& aSelections, const ActionHookResultGetter<ContextMenuItemList>& aResultGetter) {
		return HookCompletionData::toResult<ContextMenuItemList>(
			fireHook("queue_bundle", 1, [&]() {
				return toHookData(
					aSelections
				);
			}),
			aResultGetter,
			MenuApi::deserializeMenuItems
		);
	}

	api_return MenuApi::handleGetBundleMenuItems(ApiRequest& aRequest) {
		const auto selectedIds = JsonUtil::getField<vector<uint32_t>>("selectedIds", aRequest.getRequestBody(), false);

		const auto complete = aRequest.defer();
		addAsyncTask([=] {
			const auto items = ContextMenuManager::getInstance()->getQueueBundleMenu(selectedIds);
			complete(
				websocketpp::http::status_code::ok, 
				Serializer::serializeList(items, MenuApi::serializeMenuItem),
				nullptr
			);
		});

		return websocketpp::http::status_code::see_other;
	}

	api_return MenuApi::handleClickItem(ApiRequest& aRequest) {
		const auto selectedIds = JsonUtil::getField<vector<uint32_t>>("selectedIds", aRequest.getRequestBody(), false);
		const auto hookId = JsonUtil::getField<string>("hookId", aRequest.getRequestBody(), false);
		const auto menuItemId = JsonUtil::getField<string>("menuItemId", aRequest.getRequestBody(), false);
		const auto entityId = JsonUtil::getOptionalField<string>("entityId", aRequest.getRequestBody(), false);

		ContextMenuManager::getInstance()->onClickItem(selectedIds, hookId, menuItemId);
		return websocketpp::http::status_code::no_content;
	}*/

	TTHValue MenuApi::tthArrayValueParser(const json& aJson, const string& aFieldName) {
		auto tthStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
		return Deserializer::parseTTH(tthStr);
	}

	CID MenuApi::cidArrayValueParser(const json& aJson, const string& aFieldName) {
		auto cidStr = JsonUtil::parseValue<string>(aFieldName, aJson, false);
		return Deserializer::getUser(cidStr, true)->getCID();
	}

	HintedUser MenuApi::hintedUserArrayValueParser(const json& aJson, const string& aFieldName) {
		return Deserializer::parseHintedUser(aJson, aFieldName, true);
	}

	json MenuApi::serializeMenuItem(const ContextMenuItemPtr& aMenuItem) {
		return {
			{ "id", aMenuItem->getId() },
			{ "title", aMenuItem->getTitle() },
			{ "icon", aMenuItem->getIcon() },
			{ "hook_id", aMenuItem->getHookId() },
		};
	}

	ContextMenuItemList MenuApi::deserializeMenuItems(const json& aData, const ActionHookResultGetter<ContextMenuItemList>& aResultGetter) {
		const auto menuItemsJson = JsonUtil::getArrayField("menuitems", aData);

		ContextMenuItemList ret;
		for (const auto& menuItem: menuItemsJson) {
			ret.push_back(toMenuItem(menuItem, aResultGetter));
		}

		return ret;
	}

	ContextMenuItemPtr MenuApi::toMenuItem(const json& aData, const ActionHookResultGetter<ContextMenuItemList>& aResultGetter) {
		const auto id = JsonUtil::getField<string>("id", aData, false);
		const auto title = JsonUtil::getField<string>("title", aData, false);
		const auto icon = JsonUtil::getOptionalFieldDefault<string>("icon", aData, Util::emptyString);

		return make_shared<ContextMenuItem>(id, title, icon, aResultGetter.getId());
	}

	/*void MenuApi::on(ContextMenuManagerListener::MenuItemSelected, const vector<uint32_t>& aSelectedItems, const string& aHookId, const string& aMenuItemId) noexcept {
		maybeSend("menu_item_selected", [&]() {
			return json({
				{ "hook_id", aHookId },
				{ "menu_item_id", aMenuItemId },
				{ "selected_ids", aSelectedItems },
			});
		});
	}*/

	void MenuApi::onMenuItemSelected(const string& aMenuId, const json& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		maybeSend(aMenuId + "_menuitem_selected", [&]() {
			return json({
				{ "hook_id", aHookId },
				{ "menu_id", aMenuId },
				{ "menuitem_id", aMenuItemId },
				{ "selected_ids", aSelectedIds },
			});
		});
	}

	void MenuApi::on(ContextMenuManagerListener::QueueBundleMenuSelected, const vector<uint32_t>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("queue_bundle", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::QueueFileMenuSelected, const vector<uint32_t>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("queue_file", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::TransferMenuSelected, const vector<uint32_t>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("transfer", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::ShareRootMenuSelected, const vector<TTHValue>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("share_root", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::FavoriteHubMenuSelected, const vector<uint32_t>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("favorite_hub", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::UserMenuSelected, const vector<CID>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("user", aSelectedIds, aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::HintedUserMenuSelected, const vector<HintedUser>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("hinted_user", Serializer::serializeList(aSelectedIds, Serializer::serializeHintedUser), aHookId, aMenuItemId);
	}

	void MenuApi::on(ContextMenuManagerListener::HubUserMenuSelected, const vector<HintedUser>& aSelectedIds, const string& aHookId, const string& aMenuItemId) noexcept {
		onMenuItemSelected("hub_user", Serializer::serializeList(aSelectedIds, Serializer::serializeHintedUser), aHookId, aMenuItemId);
	}
}
