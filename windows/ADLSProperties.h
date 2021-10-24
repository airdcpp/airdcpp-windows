/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef ADLS_PROPERTIES_H
#define ADLS_PROPERTIES_H

#include <airdcpp/modules/ADLSearch.h>

///////////////////////////////////////////////////////////////////////////////
//
//	Dialog for new/edit ADL searches
//
///////////////////////////////////////////////////////////////////////////////
class ADLSProperties : public CDialogImpl<ADLSProperties>
{
public:

	// Constructor/destructor
	ADLSProperties(ADLSearch& _search) : search(_search) { }
	~ADLSProperties() { }

	// Dilaog unique id
	enum { IDD = IDD_ADLS_PROPERTIES };
	
	// Inline message map
	BEGIN_MSG_MAP(ADLSProperties)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_REGEXP_TEST, OnRegExpTester)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_REGEXP, FixControls)
		//COMMAND_ID_HANDLER(IDC_SOURCE_TYPE, FixControls)
		COMMAND_HANDLER(IDC_SOURCE_TYPE, CBN_SELENDOK, FixControls)
	END_MSG_MAP()
	
	// Message handlers
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRegExpTester(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT FixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { fixControls(); return 0; }
private:

	// Current search
	ADLSearch& search;

	CEdit ctrlSearch;
	CEdit ctrlComment;
	CEdit ctrlDestDir;
	CEdit ctrlMinSize;
	CEdit ctrlMaxSize;
	CButton ctrlActive;
	CButton ctrlAutoQueue;
	CButton ctrlRegexp;
	CComboBox ctrlSearchType;
	CComboBox ctrlSizeType;
	string matchRegExp(const string& aExp, const string& aString);
	void fixControls();
};

#endif // !defined(ADLS_PROPERTIES_H)