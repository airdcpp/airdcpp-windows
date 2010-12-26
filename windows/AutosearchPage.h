/* 
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

#ifndef AUTOSEARCH_PAGE_H
#define AUTOSEARCH_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "../client/AutoSearchManager.h"

class AutosearchPage : public CPropertyPage<IDD_AUTOSEARCH>, public PropPage
{
public:
	AutosearchPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_AIRDCPAGE) + _T('\\') + TSTRING(SETTINGS_AIRSEARCH)).c_str());

		SetTitle(title);
	};

	~AutosearchPage() { 
		ctrlAutosearch.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(AutosearchPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_CHANGE, onChange)
		COMMAND_ID_HANDLER(IDC_AUTOSEARCH_ENABLE, onEnable)
		COMMAND_ID_HANDLER(IDC_AUTOSEARCH_ENABLE_TIME, onEnable)
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onMoveUp)
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onMoveDown)
		NOTIFY_HANDLER(IDC_AUTOSEARCH_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_AUTOSEARCH_ITEMS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_AUTOSEARCH_ITEMS, LVN_ITEMCHANGED, onItemChanged)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& bHandled) {
		return onChange(0, 0, 0, bHandled);
	}
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	ExListViewCtrl ctrlAutosearch;

	void fixControls();
	bool gotFocusOnList;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	void addEntry(const Autosearch::Ptr fd, int pos);
	string getType(int i) {
		switch(i) {
			case 0: return STRING(ANY);			break;
			case 1: return STRING(AUDIO);		break;
			case 2: return STRING(COMPRESSED);	break;
			case 3: return STRING(DOCUMENT);	break;
			case 4: return STRING(EXECUTABLE);	break;
			case 5: return STRING(PICTURE);		break;
			case 6: return STRING(VIDEO);		break;
			case 7: return STRING(DIRECTORY);	break;
			case 8: return "TTH";				break;
			case 9: return "RegExp";			break;
			default: return STRING(ANY);
		}
	}
};

#endif //AUTOSEARCH_PAGE_H