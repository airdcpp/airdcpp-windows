/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

/*
 * Automatic Directory Listing Search
 * Henrik Engström, henrikengstrom at home se
 */

#include "stdafx.h"
#include "Resource.h"
#include "../client/DCPlusPlus.h"
#include "../client/Client.h"
#include "ADLSearchFrame.h"
#include "AdlsProperties.h"

int ADLSearchFrame::columnIndexes[] = { 
	COLUMN_ACTIVE_SEARCH_STRING,
	COLUMN_SOURCE_TYPE,
	COLUMN_DEST_DIR,
	COLUMN_MIN_FILE_SIZE,
	COLUMN_MAX_FILE_SIZE,
	COLUMN_COMMENT,
	COLUMN_RAW,
	COLUMN_REGEXP
};
int ADLSearchFrame::columnSizes[] = { 
	120, 
	90, 
	90, 
	90, 
	90,
	150,
	20,
	20
};
static ResourceManager::Strings columnNames[] = { 
	ResourceManager::ACTIVE_SEARCH_STRING, 
	ResourceManager::SOURCE_TYPE, 
	ResourceManager::DESTINATION, 
	ResourceManager::SIZE_MIN, 
	ResourceManager::MAX_SIZE, 
	ResourceManager::COMMENT,
	ResourceManager::RAW,
	ResourceManager::REGEXP
};

// Frame creation
LRESULT ADLSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// Create status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	int w[1] = { 0 };
	ctrlStatus.SetParts(1, w);

	// Create list control
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_ADLLIST);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	// Set background color
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(ADLSEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(ADLSEARCHFRAME_WIDTHS), COLUMN_LAST);
	for(int j = 0; j < COLUMN_LAST; j++) 
	{
		int fmt = LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	// Create buttons
	ctrlAdd.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_ADD);
	ctrlAdd.SetWindowText(CTSTRING(NEW));
	ctrlAdd.SetFont(WinUtil::font);

	ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_EDIT);
	ctrlEdit.SetWindowText(CTSTRING(PROPERTIES));
	ctrlEdit.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlMoveUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlMoveUp.SetWindowText(CTSTRING(MOVE_UP));
	ctrlMoveUp.SetFont(WinUtil::font);

	ctrlMoveDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlMoveDown.SetWindowText(CTSTRING(MOVE_DOWN));
	ctrlMoveDown.SetFont(WinUtil::font);

	ctrlReload.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_RELOAD);
	ctrlReload.SetWindowText(CTSTRING(RELOAD));
	ctrlReload.SetFont(WinUtil::font);

	ctrlHelp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_HELP_FAQ);
	ctrlHelp.SetWindowText(CTSTRING(WHATS_THIS));
	ctrlHelp.SetFont(WinUtil::font);

	// Create context menu
	contextMenu.CreatePopupMenu();
	contextMenu.AppendMenu(MF_STRING, IDC_ADD,    CTSTRING(NEW));
	contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	contextMenu.AppendMenu(MF_STRING, IDC_EDIT,   CTSTRING(PROPERTIES));

	SettingsManager::getInstance()->addListener(this);
	// Load all searches
	LoadAll();

	WinUtil::SetIcon(m_hWnd, _T("ADLSearch.ico"));
	bHandled = FALSE;
	return TRUE;
}

// Close window
LRESULT ADLSearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) 
{	
	if(!closed) {
		closed = true;		
		ADLSearchManager::getInstance()->Save();
		SettingsManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_FILE_ADL_SEARCH, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::ADLSEARCHFRAME_ORDER, 
		SettingsManager::ADLSEARCHFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		bHandled = FALSE;
		return 0;
	}	
}

// Recalculate frame control layout
void ADLSearchFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) 
{
	RECT rect;
	GetClientRect(&rect);

	// Position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	if(ctrlStatus.IsWindow()) 
	{
		CRect sr;
		int w[1];
		ctrlStatus.GetClientRect(sr);
		w[0] = sr.Width() - 16;
		ctrlStatus.SetParts(1, w);
	}

	// Position list control
	CRect rc = rect;
	rc.top += 2;
	rc.bottom -= 28;
	ctrlList.MoveWindow(rc);

	// Position buttons
	const long bwidth = 90;
	const long bspace = 10;
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;

	rc.left = 2;
	rc.right = rc.left + bwidth;
	ctrlAdd.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlEdit.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlRemove.MoveWindow(rc);

	rc.left += bspace;

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlMoveUp.MoveWindow(rc);

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlMoveDown.MoveWindow(rc);

	rc.left += bspace;

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlReload.MoveWindow(rc);

	rc.left += bspace;

	rc.left += bwidth + 2;
	rc.right = rc.left + bwidth;
	ctrlHelp.MoveWindow(rc);

}

// Keyboard shortcuts
LRESULT ADLSearchFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) 
	{
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_EDIT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}
	
LRESULT ADLSearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlList) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList, pt);
		}

		int status = ctrlList.GetSelectedCount() > 0 ? MFS_ENABLED : MFS_GRAYED;
		contextMenu.EnableMenuItem(IDC_EDIT, status);
		contextMenu.EnableMenuItem(IDC_REMOVE, status);
		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE; 
	}
	bHandled = FALSE;
	return FALSE; 
}

// Add new search
LRESULT ADLSearchFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	// Invoke edit dialog with fresh search
	if (ADLSearchManager::getInstance()->getRunning() > 0) {
		LogManager::getInstance()->message(CSTRING(ADLSEARCH_IN_PROGRESS));
		return 0;
	}

	ADLSearch* search = new ADLSearch();
	ADLSProperties dlg(search);
	if(dlg.DoModal((HWND)*this) == IDOK)
	{
		// Add new search to the end or if selected, just before
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		

		int i = ctrlList.GetNextItem(-1, LVNI_SELECTED);
		if(i < 0)
		{
			// Add to end
			if (!ADLSearchManager::getInstance()->addCollection(search, true, true)) {
				return 0;
			}
			i = collection.size() - 1;
		}
		else
		{
			// Add before selection
			if (!ADLSearchManager::getInstance()->addCollection(search, true, true, true, i)) {
				return 0;
			}
		}

		// Update list control
		int j = i;
		while(j < (int)collection.size())
		{
			UpdateSearch(j++);
		}
		ctrlList.EnsureVisible(i, FALSE);
	}

	return 0;
}

// Edit existing search
LRESULT ADLSearchFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	// Get selection info
	int i = ctrlList.GetNextItem(-1, LVNI_SELECTED);
	if(i < 0)
	{
		// Nothing selected
		return 0;
	}

	// Edit existing
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	ADLSearch* search = collection[i];

	// Invoke dialog with selected search
	ADLSProperties dlg(search);
	if(dlg.DoModal((HWND)*this) == IDOK)
	{
		// Update search collection
		collection[i] = search;

		// Update list control
		UpdateSearch(i);	  
	}

	return 0;
}

// Remove searches
LRESULT ADLSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	// Loop over all selected items
	int i;
	while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) >= 0)
	{
		if (!ADLSearchManager::getInstance()->removeCollection(i, false)) {
			return 0;
		}
		//collection.erase(collection.begin() + i);
		ctrlList.DeleteItem(i);
	}
	return 0;
}

LRESULT ADLSearchFrame::onReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::getInstance()->Load();
	LoadAll();
	return 0;
}

// Help
LRESULT ADLSearchFrame::onHelp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	TCHAR title[] =
		_T("ADLSearch brief description");

	TCHAR message[] = 
		_T("ADLSearch is a tool for fast searching of directory listings downloaded from users. \n")
		_T("Create a new ADLSearch entering 'avi' as search string for example. When you \n")
		_T("download a directory listing from a user, all avi-files will be placed in a special folder \n")
		_T("called <<<ADLSearch>>> for easy finding. It is almost the same as using the standard \n")
		_T("'Find' multiple times in a directory listing. \n")
		_T("\n")
		_T("Special options: \n")
		_T("- 'Active' check box selects if the search is used or not. \n")
		_T("- 'Source Type' can be the following options; 'Filename' matches search against filename, \n")
		_T("   'Directory' matches against current subdirectory and places the whole structure in the \n")
		_T("   special folder, 'Full Path' matches against whole directory + filename. \n")
		_T("- 'Destination Directory' selects the special output folder for a search. Multiple folders \n")
		_T("   with different names can exist simultaneously. \n")
		_T("- 'Min/Max Size' sets file size limits. This is not used for 'Directory' type searches. \n")
		_T("- 'Move Up'/'Move Down' can be used to organize the list of searches. \n")
		_T("\n")
		_T("There is a new option in the context menu (right-click) for directory listings. It is called \n")
		_T("'Go to directory' and can be used to jump to the original location of the file or directory. \n")
		_T("\n")
		_T("Extra features:\n")
		_T("\n")
		_T("1) If you use %y.%m.%d in a search string it will be replaced by todays date. Switch \n")
		_T("place on y/m/d, or leave any of them out to alter the substitution. If you use %[nick] \n")
		_T("it will be replaced by the nick of the user you download the directory listing from. \n")
		_T("\n")
		_T("2) If you name a destination directory 'discard', it will not be shown in the total result. \n")
		_T("Useful with the extra feature 3) below to remove uninteresting results. \n")
		_T(" \n")
		_T("3) There is a switch called 'Break on first ADLSearch match' in Settings->Advanced'.  \n")
		_T("If enabled, ADLSearch will stop after the first match for a specific file/directory. \n")
		_T("The order in the ADLSearch windows is therefore important. Example: Add a search \n")
		_T("item at the top of the list with string='xxx' and destination='discard'. It will catch \n")
		_T("many pornographic files and they will not be included in any following search results. \n")
		;

	MessageBox(message, title, MB_OK);
	return 0;
}

// Move selected entries up one step
LRESULT ADLSearchFrame::onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Get selection
	vector<int> sel;
	int i = -1;
	while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) >= 0)
	{
		sel.push_back(i);
	}
	if(sel.size() < 1)
	{
		return 0;
	}

	// Find out where to insert
	int i0 = sel[0];
	if(i0 > 0)
	{
		i0 = i0 - 1;
	}

	// Backup selected searches
	ADLSearchManager::SearchCollection backup;
	for(i = 0; i < (int)sel.size(); ++i)
	{
		backup.push_back(collection[sel[i]]);
	}

	// Erase selected searches
	for(i = sel.size() - 1; i >= 0; --i)
	{
		if (!ADLSearchManager::getInstance()->removeCollection(sel[i], true)) {
			return 0;
		}
		//collection.erase(collection.begin() + sel[i]);
	}

	// Insert (grouped together)
	for(i = 0; i < (int)sel.size(); ++i)
	{
		if (!ADLSearchManager::getInstance()->addCollection(backup[i], true, false, true, i0 + i)) {
			return 0;
		}
		//collection.insert(collection.begin() + i0 + i, backup[i]);
	}

	// Update UI
	LoadAll();

	// Restore selection
	for(i = 0; i < (int)sel.size(); ++i)
	{
		ctrlList.SetItemState(i0 + i, LVNI_SELECTED, LVNI_SELECTED);
	}

	return 0;
}

// Move selected entries down one step
LRESULT ADLSearchFrame::onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Get selection
	vector<int> sel;
	int i = -1;
	while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) >= 0)
	{
		sel.push_back(i);
	}
	if(sel.size() < 1)
	{
		return 0;
	}

	// Find out where to insert
	int i0 = sel[sel.size() - 1] + 2;
	if(i0 > (int)collection.size())
	{
		i0 = collection.size();
	}

	// Backup selected searches
	ADLSearchManager::SearchCollection backup;
	for(i = 0; i < (int)sel.size(); ++i)
	{
		backup.push_back(collection[sel[i]]);
	}

	// Erase selected searches
	for(i = sel.size() - 1; i >= 0; --i)
	{
		if (!ADLSearchManager::getInstance()->removeCollection(sel[i], true)) {
			return 0;
		}
		if(i < i0)
		{
			i0--;
		}
	}

	// Insert (grouped together)
	for(i = 0; i < (int)sel.size(); ++i)
	{
		if (!ADLSearchManager::getInstance()->addCollection(backup[i], true, false, true, i0 + i)) {
			return 0;
		}
	}

	// Update UI
	LoadAll();

	// Restore selection
	for(i = 0; i < (int)sel.size(); ++i)
	{
		ctrlList.SetItemState(i0 + i, LVNI_SELECTED, LVNI_SELECTED);
	}
	ctrlList.EnsureVisible(i0, FALSE);

	return 0;
}

// Clicked 'Active' check box
LRESULT ADLSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) 
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_EDIT), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));

	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if((item->uChanged & LVIF_STATE) == 0)
		return 0;
	if((item->uOldState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;
	if((item->uNewState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;

	if(item->iItem >= 0)
	{
		// Set new active status check box
		/*ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch* search = collection[item->iItem];
		search->isActive = (ctrlList.GetCheckState(item->iItem) != 0);*/
		ADLSearchManager::getInstance()->changeState(item->iItem, ctrlList.GetCheckState(item->iItem) != 0);
	}
	return 0;
}

// Double-click on list control
LRESULT ADLSearchFrame::onDoubleClickList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) 
{
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		// Treat as onEdit command
		PostMessage(WM_COMMAND, IDC_EDIT, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}

	return 0;
}

// Load all searches from manager
void ADLSearchFrame::LoadAll()
{
	// Clear current contents
	ctrlList.DeleteAllItems();

	// Load all searches
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	for(unsigned long l = 0; l < collection.size(); l++)
	{
		UpdateSearch(l, FALSE);
	}
}

// Update a specific search item
void ADLSearchFrame::UpdateSearch(int index, BOOL doDelete)
{
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

	// Check args
	if(index >= (int)collection.size())
	{
		return;
	}
	ADLSearch* search = collection[index];

	// Delete from list control
	if(doDelete)
	{
		ctrlList.DeleteItem(index);
	}

	// Generate values
	TStringList line;
	tstring fs;
	line.push_back(Text::toT(search->searchString));
	line.push_back(search->SourceTypeToDisplayString(search->sourceType));
	line.push_back(Text::toT(search->destDir));

	fs = _T("");
	if(search->minFileSize >= 0)
	{
		fs = Util::toStringW(search->minFileSize);
		fs += _T(" ");
		fs += search->SizeTypeToDisplayString(search->typeFileSize);
	}
	line.push_back(fs);

	fs = _T("");
	if(search->maxFileSize >= 0)
	{
		fs = Util::toStringW(search->maxFileSize);
		fs += _T(" ");
		fs += search->SizeTypeToDisplayString(search->typeFileSize);
	}
	line.push_back(fs);

	line.push_back(Text::toT(search->adlsComment));
	line.push_back(Util::toStringW(search->isRegexp));


	// Insert in list control
	ctrlList.insert(index, line);

	// Update 'Active' check box
	ctrlList.SetCheckState(index, search->isActive);
}

void ADLSearchFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/**
 * @file
 * $Id: ADLSearchFrame.cpp 477 2010-01-29 08:59:43Z bigmuscle $
 */
