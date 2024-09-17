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

#ifndef USERINFOBASEHANDLER_H
#define USERINFOBASEHANDLER_H

#include "resource.h"
#include "OMenu.h"

#include <airdcpp/ClientManager.h>
#include <airdcpp/HintedUser.h>
#include <airdcpp/UserInfoBase.h>
#include <airdcpp/Util.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>

#include "ActionUtil.h"

namespace wingui {
// emulation for non-list objects
template<class T>
struct UserListHandler {
	UserListHandler(T& _aHandler) : handler(_aHandler) { }

	void forEachSelected(void (UserInfoBase::*func)()) {
		(handler.*func)();
	}

	template<class _Function>
	_Function forEachSelectedT(_Function pred) {
		pred(&handler);
		return pred;
	}

private:
	T& handler;
};

template<class T>
class UserInfoBaseHandler {
public:
	BEGIN_MSG_MAP(UserInfoBaseHandler)
		COMMAND_RANGE_HANDLER(IDC_GRANTSLOT, IDC_UNGRANTSLOT, onGrantSlot)
	END_MSG_MAP()
	bool pmItems;
	bool listItems;

	UserInfoBaseHandler(bool aAppendPmItems = true, bool aAppendListItems = true) : pmItems(aAppendPmItems), listItems(aAppendListItems) { }

	virtual void handleMatchQueue() {
		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::matchQueue, std::placeholders::_1));
	}
	virtual void handleGetList() {
		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::getList, std::placeholders::_1));
	}
	virtual void handleBrowseList() {
		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::browseList, std::placeholders::_1));
	}
	virtual void handleGetBrowseList() {
		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::getBrowseList, std::placeholders::_1));
	}

	virtual void handleFavorites() {
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::handleFav);
	}
	virtual void handlePrivateMessage() {
		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::pm, std::placeholders::_1));
	}

	virtual void handleConnectFav() {
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::connectFav);
	}
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch(wID) {
			case IDC_GRANTSLOT:		((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::grant, std::placeholders::_1)); break;
			case IDC_GRANTSLOT_DAY:	((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::grantDay, std::placeholders::_1)); break;
			case IDC_GRANTSLOT_HOUR:	((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::grantHour, std::placeholders::_1)); break;
			case IDC_GRANTSLOT_WEEK:	((T*)this)->getUserList().forEachSelectedT(bind(&UserInfoBase::grantWeek, std::placeholders::_1)); break;
			case IDC_UNGRANTSLOT:	((T*)this)->getUserList().forEachSelected(&UserInfoBase::ungrant); break;
		}
		return 0;
	}
	virtual void handleRemoveAll() { 
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::removeAll);
	}

	struct UserTraits {
		UserTraits() { }
		void operator()(const UserInfoBase* ui) {
			if(ui->getUser()) {
				if (!ActionUtil::allowGetFullList(HintedUser(ui->getUser(), ui->getHubUrl()))) {
					allFullList = false;
				} else {
					noFullList = false;
				}

				auto fav = ui->getUser()->isFavorite();
				if (fav) {
					nonFavOnly = false;
				} else {
					favOnly = false;
				}
			}
		}

		bool noFullList = true;
		bool allFullList = true;
		bool favOnly = true;
		bool nonFavOnly = true;
	};

	template<class K>
	void appendListMenu(const UserPtr& aUser, const User::UserInfoList& list, OMenu* subMenu, bool aAddShareInfo) {
		for (auto& i: list) {
			auto url = i.hubUrl;
			auto title = Text::toT(i.hubName) + (aAddShareInfo ? (_T(" (") + Util::formatBytesW(i.shared) + _T(")")) : Util::emptyStringT);
			subMenu->appendItem(title, [=] { K()(aUser, url); });
		}
	}

	void appendUserItems(OMenu& menu, bool aShowFullList = true, const UserPtr& aUser = nullptr, bool aAllowOffline = false) {
		UserTraits traits = ((T*)this)->getUserList().forEachSelectedT(UserTraits());
		bool multipleHubs = false;

		auto appendSingleDownloadItems = [&](bool hubUnknown) -> void {
			int commonFlags = aUser && hubUnknown ? OMenu::FLAG_DISABLED : 0;
			int defaultFlag = commonFlags > 0 ? commonFlags : OMenu::FLAG_DEFAULT;

			menu.appendSeparator();
			if (aShowFullList || traits.allFullList) {
				menu.appendItem(TSTRING(GET_FILE_LIST), [=] { handleGetList(); }, defaultFlag);
			}
			if (!traits.noFullList && !traits.allFullList) {
				menu.appendItem(TSTRING(GET_BROWSE_LIST), [=] { handleGetBrowseList(); }, defaultFlag);
			}
			menu.appendItem(TSTRING(BROWSE_FILE_LIST), [=] { handleBrowseList(); }, defaultFlag);
			menu.appendItem(TSTRING(MATCH_QUEUE), [=] { handleMatchQueue(); }, commonFlags);
			menu.appendSeparator();
		};

		User::UserInfoList list;
		if (aUser) {
			list = ClientManager::getInstance()->getUserInfoList(aUser);
			if (list.size() > 1) {
				multipleHubs = true;
				if (pmItems)
					appendListMenu<ActionUtil::PM>(aUser, list, menu.createSubMenu(TSTRING(SEND_PRIVATE_MESSAGE)), false);

				if (listItems) {
					//combine items in the list based on the share size
					User::UserInfoList shareList(list.begin(), list.end());
					for (auto i = shareList.begin(); i != shareList.end(); ++i) {
						StringList names { i->hubName };

						auto matchPos = i;

						//erase all following entries with the same share size
						while ((matchPos = find_if(i+1, shareList.end(), [&i](const User::UserHubInfo& uhi) { return uhi.shared == i->shared; })) != shareList.end()) {
							names.push_back(matchPos->hubName);
							shareList.erase(matchPos);
						}

						//combine the hub names with this share size
						i->hubName = Util::listToString(names);
					}

					if (shareList.size() > 1) {
						menu.appendSeparator();
						appendListMenu<ActionUtil::BrowseList>(aUser, shareList, menu.createSubMenu(TSTRING(BROWSE_FILE_LIST)), true);
						if (aShowFullList || traits.allFullList)
							appendListMenu<ActionUtil::GetList>(aUser, shareList, menu.createSubMenu(TSTRING(GET_FILE_LIST)), true);
						if (!traits.noFullList && !traits.allFullList)
							appendListMenu<ActionUtil::GetBrowseList>(aUser, shareList, menu.createSubMenu(TSTRING(GET_BROWSE_LIST)), true);
						appendListMenu<ActionUtil::MatchQueue>(aUser, shareList, menu.createSubMenu(TSTRING(MATCH_QUEUE)), true);
					} else {
						appendSingleDownloadItems(false);
					}
				}
			}
		}
		
		if (!multipleHubs) {
			if (pmItems)
				menu.appendItem(TSTRING(SEND_PRIVATE_MESSAGE), [=] { handlePrivateMessage(); }, !listItems ? OMenu::FLAG_DEFAULT : 0);

			if (listItems)
				appendSingleDownloadItems(list.empty() && !aAllowOffline ? true : false);
		}

		{
			vector<HintedUser> tokens;
			((T*)this)->getUserList().forEachSelectedT([&tokens](const UserInfoBase* uib) {
				tokens.push_back(HintedUser(uib->getUser(), uib->getHubUrl()));
			});

			EXT_CONTEXT_MENU(menu, HintedUser, tokens);
		}

		if (!traits.favOnly) {
			menu.appendItem(TSTRING(ADD_TO_FAVORITES), [=] { handleFavorites(); });
			menu.appendSeparator();
		} else if (!traits.nonFavOnly) {
			menu.appendItem(TSTRING(REMOVE_FAVORITE_USER), [=] { handleFavorites(); });
			menu.appendSeparator();
		}

		menu.appendItem(TSTRING(REMOVE_FROM_ALL), [=] { handleRemoveAll(); });

		auto& grantMenu = *menu.createSubMenu(TSTRING(GRANT_SLOTS_MENU), true);
		grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
		grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
		grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
		grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
		grantMenu.AppendMenu(MF_SEPARATOR);
		grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
	}	
};
}

#endif // !defined(USERINFOBASEHANDLER_H)