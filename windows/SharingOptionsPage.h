#if !defined(AIRSHARING_PAGE_H)
#define AIRSHARING_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"


class SharingOptionsPage : public CPropertyPage<IDD_SHARING_OPTIONS>, public PropPage
{
public:
	SharingOptionsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_SHARINGPAGE) + _T('\\') + TSTRING(SETTINGS_SHARING_OPTIONS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~SharingOptionsPage() {
		free(title);
	}

	BEGIN_MSG_MAP(SharingOptionsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:

	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	TCHAR* title;

};

#endif // !defined(AIRSHARING_PAGE_H)


