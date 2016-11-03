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

#ifndef DOWNLOADBASEHANDLER_H
#define DOWNLOADBASEHANDLER_H

#include "OMenu.h"

#include <airdcpp/FavoriteManager.h>
#include <airdcpp/ShareManager.h>
#include <airdcpp/QueueManager.h>
#include <airdcpp/TargetUtil.h>
#include <airdcpp/Util.h>

#include "BrowseDlg.h"
#include "WinUtil.h"

template<class T>
class DownloadBaseHandler {
public:
	virtual void handleDownload(const string& aTarget, Priority p, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown) = 0;

	virtual int64_t getDownloadSize(bool /*isWhole*/) { return 0;  }
	virtual bool showDirDialog(string& /*fileName*/) { return true;  }

	enum Type {
		TYPE_PRIMARY,
		TYPE_SECONDARY,
		TYPE_BOTH
	};

	/* Action handlers */
	void onDownload(const string& aTarget, bool isWhole, bool isSizeUnknown, Priority p) {
		if (!isSizeUnknown) {
			// Get the size of the download
			auto size = getDownloadSize(isWhole);

			// Check the space
			TargetUtil::TargetInfo ti(aTarget);
			if (TargetUtil::getDiskInfo(ti) && !ti.hasFreeSpace(size) && !confirmDownload(ti, size))
				return;
		}

		handleDownload(aTarget, WinUtil::isShift() ? Priority::HIGHEST : p, isWhole, TargetUtil::TARGET_PATH, isSizeUnknown);
	}

	void onDownloadTo(bool useWhole, bool isSizeUnknown) {
		string fileName;
		bool dirDlg = useWhole || ((T*)this)->showDirDialog(fileName);

		tstring targetT;

		BrowseDlg dlg(((T*)this)->m_hWnd, BrowseDlg::TYPE_GENERIC, dirDlg ? BrowseDlg::DIALOG_SELECT_FOLDER : BrowseDlg::DIALOG_SAVE_FILE);
		dlg.setPath(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
		dlg.setTitle(TSTRING(DOWNLOAD_TO));
		if (!dirDlg) {
			dlg.setFileName(Text::toT(fileName));
		}

		if (!dlg.show(targetT))
			return;

		auto target = Text::fromT(targetT);
		SettingsManager::getInstance()->addToHistory(dirDlg ? target : Util::getFilePath(target), SettingsManager::HISTORY_DOWNLOAD_DIR);
		onDownload(target, useWhole, isSizeUnknown, Priority::DEFAULT);
	}

	void onDownloadVirtual(const string& aTarget, bool isFav, bool isWhole, bool isSizeUnknown) {
		if (isSizeUnknown) {
			handleDownload(aTarget, Priority::DEFAULT, isWhole, isFav ? TargetUtil::TARGET_FAVORITE : TargetUtil::TARGET_SHARE, true);
		} else {
			auto groupedDirectories = isFav ? FavoriteManager::getInstance()->getGroupedFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

			auto p = groupedDirectories.find(aTarget);
			dcassert(p != groupedDirectories.end());
			if (p != groupedDirectories.end()) {
				TargetUtil::TargetInfo targetInfo;
				auto size = getDownloadSize(isWhole);

				if (!TargetUtil::getTarget(p->second, targetInfo, size) && !confirmDownload(targetInfo, size)) {
					return;
				}

				handleDownload(targetInfo.getTarget(), Priority::DEFAULT, isWhole, TargetUtil::TARGET_PATH, false);
			}
		}
	}


	/* Menu creation */
	void appendDownloadMenu(OMenu& aMenu, Type aType, bool isSizeUnknown, const optional<TTHValue>& aTTH, 
		const optional<string>& aPath, bool appendPrioMenu = true, bool addDefault = true) {
		aMenu.appendItem(CTSTRING(DOWNLOAD), [=] { 
			onDownload(SETTING(DOWNLOAD_DIRECTORY), aType == TYPE_SECONDARY, isSizeUnknown, Priority::DEFAULT); }, addDefault ? OMenu::FLAG_DEFAULT : 0);

		auto targetMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_TO), true);
		appendDownloadTo(*targetMenu, aType == TYPE_SECONDARY, isSizeUnknown, aTTH, aPath);

		if (appendPrioMenu)
			appendPriorityMenu(aMenu, aType == TYPE_SECONDARY, isSizeUnknown);

		if (aType == TYPE_BOTH) {
			aMenu.appendItem(CTSTRING(DOWNLOAD_WHOLE_DIR), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), true, true, Priority::DEFAULT); });
			auto targetMenuWhole = aMenu.createSubMenu(TSTRING(DOWNLOAD_WHOLE_DIR_TO), true);

			//if we have a dupe path, pick the dir from it
			optional<string> pathWhole;
			if (aPath && !(*aPath).empty() && (*aPath).back() != PATH_SEPARATOR)
				pathWhole = Util::getFilePath(*aPath);

			appendDownloadTo(*targetMenuWhole, true, true, boost::none, pathWhole);
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
		const auto& ldl = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DOWNLOAD_DIR);
		if(!ldl.empty()) {
			targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for(auto& path: ldl) {
				targetMenu.appendItem(Text::toT(path).c_str(), [=] { onDownload(path, wholeDir, isSizeUnknown, Priority::DEFAULT); });
			}

			targetMenu.appendSeparator();
			targetMenu.appendItem(TSTRING(CLEAR_HISTORY), [] { SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_DOWNLOAD_DIR); });
		}
	}

	void appendPriorityMenu(OMenu& aMenu, bool wholeDir, bool isSizeUnknown) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		auto addItem = [&](const tstring& aTitle, Priority p) -> void {
			priorityMenu->appendItem(aTitle.c_str(), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), wholeDir, isSizeUnknown, p); });
		};

		addItem(CTSTRING(PAUSED_FORCED), Priority::PAUSED_FORCE);
		addItem(CTSTRING(PAUSED), Priority::PAUSED);
		addItem(CTSTRING(LOWEST), Priority::LOWEST);
		addItem(CTSTRING(LOW), Priority::LOW);
		addItem(CTSTRING(NORMAL), Priority::NORMAL);
		addItem(CTSTRING(HIGH), Priority::HIGH);
		addItem(CTSTRING(HIGHEST), Priority::HIGHEST);
	}

	void appendVirtualItems(OMenu &targetMenu, bool wholeDir, bool isFavDirs, bool isSizeUnknown) {
		auto directoryMap = isFavDirs ? FavoriteManager::getInstance()->getGroupedFavoriteDirs() : ShareManager::getInstance()->getGroupedDirectories();

		if (directoryMap.empty()) {
			return;
		}

		targetMenu.InsertSeparatorLast(isFavDirs ? TSTRING(SETTINGS_FAVORITE_DIRS_PAGE) : TSTRING(SHARED));
		for(const auto& dp: directoryMap) {
			const auto& groupName = dp.first;
			if (dp.second.size() > 1) {
				auto vMenu = targetMenu.createSubMenu(Text::toT(groupName).c_str(), true);
				vMenu->appendItem(CTSTRING(AUTO_SELECT), [=] { onDownloadVirtual(groupName, isFavDirs, wholeDir, isSizeUnknown); });
				vMenu->appendSeparator();
				for (const auto& target: dp.second) {
					vMenu->appendItem(Text::toT(target).c_str(), [=] { onDownload(target, wholeDir, isSizeUnknown, Priority::DEFAULT); });
				}
			} else {
				targetMenu.appendItem(Text::toT(groupName).c_str(), [=] { onDownloadVirtual(groupName, isFavDirs, wholeDir, isSizeUnknown); });
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
					targetMenu.appendItem(Text::toT(target).c_str(), [=] { onDownload(target, wholeDir, isSizeUnknown, Priority::DEFAULT); });
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
						//string displayText = isDir ? Util::getParentDir(target) + " (" + Util::getLastDir(target) + ")" : target;
						string location = isDir ? Util::getParentDir(target) : target;
						targetMenu.appendItem(Text::toT(location).c_str(), [=] { onDownload(location, wholeDir, isSizeUnknown, Priority::DEFAULT); });
					}
				}
			};

			targets = QueueManager::getInstance()->getNmdcDirPaths(isDir ? *aPath : Util::getFilePath(*aPath));
			doAppend(TSTRING(QUEUED_DUPE_PATHS));

			targets = ShareManager::getInstance()->getNmdcDirPaths(isDir ? *aPath : Util::getFilePath(*aPath));
			doAppend(TSTRING(SHARED_DUPE_PATHS));
		}
	}

	bool confirmDownload(TargetUtil::TargetInfo& targetInfo, int64_t aSize) {
		return !SETTING(FREE_SPACE_WARN) || WinUtil::showQuestionBox(Text::toT(TargetUtil::formatSizeConfirmation(targetInfo, aSize)), MB_ICONQUESTION);
	}
};

#endif // !defined(DOWNLOADBASEHANDLER_H)