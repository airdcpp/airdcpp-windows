#ifndef PARAMS_DLG_H
#define PARAMS_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ATTACH(id, var, txt) \
		var.Attach(GetDlgItem(id)); \
		var.SetWindowText(Text::toT(txt).c_str());

#define GET_TEXT(id, var) \
			GetDlgItemText(id, buf, 1024); \
			var = Text::fromT(buf);

class ParamDlg : public CDialogImpl<ParamDlg>
{
	CEdit ctrlName;
	CEdit ctrlRegExp;

public:
	string name;
	string regexp;

	enum { IDD = IDD_PARAM_DLG };

	BEGIN_MSG_MAP(ParamDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	ParamDlg() { };

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{

		ATTACH(IDC_PARAM_NAME, ctrlName, name);
		ATTACH(IDC_PARAM_REGEXP, ctrlRegExp, regexp);

		CenterWindow(GetParent());
		return FALSE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			TCHAR buf[1024];
			if ( ctrlName.GetWindowTextLength() == 0 || ctrlRegExp.GetWindowTextLength() == 0) {
				MessageBox(_T("Name or RegExp must not be empty"));
				return 0;
			}
			GET_TEXT(IDC_PARAM_NAME, name);
			GET_TEXT(IDC_PARAM_REGEXP, regexp);
		}
		EndDialog(wID);
		return 0;
	}
};

#endif //PARAMS_DLG_H
