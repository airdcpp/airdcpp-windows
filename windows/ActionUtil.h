/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef ACTION_UTIL_H
#define ACTION_UTIL_H

#include "resource.h"

#include <airdcpp/DupeType.h>
#include <airdcpp/MerkleTree.h>
#include <airdcpp/Priority.h>
#include <airdcpp/SettingItem.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/User.h>


namespace wingui {
class OMenu;
class RichTextBox;

class ActionUtil {
public:
	static void search(const tstring& aSearch, bool aSearchDirectory = false);

	struct ConnectFav {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct GetList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct BrowseList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct GetBrowseList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct MatchQueue {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct PM {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	static void showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force = false);
	static void showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags = NIIF_INFO, bool force = false);

	static bool browseList(tstring& target_, HWND aOwner);
	static bool browseApplication(tstring& target_, HWND aOwner);

	// Hash related
	static void copyMagnet(const TTHValue& /*aHash*/, const string& /*aFile*/, int64_t);
	static void searchHash(const TTHValue& aHash, const string& aFileName, int64_t aSize);
	static string makeMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize);
	static void parseMagnetUri(const tstring& aUrl, const HintedUser& aUser, RichTextBox* aCtrlEdit = nullptr);

	static bool parseDBLClick(const tstring& aString);

	static void openLink(const tstring& aUrl);
	static void openFile(const tstring& aFileName);
	static void openFolder(const tstring& aFileName);
	static void openTextFile(const string& aFileName, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, bool aIsClientView) noexcept;

	static void appendBundlePrioMenu(OMenu& aParent, const BundleList& aBundles);
	static void appendBundlePauseMenu(OMenu& aParent, const BundleList& aBundles);
	static void appendFilePrioMenu(OMenu& aParent, const QueueItemList& aFiles);

	static bool getUCParams(HWND parent, const UserCommand& cmd, ParamMap& params_) noexcept;

	static void appendPreviewMenu(OMenu& parent_, const string& aTarget);

	static void appendLanguageMenu(CComboBoxEx& ctrlLanguage) noexcept;
	static void setLanguage(int aLanguageIndex) noexcept;

	static void appendHistory(CComboBox& ctrlExcluded, SettingsManager::HistoryType aType);
	static string addHistory(CComboBox& ctrlExcluded, SettingsManager::HistoryType aType);

	static void viewLog(const string& aPath, bool aHistory = false);

	static void removeBundle(QueueToken aBundleToken);

	static LRESULT onAddressFieldChar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	static LRESULT onUserFieldChar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	static bool onConnSpeedChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);

	static void getProfileConflicts(HWND aParent, int aProfile, ProfileSettingItem::List& conflicts_);

	static void appendSpeedCombo(CComboBox& aCombo, SettingsManager::StrSetting aSetting);
	static void appendDateUnitCombo(CComboBox& aCombo, int aSel = 1);

	static void appendSizeCombos(CComboBox& aUnitCombo, CComboBox& aModeCombo, int aUnitSel = 2, int aModeSel = 1);
	static int64_t parseSize(CEdit& aSize, CComboBox& aSizeUnit);

	static void addFileDownload(const string& aTarget, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, time_t aDate, Flags::MaskType aFlags = 0, Priority aPrio = Priority::DEFAULT);

	static void connectHub(const string& aUrl);

	static void findNfo(const string& aAdcPath, const HintedUser& aUser) noexcept;
	static bool allowGetFullList(const HintedUser& aUser) noexcept;
};
}


#endif // !defined(MENU_UTIL_H)