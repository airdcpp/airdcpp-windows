/* 
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

#ifndef SEARCH_FAKEPAGE_DLG_H
#define SEARCH_FAKEPAGE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>
#include <bitset>
#include "../client/Util.h"
#include "../client/TargetUtil.h"
#include "../client/ResourceManager.h"
#include "../client/AutoSearchManager.h"

#define FILTER_MESSAGE_MAP 8
class SearchPageDlg : public CDialogImpl<SearchPageDlg> {
public:
	string searchString, comment, target, userMatch, matcherString;
	int fileType, searchInterval;
	uint8_t matcherType, action;
	TargetUtil::TargetType targetType;
	SearchTime startTime;
	SearchTime endTime;

	time_t expireTime;
	bitset<7> searchDays;

	bool display;
	bool remove;

	enum { IDD = IDD_AUTOSEARCH_DLG };

	SearchPageDlg();
	~SearchPageDlg();

	BEGIN_MSG_MAP_EX(SearchPageDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_AS_ACTION, onAction)
		COMMAND_ID_HANDLER(IDC_AS_FILETYPE, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_USE_MATCHER, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_CUSTOM_SEARCH_TIMES, onCheckTimes)
		COMMAND_ID_HANDLER(IDC_USE_EXPIRY, onCheckExpiry)
		COMMAND_HANDLER(IDC_TARGET_PATH, EN_CHANGE, onTargetChanged)
		COMMAND_HANDLER(IDC_SELECT_DIR, BN_CLICKED, onClickLocation)
		//MESSAGE_HANDLER(WM_UNINITMENUPOPUP, onDestroyMenu)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)

		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onBrowse)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + TargetUtil::countDownloadDirItems(), onDownloadFavoriteDirs)

		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
	END_MSG_MAP()


	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlSearch.SetFocus();
		return FALSE;
	}

	LRESULT onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT onTargetChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
private:
//	enum { BUF_LEN = 1024 };
	CImageList ftImage;

	CEdit ctrlSearch, ctrlCheatingDescription, ctrlTarget, ctrlUserMatch, ctrlMatcherString;
	CComboBox cRaw;
	CComboBoxEx ctrlFileType;
	CComboBox cAction;
	CButton cSelectDir;
	CComboBox cMatcherType;

	CDateTimePickerCtrl ctrlExpire, ctrlSearchStart, ctrlSearchEnd;

	OMenu targetMenu;

	void fixControls();
	void updateTargetText();
};
#endif
