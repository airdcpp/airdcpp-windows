/* 
* Copyright (C) 2003-2006 Pär Björklund, per.bjorklund@gmail.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
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

#if !defined(HIGHLIGHTDIALOG_H)
#define HIGHLIGHTDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "../client/SettingsManager.h"
#include "../client/HighlightManager.h"

class FulHighlightDialog: public CDialogImpl<FulHighlightDialog>
{
public:
	FulHighlightDialog() : bgColor(RGB(255,255,255)), fgColor(RGB(0, 0, 0)) { };
	FulHighlightDialog(ColorSettings& aCs) : cs(aCs),
					bgColor(RGB(255,255,255)), fgColor(RGB(0, 0, 0)) { };
	virtual ~FulHighlightDialog();

	enum { IDD = IDD_HIGHLIGHTDIALOG };

	BEGIN_MSG_MAP(FulHighlightDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BGCOLOR, onBgColor)
		COMMAND_ID_HANDLER(IDC_FGCOLOR, onFgColor)
		COMMAND_ID_HANDLER(IDC_SELECT_SOUND, onSelSound)
		COMMAND_ID_HANDLER(IDC_SOUND, onClickedBox)
		COMMAND_ID_HANDLER(IDC_HAS_BG_COLOR, onClickedBox)		
		COMMAND_ID_HANDLER(IDC_HAS_FG_COLOR, onClickedBox)
		COMMAND_ID_HANDLER(IDOK, onOk)
		COMMAND_ID_HANDLER(IDCANCEL, onCancel)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT onOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBgColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFgColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelSound(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedBox(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	ColorSettings& getColorSetting() { return cs; }

protected:
	void getValues();
	void initControls();

	static PropPage::TextItem texts[];

	CComboBox ctrlMatchType;

	COLORREF bgColor;
	COLORREF fgColor;

	tstring soundFile;

	CButton ctrlButton;

	ColorSettings cs;
	
};

#endif //HIGHLIGHTPAGE_H
