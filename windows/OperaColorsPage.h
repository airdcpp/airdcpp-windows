/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#ifndef OperaColorsPage_H
#define OperaColorsPage_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StdAfx.h"
#include "PropPage.h"
#include "PropPageTextStyles.h"

class OperaColorsPage : public CPropertyPage<IDD_OPERACOLORS>, public PropPage
{
public:
	OperaColorsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES) + _T('\\') + TSTRING(SETTINGS_OPERACOLORS)).c_str());
		SetTitle(title);
		hloubka = SETTING(PROGRESS_3DDEPTH);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~OperaColorsPage() { free(title);};

	BEGIN_MSG_MAP(OperaColorsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_FLAT, EN_UPDATE, On3DDepth)
		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE2, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_SEGMENT_SHOW, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_BUMPED, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_SETTINGS_DOWNLOAD_BAR_COLOR, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_SETTINGS_UPLOAD_BAR_COLOR, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_SETTINGS_SEGMENT_BAR_COLOR, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_COLOR_DOWN, BN_CLICKED, onClickedProgressTextDown)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_COLOR_UP, BN_CLICKED, onClickedProgressTextUp)
		COMMAND_HANDLER(IDC_ODC_STYLE, BN_CLICKED, onClickedProgress)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)

		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_LEFT, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_RIGHT, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_USETWO, BN_CLICKED, onMenubarClicked)
		COMMAND_HANDLER(IDC_SETTINGS_ODC_MENUBAR_BUMPED, BN_CLICKED, onMenubarClicked)
		COMMAND_ID_HANDLER(IDC_TB_PROG_STYLE, onTBProgress)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onDrawItem(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onMenubarClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedProgressOverride(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		updateProgress();
		return S_OK;
	}
	LRESULT onClickedProgress(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedProgressTextDown(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedProgressTextUp(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);

	LRESULT onTBProgress(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		EditTextStyle();
		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT On3DDepth(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		TCHAR buf[256];
		GetDlgItemText(IDC_FLAT, buf, 255);
		hloubka = Util::toInt(Text::fromT(buf));
		updateProgress();
		return 0;
	}

	void updateProgress() {
		odcStyle = (IsDlgButtonChecked(IDC_ODC_STYLE) != 0);

		bool state = (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_OVERRIDE2), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_SETTINGS_DOWNLOAD_BAR_COLOR), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_SETTINGS_UPLOAD_BAR_COLOR), state);

		::EnableWindow(::GetDlgItem(m_hWnd, IDC_FLAT),  IsDlgButtonChecked(IDC_ODC_STYLE) != 1);
		state = ((IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0) && (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE2) != 0));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_COLOR_DOWN), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_COLOR_UP), state);
		if(ctrlProgressDownDrawer.m_hWnd > 0)
			ctrlProgressDownDrawer.Invalidate();
		if(ctrlProgressUpDrawer.m_hWnd > 0)
			ctrlProgressUpDrawer.Invalidate();
	}

	void updateScreen() {
		PostMessage(WM_INITDIALOG,0,0);
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

private:
	friend UINT_PTR CALLBACK MenuBarCommDlgProc(HWND, UINT, WPARAM, LPARAM);
	friend LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	bool bDoProgress;
	bool bDoLeft;
	bool bDoSegment;
	bool odcStyle;
	
	static Item items[];
	static TextItem texts[];

	typedef CButton CCheckBox;

	COLORREF crProgressDown;
	COLORREF crProgressUp;
	COLORREF crProgressTextDown;
	COLORREF crProgressTextUp;
	COLORREF crProgressSegment;
	CCheckBox ctrlProgressOverride1;
	CCheckBox ctrlProgressOverride2;
	CButton ctrlProgressDownDrawer;
	CButton ctrlProgressUpDrawer;

	void EditTextStyle();
	LOGFONT currentFont;
	COLORREF textclr; 

	void checkBox(int id, bool b) {
		CheckDlgButton(id, b ? BST_CHECKED : BST_UNCHECKED);
	}
	bool getCheckbox(int id) {
		return (BST_CHECKED == IsDlgButtonChecked(id));
	}

	void BrowseForPic(int DLGITEM);

	COLORREF crMenubarLeft;
	COLORREF crMenubarRight;
	CButton ctrlLeftColor;
	CButton ctrlRightColor;
	CCheckBox ctrlTwoColors;
	CCheckBox ctrlBumped;
	CStatic ctrlMenubarDrawer;

	int hloubka;
	TCHAR* title;
};

#endif //OperaColorsPage_H

/**
 * @file
 * $Id: OperaColorsPage.h 321 2007-08-10 19:48:22Z bigmuscle $
 */

