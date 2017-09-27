/*
 * Copyright (C) 2011-2017 AirDC++ Project
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
#include <airdcpp/Util.h>

#include "BrowseDlg.h"
#include "WinUtil.h"

template<class T>
class DownloadBaseHandler {
public:
	virtual void handleDownload(const string& aTarget, Priority p, bool usingTree) = 0;

	virtual int64_t getDownloadSize(bool /*isWhole*/) { return 0;  }
	virtual bool showDirDialog(string& /*fileName*/) { return true;  }

	enum Type {
		TYPE_PRIMARY,
		TYPE_SECONDARY,
		TYPE_BOTH
	};

	/* Action handlers */
	void onDownload(const string& aTarget, bool aIsWhole, bool aIsSizeUnknown, Priority p) {
		if (!aIsSizeUnknown) {
			// Get the size of the download
			auto downloadSize = getDownloadSize(aIsWhole);

			// Check the space
			auto freeSpace = File::getFreeSpace(File::getMountPath(aTarget));
			if (freeSpace < downloadSize && !confirmDownload(aTarget, freeSpace, downloadSize)) {
				return;
			}
		}

		handleDownload(aTarget, WinUtil::isShift() ? Priority::HIGHEST : p, aIsWhole);
	}

	void onDownloadTo(bool aUseWhole, bool aIsSizeUnknown) {
		string fileName;
		bool dirDlg = aUseWhole || ((T*)this)->showDirDialog(fileName);

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
		onDownload(target, aUseWhole, aIsSizeUnknown, Priority::DEFAULT);
	}


	/* Menu creation */
	void appendDownloadMenu(OMenu& aMenu, Type aType, bool isSizeUnknown, const optional<TTHValue>& aTTH, 
		const optional<string>& aVirtualPath, bool appendPrioMenu = true, bool addDefault = true) {

		auto volumes = File::getVolumes();

		// Download
		aMenu.appendItem(CTSTRING(DOWNLOAD), [=] { 
			onDownload(SETTING(DOWNLOAD_DIRECTORY), aType == TYPE_SECONDARY, isSizeUnknown, Priority::DEFAULT);
		}, addDefault ? OMenu::FLAG_DEFAULT : 0);

		// Download to
		auto targetMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_TO), true);
		appendDownloadTo(*targetMenu, aType == TYPE_SECONDARY, isSizeUnknown, aTTH, aVirtualPath, volumes);

		// Download with priority
		if (appendPrioMenu) {
			appendPriorityMenu(aMenu, aType == TYPE_SECONDARY, isSizeUnknown);
		}

		if (aType == TYPE_BOTH) {
			// Download whole directory
			aMenu.appendItem(CTSTRING(DOWNLOAD_WHOLE_DIR), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), true, true, Priority::DEFAULT); });
			auto targetMenuWhole = aMenu.createSubMenu(TSTRING(DOWNLOAD_WHOLE_DIR_TO), true);

			// If we have a dupe path, pick the directory path from it
			optional<string> virtualFilePath;
			if (aVirtualPath && !(*aVirtualPath).empty() && (*aVirtualPath).back() != ADC_SEPARATOR) {
				virtualFilePath = Util::getAdcFilePath(*aVirtualPath);
			}

			appendDownloadTo(*targetMenuWhole, true, true, boost::none, virtualFilePath, volumes);
		}
	}

	void appendDownloadTo(OMenu& targetMenu_, bool aWholeDir, bool aIsSizeUnknown, const optional<TTHValue>& aTTH, const optional<string>& aVirtualPath, const File::VolumeSet& aVolumes) {
		targetMenu_.appendItem(CTSTRING(BROWSE), [=] { onDownloadTo(aWholeDir, aIsSizeUnknown); });

		//Append shared and favorite directories
		if (SETTING(SHOW_SHARED_DIRS_DL)) {
			appendVirtualItems(targetMenu_, aWholeDir, ShareManager::getInstance()->getGroupedDirectories(), TSTRING(SHARED), aIsSizeUnknown, aVolumes);
		}
		appendVirtualItems(targetMenu_, aWholeDir, FavoriteManager::getInstance()->getGroupedFavoriteDirs(), TSTRING(SETTINGS_FAVORITE_DIRS_PAGE), aIsSizeUnknown, aVolumes);

		appendTargets(targetMenu_, aWholeDir, aIsSizeUnknown, aTTH, aVirtualPath);

		//Append dir history
		const auto& historyPaths = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DOWNLOAD_DIR);
		if(!historyPaths.empty()) {
			targetMenu_.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
			for (const auto& path: historyPaths) {
				targetMenu_.appendItem(Text::toT(path).c_str(), [=] { onDownload(path, aWholeDir, aIsSizeUnknown, Priority::DEFAULT); });
			}

			targetMenu_.appendSeparator();
			targetMenu_.appendItem(TSTRING(CLEAR_HISTORY), [] { SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_DOWNLOAD_DIR); });
		}
	}

private:
	void appendPriorityMenu(OMenu& aMenu, bool aWholeDir, bool aIsSizeUnknown) {
		auto priorityMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_WITH_PRIORITY));

		auto addItem = [&](const tstring& aTitle, Priority p) -> void {
			priorityMenu->appendItem(aTitle.c_str(), [=] { onDownload(SETTING(DOWNLOAD_DIRECTORY), aWholeDir, aIsSizeUnknown, p); });
		};

		addItem(CTSTRING(PAUSED_FORCED), Priority::PAUSED_FORCE);
		addItem(CTSTRING(PAUSED), Priority::PAUSED);
		addItem(CTSTRING(LOWEST), Priority::LOWEST);
		addItem(CTSTRING(LOW), Priority::LOW);
		addItem(CTSTRING(NORMAL), Priority::NORMAL);
		addItem(CTSTRING(HIGH), Priority::HIGH);
		addItem(CTSTRING(HIGHEST), Priority::HIGHEST);
	}

	static tstring toDisplayTarget(const string& aTarget, const File::VolumeSet& aVolumes) {
		auto diskInfo = File::getDiskInfo(aTarget, aVolumes, true);

		auto ret = Text::toT(aTarget);
		if (diskInfo.freeSpace >= 0) {
			return TSTRING_F(X_BYTES_FREE, ret % Util::formatBytesW(diskInfo.freeSpace));
		}

		return ret;
	}

	void appendVirtualItems(OMenu &targetMenu, bool aWholeDir, const GroupedDirectoryMap& aDirectories, const tstring& aTitle, bool aIsSizeUnknown, const File::VolumeSet& aVolumes) {
		if (aDirectories.empty()) {
			return;
		}

		targetMenu.InsertSeparatorLast(aTitle);
		for(const auto& dp: aDirectories) {
			const auto& groupName = dp.first;
			const auto& targets = dp.second;

			if (targets.size() > 1) {
				auto vMenu = targetMenu.createSubMenu(Text::toT(groupName).c_str(), true);
				for (const auto& target: targets) {
					vMenu->appendItem(toDisplayTarget(target, aVolumes).c_str(), [=] { onDownload(target, aWholeDir, aIsSizeUnknown, Priority::DEFAULT); });
				}
			} else if (!targets.empty()) {
				auto target = *targets.begin();
				targetMenu.appendItem(Text::toT(groupName).c_str(), [=] { onDownload(target, aWholeDir, aIsSizeUnknown, Priority::DEFAULT); });
			}
		}
	}

	void appendTargets(OMenu& targetMenu, bool wholeDir, bool isSizeUnknown, const optional<TTHValue>& aTTH, const optional<string>& aVirtualPath) {


		// Append TTH locations
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

		// Append directory dupe paths
		if (aVirtualPath) {
			bool isDir = !(*aVirtualPath).empty() && (*aVirtualPath).back() == ADC_SEPARATOR;

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

			targets = QueueManager::getInstance()->getAdcDirectoryPaths(isDir ? *aVirtualPath : Util::getAdcFilePath(*aVirtualPath));
			doAppend(TSTRING(QUEUED_DUPE_PATHS));

			targets = ShareManager::getInstance()->getAdcDirectoryPaths(isDir ? *aVirtualPath : Util::getAdcFilePath(*aVirtualPath));
			doAppend(TSTRING(SHARED_DUPE_PATHS));
		}
	}

	bool confirmDownload(const string& aTarget, int64_t aFreeSpace, int64_t aDownloadSize) {
		if (!SETTING(FREE_SPACE_WARN)) {
			return true;
		}

		auto msg = TSTRING_F(CONFIRM_SIZE_WARNING,
			Util::formatBytesW(aFreeSpace) %
			Text::toT(aTarget) %
			Util::formatBytesW(aDownloadSize));

		return WinUtil::showQuestionBox(msg, MB_ICONQUESTION);
	}
};

#endif // !defined(DOWNLOADBASEHANDLER_H)