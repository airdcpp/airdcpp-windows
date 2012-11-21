/*
 * Copyright (C) 2011-2012 AirDC++ Project
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

#ifndef SEARCH_AUTOSEARCH_PARAMS_H
#define SEARCH_AUTOSEARCH_PARAMS_H

#include <atlcrack.h>
//#include <atlctrls.h>
#include <atlctrlx.h>
#include "AutosearchDlg.h"

class AutoSearchParams : public CDialogImpl<AutoSearchParams>
{
public:
	AutoSearchDlg* p;
	enum { IDD = IDD_AUTOSEARCH_PARAMS };

	CEdit ctrlMaxNumber, ctrlCurNumber, ctrlNumberLen;
	CHyperLink url;
	
	BEGIN_MSG_MAP(AutoSearchParams)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_INSERT_NUMBER, OnInsertNumber)
	END_MSG_MAP()
	
	AutoSearchParams(AutoSearchDlg* asd) : p(asd) { }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		//ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SetWindowText(_T("Params"));
		
		//params
		ATTACH(IDC_CUR_NUMBER, ctrlCurNumber);
		ATTACH(IDC_MAX_NUMBER, ctrlMaxNumber);
		ATTACH(IDC_NUMBER_LEN, ctrlNumberLen);

		CUpDownCtrl updown;
		updown.Attach(GetDlgItem(IDC_CUR_NUMBER_SPIN));
		updown.SetRange32(0, 999);
		updown.Detach();

		updown.Attach(GetDlgItem(IDC_MAX_NUMBER_SPIN));
		updown.SetRange32(0, 999);
		updown.Detach();

		updown.Attach(GetDlgItem(IDC_NUMBER_LEN_SPIN));
		updown.SetRange32(0, 999);
		updown.Detach();

		CheckDlgButton(IDC_USE_PARAMS, p->useParams);
		ctrlCurNumber.SetWindowText(Util::toStringW(p->curNumber).c_str());
		ctrlMaxNumber.SetWindowText(Util::toStringW(p->maxNumber).c_str());
		ctrlNumberLen.SetWindowText(Util::toStringW(p->numberLen).c_str());

		url.SubclassWindow(GetDlgItem(IDC_TIMEVAR_LINK));
		url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

		url.SetHyperLink(_T("http://www.cplusplus.com/reference/clibrary/ctime/strftime/"));
		url.SetLabel(_T("Available time variables"));

		CenterWindow(GetParent());
		return FALSE;
	}

	LRESULT OnInsertNumber(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		p->insertNumber();
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			//line.resize(ctrlLine.GetWindowTextLength() + 1);
			//line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));

			//params
			TCHAR buf[512];
			p->useParams = IsDlgButtonChecked(IDC_USE_PARAMS) ? true : false;
			ctrlCurNumber.GetWindowText(buf, 512);
			p->curNumber = Util::toInt(Text::fromT(buf));
			ctrlMaxNumber.GetWindowText(buf, 512);
			p->maxNumber = Util::toInt(Text::fromT(buf));
			ctrlNumberLen.GetWindowText(buf, 512);
			p->numberLen = Util::toInt(Text::fromT(buf));
		}
		EndDialog(wID);
		return 0;
	}
	
};

#endif