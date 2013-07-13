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

#ifndef SEARCH_FAKEPAGE_DLG_H
#define SEARCH_FAKEPAGE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>
#include <bitset>
#include "../client/Util.h"
#include "../client/TargetUtil.h"
#include "../client/AutoSearchManager.h"
#include "DownloadBaseHandler.h"
#include "SearchTypeCombo.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

class AutoSearchDlg : public CDialogImpl<AutoSearchDlg>, public DownloadBaseHandler<AutoSearchDlg> {
public:
	string searchString, comment, target, userMatch, matcherString, fileTypeStr, excludedWords;
	uint8_t matcherType, action;
	TargetUtil::TargetType targetType;
	SearchTime startTime;
	SearchTime endTime;

	time_t expireTime;
	bitset<7> searchDays;

	bool display;
	bool remove;
	bool checkQueued;
	bool checkShared;
	bool matchFullPath;

	int numberLen, curNumber, maxNumber;
	bool useParams;

	enum { IDD = IDD_AUTOSEARCH_DLG };

	AutoSearchDlg();
	~AutoSearchDlg();

	BEGIN_MSG_MAP_EX(AutoSearchDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_DRAWITEM, SearchTypeCombo::onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, SearchTypeCombo::onMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_AS_ACTION, onAction)
		COMMAND_ID_HANDLER(IDC_AS_FILETYPE, onTypeChanged)
		COMMAND_ID_HANDLER(IDC_USE_MATCHER, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_CUSTOM_SEARCH_TIMES, onCheckTimes)
		COMMAND_ID_HANDLER(IDC_USE_EXPIRY, onCheckExpiry)
		COMMAND_ID_HANDLER(IDC_ADVANCED, onClickAdvanced)
		COMMAND_ID_HANDLER(IDC_USE_PARAMS, onCheckParams)
		COMMAND_ID_HANDLER(IDC_CONF_PARAMS, onConfigParams)
		COMMAND_ID_HANDLER(IDC_EXACT_MATCH, onExactMatch)
		COMMAND_HANDLER(IDC_TARGET_PATH, EN_CHANGE, onTargetChanged)
		COMMAND_HANDLER(IDC_SELECT_DIR, BN_CLICKED, onClickLocation)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)

		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
	END_MSG_MAP()


	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlSearch.SetFocus();
		return FALSE;
	}

	LRESULT onConfigParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickAdvanced(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onTargetChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	/* DownloadBaseHandler */
	void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool isWhole, TargetUtil::TargetType aTargetType, bool isSizeUnknown);

	void switchMode();
	void insertNumber();
private:
//	enum { BUF_LEN = 1024 };
	CImageList ftImage;

	CEdit ctrlSearch, ctrlCheatingDescription, ctrlTarget, ctrlUserMatch, ctrlMatcherString;
	SearchTypeCombo ctrlFileType;
	CComboBox cAction;
	CButton cSelectDir;
	CButton cAdvanced;
	CComboBox cMatcherType;
	CComboBox cExcludedWords;
	CStatic cTargetType;

	CDateTimePickerCtrl ctrlExpire, ctrlSearchStart, ctrlSearchEnd;

	void fixControls();
	void updateTargetTypeText();
	bool loading;
	bool advanced;

	int searchType;
};
#endif
