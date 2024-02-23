/* 
 * Copyright (C) 2012-2024 AirDC++ Project
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

#if !defined(SPEED_PAGE_H)
#define SPEED_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"

#include <airdcpp/SettingsManager.h>

class SpeedPage : public CPropertyPage<IDD_SPEEDPAGE>, public PropPage
{
public:
	SpeedPage(SettingsManager *s) : PropPage(s), loading(true) {
		title = _tcsdup((TSTRING(SETTINGS_NETWORK) + _T('\\') + TSTRING(SETTINGS_SPEED_SLOTS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};
	~SpeedPage() { free(title); };

	BEGIN_MSG_MAP(SpeedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_UL_AUTODETECT, onEnable)
		COMMAND_ID_HANDLER(IDC_DL_AUTODETECT, onEnable)
		COMMAND_ID_HANDLER(IDC_MCN_AUTODETECT, onEnable)
		COMMAND_HANDLER(IDC_DL_SPEED, CBN_EDITCHANGE, onSpeedChanged)
		COMMAND_HANDLER(IDC_CONNECTION, CBN_EDITCHANGE, onSpeedChanged)
		COMMAND_HANDLER(IDC_DL_SPEED, CBN_SELENDOK , onSpeedChanged)
		COMMAND_HANDLER(IDC_CONNECTION, CBN_SELENDOK, onSpeedChanged)
		COMMAND_HANDLER(IDC_MCNDLSLOTS, EN_UPDATE, checkMCN)
		COMMAND_HANDLER(IDC_MCNULSLOTS, EN_UPDATE, checkMCN)
		COMMAND_HANDLER(IDC_SLOTS, EN_UPDATE, onSlotsChanged)
		COMMAND_HANDLER(IDC_DOWNLOADS, EN_UPDATE, onSlotsChanged)
	END_MSG_MAP()

	LRESULT onSpeedChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSlotsChanged(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onEnable(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT checkMCN(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void setDownloadLimits(double value);
	void setUploadLimits(double value);
	void updateValues(WORD wNotifyCode);
	void validateMCNLimits(WORD wNotifyCode);
		
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	CComboBox ctrlUpload;
	CComboBox ctrlDownload;

	CButton cAutoUL;
	CButton cAutoDL;

	CEdit ctrlMcnUL;
	CEdit ctrlMcnDL;

	void fixControls();
	int maxMCNExtras(double speed);
	static Item items[];

	static Item uploadItems[];
	static Item downloadItems[];
	static Item mcnItems[];

	static TextItem texts[];
	TCHAR* title;

	bool loading;
};

#endif //SPEED_PAGE_H