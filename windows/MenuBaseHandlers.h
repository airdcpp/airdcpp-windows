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

#include "../client/UserInfoBase.h"
#include "../client/TargetUtil.h"
#include "../client/format.h"
#include "../client/version.h"
#include "../client/QueueItem.h"
#include "../client/ShareManager.h"

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
		UserTraits() : adcOnly(true), favOnly(true), nonFavOnly(true), nmdcOnly(true) { }
		void operator()(const UserInfoBase* ui) {
			if(ui->getUser()) {
				if(ui->getUser()->isSet(User::NMDC)) 
					adcOnly = false;
				else
					nmdcOnly = false;

				bool fav = FavoriteManager::getInstance()->isFavoriteUser(ui->getUser());
				if(fav)
					nonFavOnly = false;
				if(!fav)
					favOnly = false;
			}
		}

		bool nmdcOnly;
		bool adcOnly;
		bool favOnly;
		bool nonFavOnly;
	};

	void appendUserItems(CMenu& menu, bool showFullList = true) {
		UserTraits traits = ((T*)this)->getUserList().forEachSelectedT(UserTraits()); 

		menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
		menu.AppendMenu(MF_SEPARATOR);
		if (showFullList || traits.nmdcOnly)
			menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
		if (!traits.adcOnly && !traits.nmdcOnly) // GET/BROWSE
			menu.AppendMenu(MF_STRING, IDC_GETBROWSELIST, CTSTRING(GET_BROWSE_LIST));
		menu.AppendMenu(MF_STRING, IDC_BROWSELIST, CTSTRING(BROWSE_FILE_LIST));
		menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
		menu.AppendMenu(MF_SEPARATOR);
		if(!traits.favOnly)
			menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
		if(!traits.nonFavOnly)
			menu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT_FAVUSER_HUB));
		menu.AppendMenu(MF_SEPARATOR);
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
			for(auto i = ldl.begin(); i != ldl.end(); ++i) {
				auto target = Text::fromT(*i);
				targetMenu.appendItem(i->c_str(), [this, wholeDir, target] { onDownload(target, wholeDir); });
			}
		}

		//Append TTH locations
		if(targets.size() > 0 && (type != SEARCH || !wholeDir)) {
			targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
			for(auto i = targets.begin(); i != targets.end(); ++i) {
				auto target = *i;
				targetMenu.appendItem(Text::toT(*i).c_str(), [this, target] { onDownload(target, false); });
			}
		}
	}

	void appendPriorityMenu(OMenu& aMenu, bool wholeDir) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		priorityMenu->appendItem(CTSTRING(PAUSED), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::PAUSED); });
		priorityMenu->appendItem(CTSTRING(LOWEST), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::LOWEST); });
		priorityMenu->appendItem(CTSTRING(LOW), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::LOW); });
		priorityMenu->appendItem(CTSTRING(NORMAL), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::NORMAL); });
		priorityMenu->appendItem(CTSTRING(HIGH), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::HIGH); });
		priorityMenu->appendItem(CTSTRING(HIGHEST), [this, wholeDir] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, QueueItem::HIGHEST); });
	}

	void appendVirtualItems(OMenu &targetMenu, bool wholeDir, bool isFavDirs) {
		auto l = isFavDirs ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		if (!l.empty()) {
			targetMenu.InsertSeparatorLast(isFavDirs ? TSTRING(SETTINGS_FAVORITE_DIRS_PAGE) : TSTRING(SHARED));
			for(auto i = l.begin(); i != l.end(); ++i) {
				string t = i->first;
				if (i->second.size() > 1) {
					auto vMenu = targetMenu.createSubMenu(Text::toT(i->first).c_str(), true);
					vMenu->appendItem(CTSTRING(AUTO_SELECT), [this, t, wholeDir, isFavDirs] { onDownloadVirtual(t, wholeDir, isFavDirs); });
					vMenu->appendSeparator();
					for(auto s = i->second.begin(); s != i->second.end(); ++s) {
						auto target = *s;
						vMenu->appendItem(Text::toT(*s).c_str(), [this, target, wholeDir] { onDownload(target, wholeDir); });
					}
				} else {
					targetMenu.appendItem(Text::toT(t).c_str(), [this, t, wholeDir, isFavDirs] { onDownloadVirtual(t, wholeDir, isFavDirs); });
				}
			}
		}
	}

	bool confirmDownload(TargetUtil::TargetInfo& targetInfo, int64_t aSize) {
		return (MessageBox(((T*)this)->m_hWnd, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES);
	}
};

#endif // !defined(MENUBASEHANDLER_H)