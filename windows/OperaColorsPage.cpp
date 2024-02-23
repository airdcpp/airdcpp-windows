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

#include "stdafx.h"

#include <airdcpp/SettingsManager.h>

#include "Resource.h"
#include "OperaColorsPage.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"
#include "BarShader.h"

PropPage::TextItem OperaColorsPage::texts[] = {
	{ IDC_ODC_STYLE, ResourceManager::PROGRESS_ODC_STYLE },
	{ IDC_PROGRESS_OVERRIDE, ResourceManager::SETTINGS_ZDC_PROGRESS_OVERRIDE },
	{ IDC_PROGRESS_OVERRIDE2, ResourceManager::SETTINGS_ZDC_PROGRESS_OVERRIDE },
	{ IDC_SETTINGS_ODC_MENUBAR, ResourceManager::MENUBAR },
	{ IDC_SETTINGS_ODC_MENUBAR_LEFT, ResourceManager::LEFT_COLOR },
	{ IDC_SETTINGS_ODC_MENUBAR_RIGHT, ResourceManager::RIGHT_COLOR },
	{ IDC_SETTINGS_ODC_MENUBAR_USETWO, ResourceManager::TWO_COLORS },
	{ IDC_SETTINGS_ODC_MENUBAR_BUMPED, ResourceManager::BUMPED },
	{ IDC_SETTINGS_ODC_MENUBAR2, ResourceManager::SETCZDC_PROGRESSBAR_COLORS },
	{ IDC_TB_PROG_STYLE, ResourceManager::COLOR_FONT },
	{ IDC_TB_PROGRESS_STYLE, ResourceManager::TOOLBAR_PROGRESS_STYLE },
	{ IDC_PROGRESS_SELECT_COLOR, ResourceManager::SETTINGS_SELECT_COLOR },
	{ IDC_PROGRESS_TEXT_DOWNLOAD, ResourceManager::DOWNLOAD },
	{ IDC_PROGRESS_TEXT_UPLOAD, ResourceManager::SETCZDC_UPLOAD },

	{ 0, ResourceManager::LAST }
};

#define CTLID_VALUE_RED 0x2C2
#define CTLID_VALUE_BLUE 0x2C3
#define CTLID_VALUE_GREEN 0x2C4

OperaColorsPage* current_page;
LPCCHOOKPROC color_proc;

PropPage::Item OperaColorsPage::items[] = {
	{ IDC_ODC_STYLE, SettingsManager::PROGRESSBAR_ODC_STYLE, PropPage::T_BOOL },
	{ IDC_PROGRESS_OVERRIDE, SettingsManager::PROGRESS_OVERRIDE_COLORS, PropPage::T_BOOL },
	{ IDC_PROGRESS_OVERRIDE2, SettingsManager::PROGRESS_OVERRIDE_COLORS2, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

OperaColorsPage::clrs OperaColorsPage::colours[] = {
	{ ResourceManager::DOWNLOAD, SettingsManager::DOWNLOAD_BAR_COLOR, 0 },
	{ ResourceManager::SETCZDC_UPLOAD, SettingsManager::UPLOAD_BAR_COLOR, 0 },
	{ ResourceManager::BACKGROUND, SettingsManager::PROGRESS_BACK_COLOR, 0 },
	{ ResourceManager::DONE_CHUNKS, SettingsManager::COLOR_DONE, 0 },
	{ ResourceManager::FINISHED, SettingsManager::COLOR_STATUS_FINISHED, 0 },
	{ ResourceManager::SHARED, SettingsManager::COLOR_STATUS_SHARED, 0 },
};

OperaColorsPage::clrs OperaColorsPage::ODCcolours[] = {
	{ ResourceManager::DOWNLOAD, SettingsManager::DOWNLOAD_BAR_COLOR, 0 },
	{ ResourceManager::SETCZDC_UPLOAD, SettingsManager::UPLOAD_BAR_COLOR, 0 },
	{ ResourceManager::FILE_SEGMENT, SettingsManager::PROGRESS_SEGMENT_COLOR, 0 },
	{ ResourceManager::FINISHED, SettingsManager::COLOR_STATUS_FINISHED, 0 },
	{ ResourceManager::SHARED, SettingsManager::COLOR_STATUS_SHARED, 0 },
};

UINT_PTR CALLBACK MenuBarCommDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (uMsg == WM_COMMAND) {
		if (HIWORD(wParam) == EN_CHANGE && 
			(LOWORD(wParam) == CTLID_VALUE_RED || LOWORD(wParam) == CTLID_VALUE_GREEN || LOWORD(wParam) == CTLID_VALUE_BLUE)) {
			TCHAR buf[16];
			memzero(buf, sizeof(buf));
			::GetDlgItemText(hWnd, CTLID_VALUE_RED, buf, 15);
			int color_r = Util::toInt(Text::fromT(buf));
			memzero(buf, sizeof(buf));
			::GetDlgItemText(hWnd, CTLID_VALUE_BLUE, buf, 15);
			int color_g = Util::toInt(Text::fromT(buf));
			memzero(buf, sizeof(buf));
			::GetDlgItemText(hWnd, CTLID_VALUE_GREEN, buf, 15);
			int color_b = Util::toInt(Text::fromT(buf));

			color_r = (color_r < 0) ? 0 : ((color_r > 255) ? 255 : color_r);
			color_g = (color_g < 0) ? 0 : ((color_g > 255) ? 255 : color_g);
			color_b = (color_b < 0) ? 0 : ((color_b > 255) ? 255 : color_b);

			if (current_page->bDoLeft)
				current_page->crMenubarLeft = RGB(color_r, color_g, color_b);
			else
				current_page->crMenubarRight = RGB(color_r, color_g, color_b);
			current_page->ctrlMenubarDrawer.Invalidate();
		}
	}
	
	return (*color_proc)(hWnd, uMsg, wParam, lParam);
}
LRESULT OperaColorsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	SettingsManager::getInstance()->addListener(this);

	ctrlList.Attach(GetDlgItem(IDC_COLOR_LIST));
	ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	CRect rc;
	ctrlList.GetClientRect(rc);
	ctrlList.InsertColumn(0, 0, LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlList.InsertColumn(1, 0, LVCFMT_LEFT, rc.Width() / 2, 1);

	dimmer.Attach(GetDlgItem(IDC_PROGRESS_SLIDER));
	BarBlend = SETTING(PROGRESS_LIGHTEN);
	BarDepth = SETTING(PROGRESS_3DDEPTH);
	ctrlDimmerTxt.Attach(GetDlgItem(IDC_LIGHTEN_TXT));

	sampleText = TSTRING(SAMPLE_TEXT);

	crProgressTextDown = SETTING(PROGRESS_TEXT_COLOR_DOWN);
	crProgressTextUp = SETTING(PROGRESS_TEXT_COLOR_UP);

	progress.Attach(GetDlgItem(IDC_PROGRESS1));
	progress.SetRange(0,100);
	progress.SetPos(50);
	WinUtil::decodeFont(Text::toT(SETTING(TB_PROGRESS_FONT)), currentFont );
	textclr = SETTING(TB_PROGRESS_TEXT_COLOR);

	crMenubarLeft = SETTING(MENUBAR_LEFT_COLOR);
	crMenubarRight = SETTING(MENUBAR_RIGHT_COLOR);
	ctrlLeftColor.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_LEFT));
	ctrlRightColor.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_RIGHT));
	ctrlTwoColors.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_USETWO));
	ctrlBumped.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
	ctrlMenubarDrawer.Attach(GetDlgItem(IDC_SETTINGS_ODC_MENUBAR_COLOR));

	checkBox(IDC_SETTINGS_ODC_MENUBAR_BUMPED, SETTING(MENUBAR_BUMPED));
	checkBox(IDC_SETTINGS_ODC_MENUBAR_USETWO, SETTING(MENUBAR_TWO_COLORS));

	//load defaults
	for (int i = 0; i < sizeof(colours) / sizeof(clrs); i++){
		colours[i].value = SettingsManager::getInstance()->get((SettingsManager::IntSetting)colours[i].setting, true);
	}
	for (int i = 0; i < sizeof(ODCcolours) / sizeof(clrs); i++){
		ODCcolours[i].value = SettingsManager::getInstance()->get((SettingsManager::IntSetting)ODCcolours[i].setting, true);
	}
	updateProgress();
	fillColorList();

	BOOL b;
	onMenubarClicked(0, IDC_SETTINGS_ODC_MENUBAR_USETWO, 0, b);
	// Do specialized reading here
	loading = false;
	return TRUE;
}

void OperaColorsPage::write()
{
	
	PropPage::write((HWND)*this, items);
	
	// Do specialized writing here
	// settings->set(XX, YY);
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_LEFT_COLOR, (int)crMenubarLeft);
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_RIGHT_COLOR, (int)crMenubarRight);
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_BUMPED, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
	SettingsManager::getInstance()->set(SettingsManager::MENUBAR_TWO_COLORS, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO));

	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_DOWN, (int)crProgressTextDown);
	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_TEXT_COLOR_UP, (int)crProgressTextUp);
	
	SettingsManager::getInstance()->set(SettingsManager::TB_PROGRESS_FONT, Text::fromT(WinUtil::encodeFont(currentFont)));
	SettingsManager::getInstance()->set(SettingsManager::TB_PROGRESS_TEXT_COLOR, (int)textclr);
	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_LIGHTEN, BarBlend);
	SettingsManager::getInstance()->set(SettingsManager::PROGRESS_3DDEPTH, BarDepth);

	for (int i = 0; i < sizeof(colours) / sizeof(clrs); i++){
		settings->set((SettingsManager::IntSetting)colours[i].setting, (int)colours[i].value);
	}
	for (int i = 0; i < sizeof(ODCcolours) / sizeof(clrs); i++){
		settings->set((SettingsManager::IntSetting)ODCcolours[i].setting, (int)ODCcolours[i].value);
	}


	WinUtil::progressFont = CreateFontIndirect(&currentFont);
	WinUtil::TBprogressTextColor = textclr;
	
	OperaColors::ClearCache();
}

LRESULT OperaColorsPage::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
	{
		if (cd->iSubItem != 1)
			return 0;

		CRect rc;
		ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);

		COLORREF clr = getCheckbox(IDC_PROGRESS_OVERRIDE) ? 
			(odcStyle ? ODCcolours[(int)cd->nmcd.dwItemSpec].value : colours[(int)cd->nmcd.dwItemSpec].value) : GetSysColor(COLOR_HIGHLIGHT);
	
		COLORREF color = (int)cd->nmcd.dwItemSpec != 1 ? crProgressTextDown : crProgressTextUp;

		WinUtil::drawProgressBar(cd->nmcd.hdc, rc, clr, color, colours[0].value, sampleText,
			16, 16, odcStyle, getCheckbox(IDC_PROGRESS_OVERRIDE2), BarDepth, BarBlend, DT_CENTER);

		return CDRF_SKIPDEFAULT;
	}


	default:
		return CDRF_DODEFAULT;
	}
}


LRESULT OperaColorsPage::onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {

	bHandled = FALSE;
//	if (wParam == IDC_SETTINGS_ODC_MENUBAR_COLOR) {
		DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
		if (dis->CtlType == ODT_STATIC) {
			bHandled = TRUE;
			CDC dc;
			dc.Attach(dis->hDC);
			CRect rc(dis->rcItem);
			if (dis->CtlID == IDC_SETTINGS_ODC_MENUBAR_COLOR) {
				if (getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO))
					OperaColors::FloodFill(dc, rc.left, rc.top, rc.right, rc.bottom, crMenubarLeft, crMenubarRight, getCheckbox(IDC_SETTINGS_ODC_MENUBAR_BUMPED));
				else
					dc.FillSolidRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, crMenubarLeft);
				dc.SetTextColor(OperaColors::TextFromBackground(crMenubarLeft));
			}
			dc.DrawText(sampleText.c_str(), sampleText.length(), rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

			dc.Detach();
		}
		setProgressText(sampleText);
	return S_OK;
}
void OperaColorsPage::EditTextStyle() {
	LOGFONT font;

	font = currentFont;

	CFontDialog d(&font, CF_EFFECTS | CF_SCREENFONTS, NULL, *this);
	d.m_cf.rgbColors = textclr;
	if(d.DoModal() == IDOK)
	{
		currentFont = font;
		textclr = d.GetColor();
		setProgressText(sampleText);
	}
}

void OperaColorsPage::fillColorList() {
	ctrlList.DeleteAllItems();
	if (!odcStyle){
		for (int i = 0; i < sizeof(colours) / sizeof(clrs); i++){
			ctrlList.insert(i, Text::toT(ResourceManager::getInstance()->getString(colours[i].name)));

		}
	} else {
		for (int i = 0; i < sizeof(ODCcolours) / sizeof(clrs); i++){
			ctrlList.insert(i, Text::toT(ResourceManager::getInstance()->getString(ODCcolours[i].name)));
		}
	}
}

LRESULT OperaColorsPage::onMenubarClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDC_SETTINGS_ODC_MENUBAR_LEFT) {
		CColorDialog d(crMenubarLeft, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = false;
		bDoLeft = true;
		COLORREF backup = crMenubarLeft;
		if (d.DoModal() == IDOK)
			crMenubarLeft = d.GetColor();
		else
			crMenubarLeft = backup;
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_RIGHT) {
		CColorDialog d(crMenubarRight, CC_FULLOPEN, *this);
		color_proc = d.m_cc.lpfnHook;
		d.m_cc.lpfnHook = MenuBarCommDlgProc;
		current_page = this;//d.m_cc.lCustData = (LPARAM)this;
		bDoProgress = false;
		bDoLeft = false;
		COLORREF backup = crMenubarRight;
		if (d.DoModal() == IDOK)
			crMenubarRight = d.GetColor();
		else
			crMenubarRight = backup;
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_USETWO) {
		bool b = getCheckbox(IDC_SETTINGS_ODC_MENUBAR_USETWO);
		ctrlRightColor.EnableWindow(b);
		ctrlBumped.EnableWindow(b);
	} else if (wID == IDC_SETTINGS_ODC_MENUBAR_BUMPED) {
	}
	ctrlMenubarDrawer.Invalidate();
	return S_OK;
}
LRESULT OperaColorsPage::onClickedProgress(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	odcStyle = IsDlgButtonChecked(IDC_ODC_STYLE) == 1;
	updateProgress();

	if (wID == IDC_ODC_STYLE) //reload color dropdownlist
		fillColorList();

	return TRUE;
}

LRESULT OperaColorsPage::onClickedProgressText(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	switch (wID) {
	case IDC_PROGRESS_TEXT_DOWNLOAD: {
		CColorDialog d(crProgressTextDown, 0, *this);
		if (d.DoModal() == IDOK) {
			crProgressTextDown = d.GetColor();
			ctrlList.Invalidate();
		}
		break;
	}
	case IDC_PROGRESS_TEXT_UPLOAD: {
		CColorDialog d(crProgressTextUp, 0, *this);
		if (d.DoModal() == IDOK) {
			crProgressTextUp = d.GetColor();
			ctrlList.Invalidate();
		}
		break;
	}
	default: break;
	}

	return TRUE;
}

void OperaColorsPage::setProgressText(const tstring& text){
	progress.SetRedraw(TRUE);
	progress.SetPos(50);

	CPoint ptStart;
	CRect prc;
	progress.GetClientRect(&prc);

	HDC progressTextDC = progress.GetDC();

	::SetBkMode(progressTextDC, TRANSPARENT);
	::SetTextColor(progressTextDC, textclr );
	::SelectObject(progressTextDC, CreateFontIndirect(&currentFont));
	prc.top += 5;

	::DrawText(progressTextDC, text.c_str(), text.length(), prc, DT_CENTER | DT_VCENTER );
	
	progress.ReleaseDC(progressTextDC);
	progress.SetRedraw(FALSE);
}

LRESULT OperaColorsPage::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SettingsManager::getInstance()->removeListener(this);
	if (ctrlLeftColor.m_hWnd != NULL)
		ctrlLeftColor.Detach();
	if (ctrlRightColor.m_hWnd != NULL)
		ctrlRightColor.Detach();
	if (ctrlTwoColors.m_hWnd != NULL)
		ctrlTwoColors.Detach();
	if (ctrlBumped.m_hWnd != NULL)
		ctrlBumped.Detach();
	if (ctrlMenubarDrawer.m_hWnd != NULL)
		ctrlMenubarDrawer.Detach();
	if (progress.m_hWnd != NULL)	
		progress.Detach();
	if (ctrlList.m_hWnd != NULL)
		ctrlList.Detach();
	return 1;
}

LRESULT OperaColorsPage::onSelectColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if (ctrlList.GetSelectedCount() > 0) {
		if (!odcStyle) {
			CColorDialog d(colours[ctrlList.GetSelectedIndex()].value, 0, *this);
			if (d.DoModal() == IDOK) {
				colours[ctrlList.GetSelectedIndex()].value = d.GetColor();
				ctrlList.Update(ctrlList.GetSelectedIndex());
			}
		} else {
			CColorDialog d(ODCcolours[ctrlList.GetSelectedIndex()].value, 0, *this);
			if (d.DoModal() == IDOK) {
				ODCcolours[ctrlList.GetSelectedIndex()].value = d.GetColor();
				ctrlList.Update(ctrlList.GetSelectedIndex());
			}
		}
	}
	return S_OK;
}