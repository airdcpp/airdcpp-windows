/*
 * Copyright (C) 2011-2013 AirDC++ Project
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

#ifndef DOWNLOADBASEHANDLER_H
#define DOWNLOADBASEHANDLER_H

#include "OMenu.h"

#include "../client/FavoriteManager.h"
#include "../client/ShareManager.h"
#include "../client/TargetUtil.h"
#include "../client/Util.h"
#include "../client/version.h"

#include "WinUtil.h"

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
	void onDownload(const string& aTarget, bool isWhole, QueueItemBase::Priority p = QueueItem::DEFAULT) {
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

		} else {
			target = Text::toT(SETTING(DOWNLOAD_DIRECTORY) + fileName);
			if(!WinUtil::browseFile(target, ((T*)this)->m_hWnd))
				return;

		}
		SettingsManager::getInstance()->addToHistory(showDirDialog ? target : Util::getFilePath(target), SettingsManager::HISTORY_DIR);
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
		const auto& ldl = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DIR);
		if(!ldl.empty()) {
			targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for(auto& i: ldl) {
				auto target = Text::fromT(i);
				targetMenu.appendItem(i.c_str(), [=] { onDownload(target, wholeDir); });
			}

			targetMenu.appendSeparator();
			targetMenu.appendItem(TSTRING(CLEAR_HISTORY), [] { SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_DIR); });
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

		auto addItem = [&] (const tstring& aTitle, QueueItemBase::Priority p) -> void {
			priorityMenu->appendItem(aTitle.c_str(), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, p); });
		};

		addItem(CTSTRING(PAUSED_FORCED), QueueItem::PAUSED_FORCE);
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
		//return WinUtil::MessageBoxConfirm(SettingsManager::FREE_SPACE_WARN, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)));
		return !SETTING(FREE_SPACE_WARN) || (MessageBox(((T*)this)->m_hWnd, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES);
	}
};

#endif // !defined(DOWNLOADBASEHANDLER_H)