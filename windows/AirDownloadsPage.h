

#if !defined(AIRDOWNLOADS_PAGE_H)
#define AIRDOWNLOADS_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"


class AirDownloadsPage : public CPropertyPage<IDD_AIRDOWNLOADSPAGE>, public PropPage
{
public:
	AirDownloadsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(SETTINGS_AIRDOWNLOADS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~AirDownloadsPage() {
		free(title);
	}

	BEGIN_MSG_MAP(AirDownloadsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
protected:

	static Item items[];
	static TextItem texts[];
	static ListItem optionItems[];
	TCHAR* title;

};

#endif // !defined(AIRDOWNLOADS_PAGE_H)


