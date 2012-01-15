
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"
#include "FavoriteDirDlg.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

FavoriteDirDlg::FavoriteDirDlg() {
	name = _T("");
	path = _T("");
}

FavoriteDirDlg::~FavoriteDirDlg() {
	ctrlName.Detach();
	ctrlPath.Detach();
}

LRESULT FavoriteDirDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_FDNAME, ctrlName);
	ATTACH(IDC_FDPATH, ctrlPath);
	ctrlName.SetWindowText(name.c_str());
	ctrlPath.SetWindowText(path.c_str());

	::SetWindowText(GetDlgItem(IDOK), (TSTRING(OK)).c_str());
	::SetWindowText(GetDlgItem(IDCANCEL), (TSTRING(CANCEL)).c_str());
	::SetWindowText(GetDlgItem(IDC_FAVDIR_EXPLAIN), (TSTRING(FAVDIR_EXPLAIN)).c_str());

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT FavoriteDirDlg::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[2048];

	GetDlgItemText(IDC_FDPATH, buf, MAX_PATH);
	tstring oldstr = buf;
	tstring addstr;

	if(WinUtil::browseDirectory(addstr, m_hWnd) == IDOK) {
		tstring newstr;
		if(!oldstr.empty()){
			if(oldstr[oldstr.length() -1] != _T('|'))
				oldstr += _T("|");

			newstr = oldstr + addstr;
		} else
			newstr = addstr;

		SetDlgItemText(IDC_FDPATH, newstr.c_str());
	}
	return 0;
}

LRESULT FavoriteDirDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		TCHAR buf[MAX_PATH];
		tstring tmp;
		if (ctrlName.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}
		GetDlgItemText(IDC_FDNAME, buf, MAX_PATH);
		name = buf;

		//resize tstring to it, avoid possible buffer overflow.
		tmp.resize(ctrlPath.GetWindowTextLength() + 1);
		tmp.resize(GetDlgItemText(IDC_FDPATH, &tmp[0], tmp.size()));
		path = tmp;
		if(path.empty()) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}
	}
	EndDialog(wID);
	return 0;
}


