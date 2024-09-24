/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/StdAfx.h>
#include <windows/PropPage.h>
#include <windows/PropPageTextStyles.h>
#include <windows/ExListViewCtrl.h>

namespace wingui {
class OperaColorsPage : public CPropertyPage<IDD_OPERACOLORS>, public PropPage, private SettingsManagerListener
{
public:
	OperaColorsPage(SettingsManager *s) : PropPage(s), loading(true) {
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES) + _T('\\') + TSTRING(SETTINGS_OPERACOLORS)).c_str());
		SetTitle(title);
		hloubka = SETTING(PROGRESS_3DDEPTH);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~OperaColorsPage() { free(title);};

	BEGIN_MSG_MAP(OperaColorsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		NOTIFY_HANDLER(IDC_COLOR_LIST, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_COLOR_LIST, LVN_ITEMCHANGED, onItemchanged)
		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_OVERRIDE2, BN_CLICKED, onClickedProgressOverride)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_DOWNLOAD, BN_CLICKED, onClickedProgressText)
		COMMAND_HANDLER(IDC_PROGRESS_TEXT_UPLOAD, BN_CLICKED, onClickedProgressText)
		COMMAND_HANDLER(IDC_ODC_STYLE, BN_CLICKED, onClickedProgress)
		COMMAND_HANDLER(IDC_PROGRESS_SELECT_COLOR, BN_CLICKED, onSelectColor)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)
		MESSAGE_HANDLER(WM_HSCROLL, onSlider)

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
	LRESULT onClickedProgressText(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);

	LRESULT onSlider(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if (reinterpret_cast<HWND>(lParam) == dimmer.m_hWnd) {
			if (LOWORD(wParam) == SB_ENDSCROLL) {
				if (odcStyle)
					BarBlend = dimmer.GetPos();
				else
					BarDepth = dimmer.GetPos();
				ctrlList.Invalidate();
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onItemchanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateProgress();
		return 0;
	}
	LRESULT onSelectColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onTBProgress(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		EditTextStyle();
		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);


	void updateProgress() {
		odcStyle = (IsDlgButtonChecked(IDC_ODC_STYLE) != 0);
		bool itemSelected = ctrlList.GetSelectedCount() > 0;
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_SELECT_COLOR), itemSelected);

		bool state = (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_OVERRIDE2), state);
		if (odcStyle) {
			dimmer.SetRange(0, 100);
			ctrlDimmerTxt.SetWindowText(CTSTRING(BLEND_COLOR));
			dimmer.SetPos(BarBlend);
		} else {
			dimmer.SetRange(1, 5);
			dimmer.SetPos(BarDepth);
			ctrlDimmerTxt.SetWindowText(CTSTRING(BAR_DEPTH));
		}
		state = ((IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE) != 0) && (IsDlgButtonChecked(IDC_PROGRESS_OVERRIDE2) != 0));
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_DOWNLOAD), state);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PROGRESS_TEXT_UPLOAD), state);
		ctrlList.Invalidate();
	}

	void updateScreen() {
		PostMessage(WM_INITDIALOG,0,0);
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

private:
	struct clrs{
		ResourceManager::Strings name;
		int setting;
		COLORREF value;
	};


	friend UINT_PTR CALLBACK MenuBarCommDlgProc(HWND, UINT, WPARAM, LPARAM);
	friend LRESULT PropPageTextStyles::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	bool bDoProgress;
	bool bDoLeft;
	bool bDoSegment;
	bool odcStyle;
	bool loading;

	static Item items[];
	static TextItem texts[];
	static clrs colours[];
	static clrs ODCcolours[];

	typedef CButton CCheckBox;

	COLORREF crProgressTextDown;
	COLORREF crProgressTextUp;
	CCheckBox ctrlProgressOverride1;
	CCheckBox ctrlProgressOverride2;
	CProgressBarCtrl progress;
	tstring sampleText;

	int BarDepth;
	int BarBlend;

	CTrackBarCtrl dimmer;
	CStatic ctrlDimmerTxt;

	ExListViewCtrl ctrlList;

	void EditTextStyle();
	LOGFONT currentFont;
	COLORREF textclr; 

	void checkBox(int id, bool b) {
		CheckDlgButton(id, b ? BST_CHECKED : BST_UNCHECKED);
	}
	bool getCheckbox(int id) {
		return (BST_CHECKED == IsDlgButtonChecked(id));
	}

	void setProgressText(const tstring& text);
	void fillColorList();

	COLORREF crMenubarLeft;
	COLORREF crMenubarRight;
	CButton ctrlLeftColor;
	CButton ctrlRightColor;
	CCheckBox ctrlTwoColors;
	CCheckBox ctrlBumped;
	CStatic ctrlMenubarDrawer;

	int hloubka;
	TCHAR* title;

	virtual void on(SettingsManagerListener::ReloadPages, int) noexcept {
		SendMessage(WM_DESTROY,0,0);
		SendMessage(WM_INITDIALOG,0,0);
	}
};
}

#endif //OperaColorsPage_H

