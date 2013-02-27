#ifndef UserListColours_H
#define UserListColours_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"

class UserListColours : public CPropertyPage<IDD_USERLIST_COLOURS>, public PropPage, private SettingsManagerListener
{
public:

	UserListColours(SettingsManager *s) : PropPage(s) { 
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_TEXT_STYLES) + _T('\\') + TSTRING(SETTINGS_USER_COLORS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~UserListColours() { free(title); };

	BEGIN_MSG_MAP(UserListColours)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_CHANGE_COLOR, BN_CLICKED, onChangeColour)
		COMMAND_ID_HANDLER(IDC_LIST_VIEW_FONT, onEditFont)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onChangeColour(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		SettingsManager::getInstance()->removeListener(this);
		n_lsbList.ResetContent();
		n_lsbList.Detach();
		n_Preview.Detach();
		return 1;
	}

	LRESULT onEditFont(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		EditTextStyle();
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	CRichEditCtrl n_Preview;
private:

	void refreshPreview();
	void EditTextStyle();

	CListBox n_lsbList;
	int normalColour;
	int favoriteColour;
	int reservedSlotColour;
	int ignoredColour;
	int pasiveColour;
	int opColour; 
	LOGFONT currentFont;
	bool fontChanged;

	virtual void on(SettingsManagerListener::ReloadPages, int) {
		SendMessage(WM_DESTROY,0,0);
		SendMessage(WM_INITDIALOG,0,0);
	}

protected:

	static TextItem texts[];
	TCHAR* title;
};

#endif //UserListColours_H