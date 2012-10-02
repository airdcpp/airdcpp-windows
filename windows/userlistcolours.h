#ifndef UserListColours_H
#define UserListColours_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"

class UserListColours : public CPropertyPage<IDD_USERLIST_COLOURS>, public PropPage
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
		COMMAND_HANDLER(IDC_IMAGEBROWSE, BN_CLICKED, onImageBrowse)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onChangeColour(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onImageBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	CRichEditCtrl n_Preview;
private:
	void BrowseForPic(int DLGITEM);

	void refreshPreview();

	CListBox n_lsbList;
	int normalColour;
	int favoriteColour;
	int reservedSlotColour;
	int ignoredColour;
	int pasiveColour;
	int opColour; 

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;
};

#endif //UserListColours_H