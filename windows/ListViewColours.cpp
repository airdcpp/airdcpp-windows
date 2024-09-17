/* 
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

#include "Resource.h"
#include "ListViewColours.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"
#include "ResourceLoader.h"

namespace wingui {
PropPage::TextItem ListViewColours::texts[] = {
	{ IDC_CHANGE_COLOR, ResourceManager::SETTINGS_CHANGE },
	{ IDC_USER_LIST_COLOR_DESC, ResourceManager::SETTINGS_USER_COLORS },
	{ IDC_LIST_VIEW_FONT, ResourceManager::SETTINGS_CHANGE },
	{ IDC_LIST_GENERAL, ResourceManager::LIST_TEXTSTYLE },
	{ 0, ResourceManager::LAST }
};


LRESULT ListViewColours::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	SettingsManager::getInstance()->addListener(this);

	::GetObject(WinUtil::listViewFont, sizeof(currentFont), &currentFont);
	fontChanged = false;

	normalColour = SETTING(NORMAL_COLOUR);
	favoriteColour = SETTING(FAVORITE_COLOR);
	reservedSlotColour = SETTING(RESERVED_SLOT_COLOR);
	ignoredColour = SETTING(IGNORED_COLOR);
	pasiveColour = SETTING(PASIVE_COLOR);
	opColour = SETTING(OP_COLOR);

	n_lsbList.Attach( GetDlgItem(IDC_USERLIST_COLORS) );
	n_Preview.Attach( GetDlgItem(IDC_PREVIEW) );
	n_Preview.SetBackgroundColor(SETTING(BACKGROUND_COLOR));
	n_lsbList.AddString(CTSTRING(NORMAL));
	n_lsbList.AddString(CTSTRING(FAVORITE));	
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_RESERVED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_IGNORED));
	n_lsbList.AddString(CTSTRING(COLOR_OP));
	n_lsbList.AddString(CTSTRING(PASSIVE_USER));
	n_lsbList.SetCurSel( 0 );

	refreshPreview();
	return TRUE;
}
LRESULT ListViewColours::onChangeColour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
	int index = n_lsbList.GetCurSel();
	if(index == -1) return 0;
	int colour = 0;
	switch(index) {
		case 0: colour = normalColour; break;
		case 1: colour = favoriteColour; break;
		case 2: colour = reservedSlotColour; break;
		case 3: colour = ignoredColour; break;
		case 4: colour = opColour; break;
		case 5: colour = pasiveColour; break;
		default: break;
	}
	CColorDialog d( colour, 0, hWndCtl );
	if (d.DoModal() == IDOK) {
		switch(index) {
			case 0: normalColour = d.GetColor(); break;
			case 1: favoriteColour = d.GetColor(); break;
			case 2: reservedSlotColour = d.GetColor(); break;
			case 3: ignoredColour = d.GetColor(); break;
			case 4: opColour = d.GetColor(); break;
			case 5: pasiveColour = d.GetColor(); break;
			default: break;
		}
		refreshPreview();
	}
	return TRUE;
}

void ListViewColours::refreshPreview() {
	
	n_Preview.SetFont(CreateFontIndirect(&currentFont));

	CHARFORMAT2 cf;
	n_Preview.SetWindowText(_T(""));
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	
	cf.crTextColor = normalColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText(TSTRING(NORMAL).c_str());

	cf.crTextColor = favoriteColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(FAVORITE)).c_str());
	
	cf.crTextColor = reservedSlotColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_RESERVED)).c_str());
	
	cf.crTextColor = ignoredColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_IGNORED)).c_str());

	cf.crTextColor = opColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(COLOR_OP)).c_str());

	cf.crTextColor = pasiveColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(PASSIVE_USER)).c_str());

	n_Preview.InvalidateRect( NULL );
}

void ListViewColours::EditTextStyle() {
	LOGFONT font;

	font = currentFont;

	CFontDialog d(&font, CF_SCREENFONTS, NULL, *this);
	//d.m_cf.rgbColors = textclr;
	if(d.DoModal() == IDOK){
		fontChanged = true;
		currentFont = font;
		refreshPreview();
	}
	
}

void ListViewColours::write() {

	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, normalColour);
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, favoriteColour);
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, reservedSlotColour);
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, ignoredColour);
	SettingsManager::getInstance()->set(SettingsManager::PASIVE_COLOR, pasiveColour);
	SettingsManager::getInstance()->set(SettingsManager::OP_COLOR, opColour);

	//prevent changing the font handle unless its really changed, keeps it compareable for real changes.
	if(fontChanged) {
		WinUtil::listViewFont = CreateFontIndirect(&currentFont);
		SettingsManager::getInstance()->set(SettingsManager::LIST_VIEW_FONT, Text::fromT(WinUtil::encodeFont(currentFont)));
	}

}

}