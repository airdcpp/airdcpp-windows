#pragma once
#include "stdafx.h"

// Wizard dialog

class WizardDlg : public CDialogImpl<WizardDlg>
{


public:
	WizardDlg(){};   // standard constructor
	~WizardDlg(){};
	
	BEGIN_MSG_MAP(WizardDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDNEXT, onNext)
	COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	COMMAND_ID_HANDLER(IDC_PUBLIC, OnDlgButton)
	COMMAND_ID_HANDLER(IDC_RAR, OnDlgButton)
	COMMAND_ID_HANDLER(IDC_NON_SEGMENT, OnDlgButton)
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
	
		void write();
private:
		CComboBox ctrlUpload;	
		CComboBox ctrlDownload;	
		void fixcontrols();
		
protected:

	

};
