/*
 * Copyright (C) 2011-2016 AirDC++ Project
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

#include "TabbedDialog.h"

class AutoSearchParams : public CDialogImpl<AutoSearchParams>
{
public:
	AutoSearchGeneralPage* p;
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
	
	AutoSearchParams(AutoSearchGeneralPage* asd) : p(asd) { }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		SetWindowText(_T("Params"));
		
		//params
		ATTACH(IDC_CUR_NUMBER, ctrlCurNumber);
		ATTACH(IDC_MAX_NUMBER, ctrlMaxNumber);
		ATTACH(IDC_NUMBER_LEN, ctrlNumberLen);
		
		SetDlgItemText(IDC_INCR_PARAMETERS, CTSTRING(INCREMENTING_NUMBERS));
		SetDlgItemText(IDC_AS_CURNUMBER_HELP2, CTSTRING(NEXT_NUMBER_TO_SEARCH_FOR));
		SetDlgItemText(IDC_INCR_NUM_DESC, CTSTRING(AS_INC_DESC));
		SetDlgItemText(IDC_CUR_NUMBER_LBL, CTSTRING(CURRENT_NUMBER));
		SetDlgItemText(IDC_MAX_NUMBER_LBL, CTSTRING(LAST_NUMBER_TO_SEARCH));
		SetDlgItemText(IDC_AS_MAXNUMBER_HELP, CTSTRING(AS_MAX_NUMBER_HELP));
		SetDlgItemText(IDC_NUMBER_LEN_LBL, CTSTRING(MINIMUM_LEN));
		SetDlgItemText(IDC_AS_NUMLEN_HELP, CTSTRING(AS_NUM_LEN_HELP));
		SetDlgItemText(IDC_INSERT_NUMBER, CTSTRING(INSERT_IN_SEARCHSTRING));
		SetDlgItemText(IDC_AS_TIMEVAR_HELP, CTSTRING(AS_TIMEVAR_HELP));
		SetDlgItemText(IDC_TIMEVAR_LINK, CTSTRING(AS_TIMEVAR_AVAILABLE));

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

		ctrlCurNumber.SetWindowText(Util::toStringW(p->options.curNumber).c_str());
		ctrlMaxNumber.SetWindowText(Util::toStringW(p->options.maxNumber).c_str());
		ctrlNumberLen.SetWindowText(Util::toStringW(p->options.numberLen).c_str());

		url.SubclassWindow(GetDlgItem(IDC_TIMEVAR_LINK));
		url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

		url.SetHyperLink(_T("http://www.cplusplus.com/reference/clibrary/ctime/strftime/"));
		url.SetLabel(CTSTRING(AS_TIMEVAR_AVAILABLE));

		CenterWindow(GetParent());
		return FALSE;
	}

	LRESULT OnInsertNumber(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		p->insertNumber();
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			//params
			TCHAR buf[512];
			ctrlCurNumber.GetWindowText(buf, 512);
			auto curNumber = Util::toInt(Text::fromT(buf));
			ctrlMaxNumber.GetWindowText(buf, 512);
			auto maxNumber = Util::toInt(Text::fromT(buf));

			if (maxNumber > 0 && curNumber > maxNumber) {
				MessageBox(CTSTRING(AS_OVER_MAX_NUMBER));
				return 0;
			}

			p->options.curNumber = curNumber;
			p->options.maxNumber = maxNumber;

			ctrlNumberLen.GetWindowText(buf, 512);
			p->options.numberLen = Util::toInt(Text::fromT(buf));
		}
		EndDialog(wID);
		return 0;
	}
	
};

#endif