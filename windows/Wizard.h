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
