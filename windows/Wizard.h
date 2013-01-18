/*
 * Copyright (C) 2012-2013 AirDC++ Project
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

#pragma once
#include "stdafx.h"

// Wizard dialog

class WizardDlg : public CDialogImpl<WizardDlg>
{


public:
	WizardDlg(){};   // standard constructor
	~WizardDlg(){};
	
	BEGIN_MSG_MAP(WizardDlg)
	COMMAND_HANDLER(IDC_DOWN_SPEED, CBN_SELENDOK , OnDownSpeed)
	COMMAND_HANDLER(IDC_CONNECTION, CBN_SELENDOK , OnUploadSpeed)
	COMMAND_HANDLER(IDC_DOWN_SPEED, CBN_EDITCHANGE  , OnDownSpeed)
	COMMAND_HANDLER(IDC_CONNECTION, CBN_EDITCHANGE  , OnUploadSpeed)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDNEXT, onNext)
	COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	COMMAND_ID_HANDLER(IDC_PUBLIC, OnDlgButton)
	COMMAND_ID_HANDLER(IDC_RAR, OnDlgButton)
	COMMAND_ID_HANDLER(IDC_PRIVATE_HUB, OnDlgButton)
	COMMAND_ID_HANDLER(IDC_DL_AUTODETECT_WIZ, OnDetect)
	COMMAND_ID_HANDLER(IDC_UL_AUTODETECT_WIZ, OnDetect)
	COMMAND_ID_HANDLER(IDC_SPEEDTEST, onSpeedtest)
	MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)	
	END_MSG_MAP()


// Dialog Data
	enum { IDD = IDD_WIZARD };

	CEdit nickline;
	CEdit explain;
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}

	LRESULT onNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDlgButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDownSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnUploadSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeedChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeedtest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT, WPARAM, LPARAM, BOOL&);

	void write();
	void setDownloadLimits(double value);
	void setUploadLimits(double value);

private:
	CComboBox ctrlUpload;	
	CComboBox ctrlDownload;	
	CComboBoxEx ctrlLanguage;	
	void fixcontrols();
	string upload;
	string download;
};
