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

#if !defined(MENUBASEHANDLER_H)
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
	BEGIN_MSG_MAP(DownloadBaseHandler)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadDir)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadTo)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_SHARED_DIRS + 999, onDownloadVirtual)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS_RANGE, IDC_DOWNLOAD_WHOLE_SHARED_DIRS_RANGE+1999, onDownloadRangeDir)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET+999, onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_PREVIOUS_DIRS, IDC_PREVIOUS_DIRS_WHOLE+999, onDownloadPrevious)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST+90, onDownloadWithPrio)
	END_MSG_MAP()

	enum Type {
		FILELIST,
		SEARCH,
		AUTO_SEARCH,
		MAGNET,
	};

	StringList targets;
	Type type;
	bool isSizeUnknown;

	bool useVirtualDir(bool aIsWhole) {
		return isSizeUnknown || (type == SEARCH && aIsWhole);
	}

	LRESULT onDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		download(SETTING(DOWNLOAD_DIRECTORY), false);
		return 0;
	}

	LRESULT onDownloadDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		download(SETTING(DOWNLOAD_DIRECTORY), true);
		return 0;
	}

	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		download(targets[IDC_DOWNLOAD_TARGET-wID], false);
		return 0;
	}

	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD wID, HWND m_hWnd, BOOL& /*bHandled*/) {
		auto useWhole = wID == IDC_DOWNLOADDIRTO;
		string fileName;
		bool showDirDialog = useWhole || ((T*)this)->showDirDialog(fileName);

		tstring target;
		if (showDirDialog) {
			target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(!WinUtil::browseDirectory(target, ((T*)this)->m_hWnd))
				return false;

			SettingsManager::getInstance()->addDirToHistory(target);
		} else {
			target = Text::toT(SETTING(DOWNLOAD_DIRECTORY) + fileName);
			if(!WinUtil::browseFile(target, ((T*)this)->m_hWnd))
				return false;

			SettingsManager::getInstance()->addDirToHistory(Util::getFilePath(target));
		}
		download(Text::fromT(target), useWhole);
		return 0;
	}

	LRESULT onDownloadPrevious(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		auto useWhole = wID >= IDC_PREVIOUS_DIRS_WHOLE;
		int newId = wID - (useWhole ? IDC_PREVIOUS_DIRS_WHOLE : IDC_PREVIOUS_DIRS);

		string target = Text::fromT(SettingsManager::getInstance()->getDirHistory()[newId]);

		download(target, useWhole);
		return 0;
	}

	LRESULT onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		auto useWhole = wID > IDC_PRIORITY_HIGHEST;
		QueueItem::Priority p;
		switch(useWhole ? wID-90 : wID) {
			case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
			case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
			case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
			case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
			case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
			case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
			default: p = QueueItem::DEFAULT; break;
		}
		download(SETTING(DOWNLOAD_DIRECTORY), useWhole, p);
		return 0;
	}

	LRESULT onDownloadRangeDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int newId;
		bool isFav=true, isWhole=true;
		if (wID >= IDC_DOWNLOAD_WHOLE_SHARED_DIRS_RANGE) {
			newId = wID - IDC_DOWNLOAD_WHOLE_SHARED_DIRS_RANGE;
			isFav = false;
		} else if (wID >= IDC_DOWNLOAD_SHARED_DIRS_RANGE) {
			newId = wID - IDC_DOWNLOAD_SHARED_DIRS_RANGE;
			isFav = false;
			isWhole = false;
		} else if (wID >= IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS_RANGE) {
			newId = wID - IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS_RANGE;
		} else {
			isWhole = false;
			newId = wID - IDC_DOWNLOAD_FAVORITE_DIRS_RANGE;
		}

		dcassert(newId >= 0);
		auto l = isFav ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		int counter=0;
		for(auto i = l.begin(); i != l.end(); ++i) {
			if (i->second.size() > 1) {
				for(auto s = i->second.begin(); s != i->second.end(); ++s) {
					if (counter == newId) {
						download(*s, isWhole);
						return 0;
					}
					counter++;
				}
			}
		}
		return 0;
	}

	LRESULT onDownloadVirtual(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int newId;
		bool isFav=true, isWhole=true;
		if (wID >= IDC_DOWNLOAD_WHOLE_SHARED_DIRS) {
			newId = wID - IDC_DOWNLOAD_WHOLE_SHARED_DIRS;
			isFav = false;
		} else if (wID >= IDC_DOWNLOAD_SHARED_DIRS) {
			newId = wID - IDC_DOWNLOAD_SHARED_DIRS;
			isFav = false;
			isWhole = false;
		} else if (wID >= IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS) {
			newId = wID - IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS;
		} else {
			isWhole = false;
			newId = wID - IDC_DOWNLOAD_FAVORITE_DIRS;
		}

		dcassert(newId >= 0);

		if (useVirtualDir(isWhole)) {
			string target = isFav ? FavoriteManager::getInstance()->getFavoriteDirs()[newId].first : ShareManager::getInstance()->getGroupedDirectories()[newId].first;
			((T*)this)->download(target, QueueItem::DEFAULT, isWhole, isFav ? TargetUtil::TARGET_FAVORITE : TargetUtil::TARGET_SHARE, true);
		} else {
			StringList targets = isFav ? FavoriteManager::getInstance()->getFavoriteDirs()[newId].second : ShareManager::getInstance()->getGroupedDirectories()[newId].second;
			TargetUtil::TargetInfo targetInfo;
			int64_t size = ((T*)this)->getDownloadSize(isWhole);

			if (!TargetUtil::getTarget(targets, targetInfo, size) && !confirmDownload(targetInfo, size)) {
				return 0;
			}

			((T*)this)->download(targetInfo.targetDir, QueueItem::DEFAULT, isWhole, TargetUtil::TARGET_PATH, false);
		}
		return 0;
	}

	void download(const string& aTarget, bool isWhole, QueueItem::Priority p = QueueItem::DEFAULT) {
		if (!isSizeUnknown) {
			/* Get the size of the download */
			int64_t size = ((T*)this)->getDownloadSize(isWhole);
			/* Check the space */
			TargetUtil::TargetInfo ti;
			ti.targetDir = aTarget;
			if (TargetUtil::getDiskInfo(ti) && ti.getFreeSpace() < size && !confirmDownload(ti, size))
				return;
		}

		((T*)this)->download(aTarget, WinUtil::isShift() ? QueueItem::HIGHEST : p, isWhole, TargetUtil::TARGET_PATH, isSizeUnknown);
	}

	void appendDownloadTo(OMenu& targetMenu, bool wholeDir) {
		targetMenu.AppendMenu(MF_STRING, (wholeDir ? IDC_DOWNLOADDIRTO : IDC_DOWNLOADTO), CTSTRING(BROWSE));

		//Append shared and favorite directories
		if (SETTING(SHOW_SHARED_DIRS_FAV)) {
			appendVirtualItems(targetMenu, wholeDir, false);
		}
		appendVirtualItems(targetMenu, wholeDir, true);

		//Append dir history
		int n = 0;
		auto ldl = SettingsManager::getInstance()->getDirHistory();
		if(!ldl.empty()) {
			targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for(auto i = ldl.begin(); i != ldl.end(); ++i) {
				targetMenu.AppendMenu(MF_STRING, (wholeDir ? IDC_PREVIOUS_DIRS_WHOLE : IDC_PREVIOUS_DIRS) + n, i->c_str());
				++n;
			}
		}

		//Append TTH locations
		if(targets.size() > 0 && (type != SEARCH || !wholeDir)) {
			n = 0;
			targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
			for(auto i = targets.begin(); i != targets.end(); ++i) {
				targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, Text::toT(Util::getFileName(*i)).c_str());
				n++;
			}
		}
	}

	void appendDownloadMenu(OMenu& aMenu, Type aType, bool isWhole, bool aUseVirtualDir) {
		type = aType;
		isSizeUnknown = aUseVirtualDir;
		((T*)this)->appendDownloadItems(aMenu, isWhole);
	}

	void appendPriorityMenu(OMenu& aMenu, bool wholeDir) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED + (wholeDir ? 90 : 0), CTSTRING(PAUSED));
		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST + (wholeDir ? 90 : 0), CTSTRING(LOWEST));
		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_LOW + (wholeDir ? 90 : 0), CTSTRING(LOW));
		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL + (wholeDir ? 90 : 0), CTSTRING(NORMAL));
		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_HIGH + (wholeDir ? 90 : 0), CTSTRING(HIGH));
		priorityMenu->AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST + (wholeDir ? 90 : 0), CTSTRING(HIGHEST));
	}

	void appendVirtualItems(OMenu &targetMenu, bool wholeDir, bool isFavDirs) {
		auto l = isFavDirs ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		auto getIDC = [wholeDir, isFavDirs](bool isSubDir) {
			return isSubDir ? (
					isFavDirs ? (
						(wholeDir ? IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS_RANGE : IDC_DOWNLOAD_FAVORITE_DIRS_RANGE)
					) : (
						(wholeDir ? IDC_DOWNLOAD_WHOLE_SHARED_DIRS_RANGE : IDC_DOWNLOAD_SHARED_DIRS_RANGE)
					)
				) : isFavDirs ? (
						(wholeDir ? IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS : IDC_DOWNLOAD_FAVORITE_DIRS)
					) : (
						(wholeDir ? IDC_DOWNLOAD_WHOLE_SHARED_DIRS : IDC_DOWNLOAD_SHARED_DIRS)
					);
		};

		int subCounter=0, virtualCounter=0;
		if (!l.empty()) {
			targetMenu.InsertSeparatorLast(isFavDirs ? TSTRING(SETTINGS_FAVORITE_DIRS_PAGE) : TSTRING(SHARED));
			for(auto i = l.begin(); i != l.end(); ++i, ++virtualCounter) {
				if (i->second.size() > 1) {
					OMenu* pathMenu = targetMenu.createSubMenu(Text::toT(i->first));
					pathMenu->AppendMenu(MF_STRING, getIDC(false) + virtualCounter, CTSTRING(AUTO_SELECT));
					pathMenu->AppendMenu(MF_SEPARATOR);
					for(auto s = i->second.begin(); s != i->second.end(); ++s) {
						pathMenu->AppendMenu(MF_STRING, getIDC(true) + subCounter, Text::toT(*s).c_str());
						subCounter++;
					}
				} else {
					targetMenu.AppendMenu(MF_STRING, getIDC(false) + virtualCounter, Text::toT(i->first).c_str());
				}
			}
		}
	}

	bool confirmDownload(TargetUtil::TargetInfo& targetInfo, int64_t aSize) {
		return (MessageBox(((T*)this)->m_hWnd, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES);
	}
};

#endif // !defined(MENUBASEHANDLER_H)