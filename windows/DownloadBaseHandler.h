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
#include "../client/QueueManager.h"
#include "../client/TargetUtil.h"
#include "../client/Util.h"
#include "../client/version.h"

#include "WinUtil.h"

template<class T>
class DownloadBaseHandler {
public:
	virtual void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown) = 0;

	virtual int64_t getDownloadSize(bool /*isWhole*/) { return 0;  }
	virtual bool showDirDialog(string& /*fileName*/) { return true;  }

	enum Type {
		TYPE_PRIMARY,
		TYPE_SECONDARY,
		TYPE_BOTH
	};

	/* Action handlers */
	void onDownload(const string& aTarget, bool isWhole, bool isSizeUnknown, QueueItemBase::Priority p) {
		if (!isSizeUnknown) {
			/* Get the size of the download */
			int64_t size = getDownloadSize(isWhole);
			/* Check the space */
			TargetUtil::TargetInfo ti;
			ti.targetDir = aTarget;
			if (TargetUtil::getDiskInfo(ti) && ti.getFreeSpace() < size && !confirmDownload(ti, size))
				return;
		}

		handleDownload(aTarget, WinUtil::isShift() ? QueueItem::HIGHEST : p, isWhole, TargetUtil::TARGET_PATH, isSizeUnknown);
	}

	void onDownloadTo(bool useWhole, bool isSizeUnknown) {
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
		onDownload(Text::fromT(target), useWhole, isSizeUnknown, QueueItemBase::DEFAULT);
	}

	void onDownloadVirtual(const string& aTarget, bool isFav, bool isWhole, bool isSizeUnknown) {
		if (isSizeUnknown) {
			handleDownload(aTarget, QueueItem::DEFAULT, isWhole, isFav ? TargetUtil::TARGET_FAVORITE : TargetUtil::TARGET_SHARE, true);
		} else {
			auto list = isFav ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();
			auto p = find_if(list.begin(), list.end(), CompareFirst<string, StringList>(aTarget));
			dcassert(p != list.end());

			TargetUtil::TargetInfo targetInfo;
			int64_t size = getDownloadSize(isWhole);

			if (!TargetUtil::getTarget(p->second, targetInfo, size) && !confirmDownload(targetInfo, size)) {
				return;
			}

			handleDownload(targetInfo.targetDir, QueueItem::DEFAULT, isWhole, TargetUtil::TARGET_PATH, false);
		}
	}


	/* Menu creation */
	void appendDownloadMenu(OMenu& aMenu, Type aType, bool isSizeUnknown, const optional<TTHValue>& aTTH, const optional<string>& aPath, bool appendPrioMenu = true) {
		aMenu.appendItem(CTSTRING(DOWNLOAD), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), aType == TYPE_SECONDARY, isSizeUnknown, QueueItemBase::DEFAULT); }, OMenu::FLAG_DEFAULT);

		auto targetMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_TO), true);
		appendDownloadTo(*targetMenu, aType == TYPE_SECONDARY, isSizeUnknown, aTTH, aPath);

		if (appendPrioMenu)
			appendPriorityMenu(aMenu, aType == TYPE_SECONDARY, isSizeUnknown);

		if (aType == TYPE_BOTH) {
			aMenu.appendItem(CTSTRING(DOWNLOAD_WHOLE_DIR), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), true, true, QueueItemBase::DEFAULT); });
			auto targetMenuWhole = aMenu.createSubMenu(TSTRING(DOWNLOAD_WHOLE_DIR_TO), true);

			//if we have a dupe path, pick the dir from it
			optional<string> pathWhole;
			if (aPath && !(*aPath).empty() && (*aPath).back() != PATH_SEPARATOR)
				pathWhole = Util::getFilePath(*aPath);

			appendDownloadTo(*targetMenuWhole, true, true, nullptr, pathWhole);
		}
	}

	void appendDownloadTo(OMenu& targetMenu, bool wholeDir, bool isSizeUnknown, const optional<TTHValue>& aTTH, const optional<string>& aPath) {
		targetMenu.appendItem(CTSTRING(BROWSE), [=] { onDownloadTo(wholeDir, isSizeUnknown); });

		//Append shared and favorite directories
		if (SETTING(SHOW_SHARED_DIRS_FAV)) {
			appendVirtualItems(targetMenu, wholeDir, false, isSizeUnknown);
		}
		appendVirtualItems(targetMenu, wholeDir, true, isSizeUnknown);

		appendTargets(targetMenu, wholeDir, isSizeUnknown, aTTH, aPath);

		//Append dir history
		const auto& ldl = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DIR);
		if(!ldl.empty()) {
			targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for(auto& i: ldl) {
				auto target = Text::fromT(i);
				targetMenu.appendItem(i.c_str(), [=] { onDownload(target, wholeDir, isSizeUnknown, QueueItemBase::DEFAULT); });
			}

			targetMenu.appendSeparator();
			targetMenu.appendItem(TSTRING(CLEAR_HISTORY), [] { SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_DIR); });
		}
	}

	void appendPriorityMenu(OMenu& aMenu, bool wholeDir, bool isSizeUnknown) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		auto addItem = [&] (const tstring& aTitle, QueueItemBase::Priority p) -> void {
			priorityMenu->appendItem(aTitle.c_str(), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, isSizeUnknown, p); });
		};

		addItem(CTSTRING(PAUSED_FORCED), QueueItem::PAUSED_FORCE);
		addItem(CTSTRING(PAUSED), QueueItem::PAUSED);
		addItem(CTSTRING(LOWEST), QueueItem::LOWEST);
		addItem(CTSTRING(LOW), QueueItem::LOW);
		addItem(CTSTRING(NORMAL), QueueItem::NORMAL);
		addItem(CTSTRING(HIGH), QueueItem::HIGH);
		addItem(CTSTRING(HIGHEST), QueueItem::HIGHEST);
	}

	void appendVirtualItems(OMenu &targetMenu, bool wholeDir, bool isFavDirs, bool isSizeUnknown) {
		auto l = isFavDirs ? FavoriteManager::getInstance()->getFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		if (!l.empty()) {
			targetMenu.InsertSeparatorLast(isFavDirs ? TSTRING(SETTINGS_FAVORITE_DIRS_PAGE) : TSTRING(SHARED));
			for(auto& i: l) {
				string t = i.first;
				if (i.second.size() > 1) {
					auto vMenu = targetMenu.createSubMenu(Text::toT(i.first).c_str(), true);
					vMenu->appendItem(CTSTRING(AUTO_SELECT), [=] { onDownloadVirtual(t, isFavDirs, wholeDir, isSizeUnknown); });
					vMenu->appendSeparator();
					for(auto& target: i.second) {
						vMenu->appendItem(Text::toT(target).c_str(), [=] { onDownload(target, wholeDir, isSizeUnknown, QueueItemBase::DEFAULT); });
					}
				} else {
					targetMenu.appendItem(Text::toT(t).c_str(), [=] { onDownloadVirtual(t, isFavDirs, wholeDir, isSizeUnknown); });
				}
			}
		}
	}

	void appendTargets(OMenu& targetMenu, bool wholeDir, bool isSizeUnknown, const optional<TTHValue>& aTTH, const optional<string>& aPath) {
		// append TTH locations
		StringList tthTargets;
		if (aTTH) {
			tthTargets = QueueManager::getInstance()->getTargets(*aTTH);
			if (tthTargets.size()) {
				targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
				for (auto& target : tthTargets) {
					targetMenu.appendItem(Text::toT(target).c_str(), [=] { onDownload(target, wholeDir, isSizeUnknown, QueueItemBase::DEFAULT); });
				}
			}
		}

		// append path matches
		if (aPath) {
			bool isDir = !(*aPath).empty() && (*aPath).back() == PATH_SEPARATOR;

			StringList targets;
			auto doAppend = [&](const tstring& aTitle) {
				//don't list TTH paths again in here
				for (auto i = targets.begin(); i != targets.end();) {
					if (find_if(tthTargets.begin(), tthTargets.end(), [i](const string& aTarget) { return Util::getFilePath(aTarget) == *i; }) != tthTargets.end()) {
						i = targets.erase(i);
					} else {
						i++;
					}
				}

				if (!targets.empty()) {
					targetMenu.InsertSeparatorLast(aTitle);
					for (auto& target : targets) {
						//use the parent if it's a dir
						string displayText = isDir ? Util::getParentDir(target) + " (" + Util::getLastDir(target) + ")" : target;
						targetMenu.appendItem(Text::toT(displayText).c_str(), [=] { onDownload(isDir ? Util::getParentDir(target) : target, wholeDir, isSizeUnknown, QueueItemBase::DEFAULT); });
					}
				}
			};

			targets = QueueManager::getInstance()->getDirPaths(isDir ? *aPath : Util::getFilePath(*aPath));
			doAppend(TSTRING(QUEUED_DUPE_PATHS));

			targets = ShareManager::getInstance()->getDirPaths(isDir ? *aPath : Util::getFilePath(*aPath));
			doAppend(TSTRING(SHARED_DUPE_PATHS));
		}
	}

	bool confirmDownload(TargetUtil::TargetInfo& targetInfo, int64_t aSize) {
		//return WinUtil::MessageBoxConfirm(SettingsManager::FREE_SPACE_WARN, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)));
		return !SETTING(FREE_SPACE_WARN) || (MessageBox(((T*)this)->m_hWnd, Text::toT(TargetUtil::getInsufficientSizeMessage(targetInfo, aSize)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES);
	}
};

#endif // !defined(DOWNLOADBASEHANDLER_H)