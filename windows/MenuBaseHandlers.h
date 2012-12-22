/*
 * Copyright (C) 2011-2012 AirDC++ Project
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

#ifndef MENUBASEHANDLER_H
#define MENUBASEHANDLER_H

#include "resource.h"
#include "OMenu.h"

#include "../client/ClientManager.h"
#include "../client/FavoriteManager.h"
#include "../client/format.h"
#include "../client/QueueItem.h"
#include "../client/ShareManager.h"
#include "../client/TargetUtil.h"
#include "../client/UserInfoBase.h"
#include "../client/Util.h"
#include "../client/version.h"

#include "WinUtil.h"
#include <boost/bind.hpp>

template<class T>
class UserInfoBaseHandler {
public:
	BEGIN_MSG_MAP(UserInfoBaseHandler)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_RANGE_HANDLER(IDC_GRANTSLOT, IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_CONNECT, onConnectFav)
		COMMAND_ID_HANDLER(IDC_GETBROWSELIST, onGetBrowseList)
	END_MSG_MAP()
	bool pmItems;
	bool listItems;

	UserInfoBaseHandler(bool appendPmItems=true, bool appendListItems=true) : pmItems(appendPmItems), listItems(appendListItems) { }
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::matchQueue, _1));
		return 0;
	}
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::getList, _1));
		return 0;
	}
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::browseList, _1));
		return 0;
	}
	LRESULT onGetBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::getBrowseList, _1));
		return 0;
	}
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::addFav);
		return 0;
	}
	virtual LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::pm, _1));
		return 0;
	}
	LRESULT onConnectFav(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::connectFav);
		return 0;
	}
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch(wID) {
			case IDC_GRANTSLOT:		((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::grant, _1)); break;
			case IDC_GRANTSLOT_DAY:	((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::grantDay, _1)); break;
			case IDC_GRANTSLOT_HOUR:	((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::grantHour, _1)); break;
			case IDC_GRANTSLOT_WEEK:	((T*)this)->getUserList().forEachSelectedT(boost::bind(&UserInfoBase::grantWeek, _1)); break;
			case IDC_UNGRANTSLOT:	((T*)this)->getUserList().forEachSelected(&UserInfoBase::ungrant); break;
		}
		return 0;
	}
	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
		((T*)this)->getUserList().forEachSelected(&UserInfoBase::removeAll);
		return 0;
	}

	struct UserTraits {
		UserTraits() : noFullList(true), favOnly(true), nonFavOnly(true), allFullList(true) { }
		void operator()(const UserInfoBase* ui) {
			if(ui->getUser()) {
				if(!ui->getUser()->isSet(User::NMDC) && ClientManager::getInstance()->getShareInfo(HintedUser(ui->getUser(), ui->getHubUrl())).second > SETTING(FULL_LIST_DL_LIMIT)) { 
					allFullList = false;
				} else {
					noFullList = false;
				}

				bool fav = FavoriteManager::getInstance()->isFavoriteUser(ui->getUser());
				if(fav)
					nonFavOnly = false;
				if(!fav)
					favOnly = false;
			}
		}

		bool noFullList;
		bool allFullList;
		bool favOnly;
		bool nonFavOnly;
	};

	template<class K>
	void appendListMenu(UserPtr aUser, const User::UserInfoList& list, OMenu* subMenu, bool addShareInfo) {
		for (auto& i: list) {
			string url = i.hubUrl;
			subMenu->appendItem(Text::toT(i.hubName) + (addShareInfo ? (_T(" (") + Util::formatBytesW(i.shared) + _T(")")) : Util::emptyStringT), 
				[this, aUser, url] { K()(aUser, url); });
		}
	}

	void appendUserItems(OMenu& menu, bool showFullList = true, UserPtr aUser = nullptr) {
		UserTraits traits = ((T*)this)->getUserList().forEachSelectedT(UserTraits());
		bool multipleHubs = false;

		auto appendSingleDownloadItems = [&menu, traits, showFullList] () -> void {
			menu.AppendMenu(MF_SEPARATOR);
			int defaultItem = 0;
			if (showFullList || traits.allFullList) {
				menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
				defaultItem = IDC_GETLIST;
			}
			if (!traits.noFullList && !traits.allFullList) {
				menu.AppendMenu(MF_STRING, IDC_GETBROWSELIST, CTSTRING(GET_BROWSE_LIST));
				defaultItem = IDC_GETBROWSELIST;
			}
			menu.AppendMenu(MF_STRING, IDC_BROWSELIST, CTSTRING(BROWSE_FILE_LIST));
			menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
			menu.AppendMenu(MF_SEPARATOR);

			menu.SetMenuDefaultItem(defaultItem > 0 ? defaultItem : IDC_BROWSELIST);
		};

		if (aUser) {
			User::UserInfoList list;
			ClientManager::getInstance()->getUserInfoList(aUser, list);
			if (list.size() > 1) {
				multipleHubs = true;
				if (pmItems)
					appendListMenu<WinUtil::PM>(aUser, list, menu.createSubMenu(CTSTRING(SEND_PRIVATE_MESSAGE)), false);

				if (listItems) {
					//combine items in the list based on the share size
					User::UserInfoList shareList(list.begin(), list.end());
					for (auto i = shareList.begin(); i != shareList.end(); ++i) {
						StringList names;
						names.push_back(i->hubName);

						auto matchPos = i;
						while ((matchPos = find_if(i+1, shareList.end(), [i](User::UserHubInfo uhi) { return uhi.shared == i->shared; })) != shareList.end()) {
							names.push_back(matchPos->hubName);
							shareList.erase(matchPos);
						}
						i->hubName = Util::toString(names);
					}

					if (shareList.size() > 1) {
						menu.AppendMenu(MF_SEPARATOR);
						appendListMenu<WinUtil::BrowseList>(aUser, shareList, menu.createSubMenu(CTSTRING(BROWSE_FILE_LIST)), true);
						if (showFullList || traits.allFullList)
							appendListMenu<WinUtil::GetList>(aUser, shareList, menu.createSubMenu(CTSTRING(GET_FILE_LIST)), true);
						if (!traits.noFullList && !traits.allFullList)
							appendListMenu<WinUtil::GetBrowseList>(aUser, shareList, menu.createSubMenu(CTSTRING(GET_BROWSE_LIST)), true);
						appendListMenu<WinUtil::MatchQueue>(aUser, shareList, menu.createSubMenu(CTSTRING(MATCH_QUEUE)), true);
						//menu.AppendMenu(MF_SEPARATOR);
					} else {
						appendSingleDownloadItems();
					}
				}

				//if(!traits.nonFavOnly)
				//	appendListMenu<WinUtil::ConnectFav>(aUser, list, menu.createSubMenu(CTSTRING(CONNECT_FAVUSER_HUB)), false);
			}
		} 
		
		if (!multipleHubs) {
			if (pmItems)
				menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));

			if (listItems) {
				appendSingleDownloadItems();
			} else {
				menu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
			}

			//if(!traits.nonFavOnly)
			//	menu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT_FAVUSER_HUB));
		}

		if(!traits.favOnly) {
			menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
			menu.AppendMenu(MF_SEPARATOR);
		}
		menu.AppendMenu(MF_STRING, IDC_REMOVEALL, CTSTRING(REMOVE_FROM_ALL));
		menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	}	
};


template<class T>
class DownloadBaseHandler {
public:
	enum Type {
		FILELIST,
		SEARCH,
		AUTO_SEARCH,
		MAGNET,
	};

	StringList targets;
	Type type;
	bool isSizeUnknown;


	/* Action handlers */
	void onDownload(const string& aTarget, bool isWhole, QueueItem::Priority p = QueueItem::DEFAULT) {
		if (!isSizeUnknown) {
			/* Get the size of the download */
			int64_t size = ((T*)this)->getDownloadSize(isWhole);
			/* Check the space */
			TargetUtil::TargetInfo ti;
			ti.targetDir = aTarget;
			if (TargetUtil::getDiskInfo(ti) && ti.getFreeSpace() < size && !confirmDownload(ti, size))
				return;
		}

		((T*)this)->handleDownload(aTarget, WinUtil::isShift() ? QueueItem::HIGHEST : p, isWhole, TargetUtil::TARGET_PATH, isSizeUnknown);
	}

	void onDownloadTo(bool useWhole) {
		string fileName;
		bool showDirDialog = useWhole || ((T*)this)->showDirDialog(fileName);

		tstring target;
		if (showDirDialog) {
			target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(!WinUtil::browseDirectory(target, ((T*)this)->m_hWnd))
				return;

			SettingsManager::getInstance()->addDirToHistory(target);
		} else {
			target = Text::toT(SETTING(DOWNLOAD_DIRECTORY) + fileName);
			if(!WinUtil::browseFile(target, ((T*)this)->m_hWnd))
				return;

			SettingsManager::getInstance()->addDirToHistory(Util::getFilePath(target));
		}
		onDownload(Text::fromT(target), useWhole);
	}

	void onDownloadVirtual(const string& aTarget, bool isFav, bool isWhole) {
		if (isSizeUnknown || (type == SEARCH && isWhole)) {
			((T*)this)->handleDownload(aTarget, QueueItem::DEFAULT, isWhole, isFav ? TargetUtil::TARGET_FAVORITE : TargetUtil::TARGET_SHARE, true);
		} else {
			auto list = isFav ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();
			auto p = find_if(list.begin(), list.end(), CompareFirst<string, StringList>(aTarget));
			dcassert(p != list.end());

			TargetUtil::TargetInfo targetInfo;
			int64_t size = ((T*)this)->getDownloadSize(isWhole);

			if (!TargetUtil::getTarget(p->second, targetInfo, size) && !confirmDownload(targetInfo, size)) {
				return;
			}

			((T*)this)->handleDownload(targetInfo.targetDir, QueueItem::DEFAULT, isWhole, TargetUtil::TARGET_PATH, false);
		}
	}


	/* Menu creation */
	void appendDownloadMenu(OMenu& aMenu, Type aType, bool isWhole, bool aUseVirtualDir) {
		type = aType;
		isSizeUnknown = aUseVirtualDir;
		((T*)this)->appendDownloadItems(aMenu, isWhole);
	}

	void appendDownloadTo(OMenu& targetMenu, bool wholeDir) {
		targetMenu.appendItem(CTSTRING(BROWSE), [this, wholeDir] { onDownloadTo(wholeDir); });

		//Append shared and favorite directories
		if (SETTING(SHOW_SHARED_DIRS_FAV)) {
			appendVirtualItems(targetMenu, wholeDir, false);
		}
		appendVirtualItems(targetMenu, wholeDir, true);

		//Append dir history
		auto ldl = SettingsManager::getInstance()->getDirHistory();
		if(!ldl.empty()) {
			targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for(auto& i: ldl) {
				auto target = Text::fromT(i);
				targetMenu.appendItem(i.c_str(), [=] { onDownload(target, wholeDir); });
			}

			targetMenu.appendSeparator();
			targetMenu.appendItem(TSTRING(CLEAR_HISTORY), [] { SettingsManager::getInstance()->clearDirHistory(); });
		}

		//Append TTH locations
		if(targets.size() > 0 && (type != SEARCH || !wholeDir)) {
			targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
			for(auto& target: targets) {
				targetMenu.appendItem(Text::toT(target).c_str(), [=] { onDownload(target, false); });
			}
		}
	}

	void appendPriorityMenu(OMenu& aMenu, bool wholeDir) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		auto addItem = [&] (const tstring& aTitle, QueueItem::Priority p) -> void {
			priorityMenu->appendItem(aTitle.c_str(), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, p); });
		};

		addItem(CTSTRING(PAUSED), QueueItem::PAUSED);
		addItem(CTSTRING(LOWEST), QueueItem::LOWEST);
		addItem(CTSTRING(LOW), QueueItem::LOW);
		addItem(CTSTRING(NORMAL), QueueItem::NORMAL);
		addItem(CTSTRING(HIGH), QueueItem::HIGH);
		addItem(CTSTRING(HIGHEST), QueueItem::HIGHEST);
	}

	void appendVirtualItems(OMenu &targetMenu, bool wholeDir, bool isFavDirs) {
		auto l = isFavDirs ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		if (!l.empty()) {
			targetMenu.InsertSeparatorLast(isFavDirs ? TSTRING(SETTINGS_FAVORITE_DIRS_PAGE) : TSTRING(SHARED));
			for(auto& i: l) {
				string t = i.first;
				if (i.second.size() > 1) {
					auto vMenu = targetMenu.createSubMenu(Text::toT(i.first).c_str(), true);
					vMenu->appendItem(CTSTRING(AUTO_SELECT), [=] { onDownloadVirtual(t, isFavDirs, wholeDir); });
					vMenu->appendSeparator();
					for(auto& target: i.second) {
						vMenu->appendItem(Text::toT(target).c_str(), [=] { onDownload(target, wholeDir); });
					}
				} else {
					targetMenu.appendItem(Text::toT(t).c_str(), [=] { onDownloadVirtual(t, isFavDirs, wholeDir); });
				}
			}
		}
	}

	bool confirmDownload(TargetUtil::TargetInfo& targetInfo, int64_t aSize) {
		return (MessageBox(((T*)this)->m_hWnd, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES);
	}
};

#endif // !defined(MENUBASEHANDLER_H)