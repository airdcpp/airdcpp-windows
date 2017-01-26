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

#ifndef AS_SEARCH_GENERALPAGE_H
#define AS_SEARCH_GENERALPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>
#include <airdcpp/Util.h>
#include <airdcpp/modules/AutoSearchManager.h>
#include "AutoSearchItemSettings.h"
#include "DownloadBaseHandler.h"
#include "SearchTypeCombo.h"
#include "TabbedDialog.h"


class AutoSearchGeneralPage : public CDialogImpl<AutoSearchGeneralPage>, public DownloadBaseHandler<AutoSearchGeneralPage>, public TabPage {
public:

	AutoSearchItemSettings& options;

	enum { IDD = IDD_AS_GENERAL };

	AutoSearchGeneralPage(AutoSearchItemSettings& aSettings, const string& aName);
	~AutoSearchGeneralPage();

	BEGIN_MSG_MAP_EX(AutoSearchGeneralPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_DRAWITEM, SearchTypeCombo::onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, SearchTypeCombo::onMeasureItem)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
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
	
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		if (uMsg != WM_CTLCOLORDLG) {
			SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)GetStockObject(COLOR_3DFACE);
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
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
	void handleDownload(const string& aTarget, Priority p, bool isWhole);
	void insertNumber();
	
	bool write();
	string getName() { return name; }
	void moveWindow(CRect& rc) { this->MoveWindow(rc); }
	void create(HWND aParent) { this->Create(aParent); }
	void showWindow(BOOL aShow) { this->ShowWindow(aShow); }


private:
	//	enum { BUF_LEN = 1024 };
	CImageListManaged ftImage;

	CEdit ctrlSearch, ctrlCheatingDescription, ctrlTarget;
	SearchTypeCombo ctrlFileType;
	CComboBox cAction, cGroups;
	CButton cSelectDir;
	CStatic cTargetType;

	CDateTimePickerCtrl ctrlExpire;

	string name;
	void fixControls();
	void updateTargetTypeText();
	bool loading;

};
#endif
#pragma once
