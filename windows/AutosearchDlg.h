/*
 * Copyright (C) 2011-2015 AirDC++ Project
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

class AutoSearchGeneralPage : public CDialogImpl<AutoSearchGeneralPage>, public DownloadBaseHandler<AutoSearchGeneralPage> {
public:

	ItemSettings& options;

	enum { IDD = IDD_AS_GENERAL };

	AutoSearchGeneralPage(ItemSettings& aSettings);
	~AutoSearchGeneralPage();

	BEGIN_MSG_MAP_EX(AutoSearchGeneralPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_DRAWITEM, SearchTypeCombo::onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, SearchTypeCombo::onMeasureItem)
		COMMAND_ID_HANDLER(IDC_AS_ACTION, onAction)
		COMMAND_ID_HANDLER(IDC_AS_FILETYPE, onTypeChanged)
		COMMAND_ID_HANDLER(IDC_USE_EXPIRY, onCheckExpiry)
		COMMAND_ID_HANDLER(IDC_USE_PARAMS, onCheckParams)
		COMMAND_ID_HANDLER(IDC_CONF_PARAMS, onConfigParams)
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
	LRESULT onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onTargetChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	/* DownloadBaseHandler */
	void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool isWhole, TargetUtil::TargetType aTargetType, bool isSizeUnknown);
	void insertNumber();
	bool write();

private:
//	enum { BUF_LEN = 1024 };
	CImageList ftImage;

	CEdit ctrlSearch, ctrlCheatingDescription, ctrlTarget;
	SearchTypeCombo ctrlFileType;
	CComboBox cAction;
	CButton cSelectDir;
	CStatic cTargetType;

	CDateTimePickerCtrl ctrlExpire;

	void fixControls();
	void updateTargetTypeText();
	bool loading;

};

class AutoSearchAdvancedPage : public CDialogImpl<AutoSearchAdvancedPage>{
public:

	ItemSettings& options;

	enum { IDD = IDD_AS_ADVANCED };

	AutoSearchAdvancedPage(ItemSettings& aSettings);
	~AutoSearchAdvancedPage();

	BEGIN_MSG_MAP_EX(AutoSearchAdvancedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_USE_MATCHER, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_CUSTOM_SEARCH_TIMES, onCheckTimes)
		COMMAND_ID_HANDLER(IDC_EXACT_MATCH, onExactMatch)
		END_MSG_MAP()


	bool write();
	LRESULT onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

private:

	CEdit  ctrlCheatingDescription, ctrlUserMatch, ctrlMatcherString;
	CComboBox cMatcherType;
	CComboBox cExcludedWords;

	CDateTimePickerCtrl ctrlSearchStart, ctrlSearchEnd;

	void fixControls();
	bool loading;

};



#endif
