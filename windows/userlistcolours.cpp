
#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

#include "Resource.h"
#include "UserListColours.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem UserListColours::texts[] = {
	{ IDC_CHANGE_COLOR, ResourceManager::SETTINGS_CHANGE },
	{ IDC_USERLIST, ResourceManager::USERLIST_ICONS },
	{ IDC_IMAGEBROWSE, ResourceManager::BROWSE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item UserListColours::items[] = {
	{ IDC_USERLIST_IMAGE, SettingsManager::USERLIST_IMAGE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

LRESULT UserListColours::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	normalColour = SETTING(NORMAL_COLOUR);
	favoriteColour = SETTING(FAVORITE_COLOR);
	reservedSlotColour = SETTING(RESERVED_SLOT_COLOR);
	ignoredColour = SETTING(IGNORED_COLOR);
	pasiveColour = SETTING(PASIVE_COLOR);
	opColour = SETTING(OP_COLOR);

	n_lsbList.Attach( GetDlgItem(IDC_USERLIST_COLORS) );
	n_Preview.Attach( GetDlgItem(IDC_PREVIEW) );
	n_Preview.SetBackgroundColor(WinUtil::bgColor);
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_NORMAL));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_FAVORITE));	
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_RESERVED));
	n_lsbList.AddString(CTSTRING(SETTINGS_COLOR_IGNORED));
	n_lsbList.AddString(CTSTRING(COLOR_OP));
	n_lsbList.AddString(CTSTRING(COLOR_PASIVE));
	n_lsbList.SetCurSel( 0 );

	refreshPreview();

	return TRUE;
}
LRESULT UserListColours::onChangeColour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
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

void UserListColours::refreshPreview() {

	CHARFORMAT2 cf;
	n_Preview.SetWindowText(_T(""));
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	
	cf.crTextColor = normalColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText(TSTRING(SETTINGS_COLOR_NORMAL).c_str());

	cf.crTextColor = favoriteColour;
	n_Preview.SetSelectionCharFormat(cf);
	n_Preview.AppendText((_T("\r\n") + TSTRING(SETTINGS_COLOR_FAVORITE)).c_str());
	
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
	n_Preview.AppendText((_T("\r\n") + TSTRING(COLOR_PASIVE)).c_str());

	n_Preview.InvalidateRect( NULL );
}

void UserListColours::write() {
	if(PropertiesDlg::needUpdate)
	{
		SendMessage(WM_DESTROY,0,0);
		SendMessage(WM_INITDIALOG,0,0);
	}
	PropPage::write((HWND)*this, items);
	SettingsManager::getInstance()->set(SettingsManager::NORMAL_COLOUR, normalColour);
	SettingsManager::getInstance()->set(SettingsManager::FAVORITE_COLOR, favoriteColour);
	SettingsManager::getInstance()->set(SettingsManager::RESERVED_SLOT_COLOR, reservedSlotColour);
	SettingsManager::getInstance()->set(SettingsManager::IGNORED_COLOR, ignoredColour);
	SettingsManager::getInstance()->set(SettingsManager::PASIVE_COLOR, pasiveColour);
	SettingsManager::getInstance()->set(SettingsManager::OP_COLOR, opColour);

	WinUtil::reLoadImages(); // User Icon Begin / End

}

void UserListColours::BrowseForPic(int DLGITEM) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(DLGITEM, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(DLGITEM, x.c_str());
	}
}

LRESULT UserListColours::onImageBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BrowseForPic(IDC_USERLIST_IMAGE);
	return 0;
}