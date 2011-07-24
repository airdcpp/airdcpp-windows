

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
		title = _tcsdup((TSTRING(SETTINGS_AIRDCPAGE) + _T('\\') + TSTRING(SETTINGS_AIRDOWNLOADS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~AirDownloadsPage() {
		free(title);
	}

	BEGIN_MSG_MAP(AirDownloadsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_ANTIVIR_BROWSE, BN_CLICKED, onBrowse)

	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	


	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

};

#endif // !defined(AIRDOWNLOADS_PAGE_H)


