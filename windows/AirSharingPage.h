#if !defined(AIRSHARING_PAGE_H)
#define AIRSHARING_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"


class AirSharingPage : public CPropertyPage<IDD_AIRSHARINGPAGE>, public PropPage
{
public:
	AirSharingPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_AIRDCPAGE) + _T('\\') + TSTRING(SETTINGS_AIRSHARING)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~AirSharingPage() {
//		free(title);
	}

	BEGIN_MSG_MAP(AirSharingPage)
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


