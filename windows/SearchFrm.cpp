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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "MainFrm.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "BarShader.h"
#include "../client/Wildcards.h"

#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/SearchManager.h"


int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_HITS, COLUMN_NICK, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_IP, COLUMN_TTH };
int SearchFrame::columnSizes[] = { 210, 80, 100, 50, 80, 100, 40, 70, 150, 80, 100, 150 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE,  ResourceManager::HIT_COUNT, ResourceManager::USER, ResourceManager::TYPE, ResourceManager::SIZE,
	ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION, 
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT };

SearchFrame::FrameMap SearchFrame::frames;

void SearchFrame::openWindow(const tstring& str /* = Util::emptyString */, LONGLONG size /* = 0 */, SearchManager::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, SearchManager::TypeModes type /* = SearchManager::TYPE_ANY ( 0 ) */) {
	SearchFrame* pChild = new SearchFrame();
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);

	frames.insert( FramePair(pChild->m_hWnd, pChild) );
}

void SearchFrame::closeAll() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		::PostMessage(i->first, WM_CLOSE, 0, 0);
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{


	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlSearchBox.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);

	lastSearches = SettingsManager::getInstance()->getSearchHistory();
	for(TStringIter i = lastSearches.begin(); i != lastSearches.end(); ++i) {
		ctrlSearchBox.InsertString(0, i->c_str());
	}

	searchBoxContainer.SubclassWindow(ctrlSearchBox.m_hWnd);
	ctrlSearchBox.SetExtendedUI();
	
	ctrlPurge.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_PURGE);
	ctrlPurge.SetWindowText(CTSTRING(PURGE));
	ctrlPurge.SetFont(WinUtil::systemFont);
	purgeContainer.SubclassWindow(ctrlPurge.m_hWnd);
	
	ctrlMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	modeContainer.SubclassWindow(ctrlMode.m_hWnd);

	ctrlSize.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	sizeContainer.SubclassWindow(ctrlSize.m_hWnd);
	
	ctrlSkiplist.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	ctrlSkiplist.SetFont(WinUtil::font);
	ctrlSkiplist.SetWindowText(Text::toT(SETTING(SKIPLIST_SEARCH)).c_str());
	ctrlSkiplistContainer.SubclassWindow(ctrlSkiplist.m_hWnd);

	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	sizeModeContainer.SubclassWindow(ctrlSizeMode.m_hWnd);

	ctrlFiletype.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);

	searchTypes.CreateFromImage(IDB_SEARCH_TYPES, 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	fileTypeContainer.SubclassWindow(ctrlFiletype.m_hWnd);

	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
		ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_RESULTS);
	} else {
		ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_RESULTS);
	}
	ctrlResults.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	resultsContainer.SubclassWindow(ctrlResults.m_hWnd);
	
	if (BOOLSETTING(USE_SYSTEM_ICONS)) {
		ctrlResults.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	} else {
		images.CreateFromImage(IDB_SPEEDS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		ctrlResults.SetImageList(images, LVSIL_SMALL);
	}

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	hubsContainer.SubclassWindow(ctrlHubs.m_hWnd);	

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::font);

	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);

	ctrlFilterSelContainer.SubclassWindow(ctrlFilterSel.m_hWnd);
	ctrlFilterSel.SetFont(WinUtil::font);

	searchLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	searchLabel.SetFont(WinUtil::systemFont, FALSE);
	searchLabel.SetWindowText(CTSTRING(SEARCH_FOR));

	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CTSTRING(SIZE));

	ctrlSkipBool.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_USKIPLIST);
	ctrlSkipBool.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlSkipBool.SetFont(WinUtil::systemFont, FALSE);
	ctrlSkipBool.SetWindowText(CTSTRING(SKIPLIST));
	SkipBoolContainer.SubclassWindow(ctrlSkipBool.m_hWnd);

	typeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	typeLabel.SetFont(WinUtil::systemFont, FALSE);
	typeLabel.SetWindowText(CTSTRING(FILE_TYPE));

	srLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	srLabel.SetFont(WinUtil::systemFont, FALSE);
	srLabel.SetWindowText(CTSTRING(SEARCH_IN_RESULTS));

	optionLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	optionLabel.SetFont(WinUtil::systemFont, FALSE);
	optionLabel.SetWindowText(CTSTRING(SEARCH_OPTIONS));

	hubsLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	hubsLabel.SetFont(WinUtil::systemFont, FALSE);
	hubsLabel.SetWindowText(CTSTRING(HUBS));

	ctrlSlots.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FREESLOTS);
	ctrlSlots.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlSlots.SetFont(WinUtil::systemFont, FALSE);
	ctrlSlots.SetWindowText(CTSTRING(ONLY_FREE_SLOTS));
	slotsContainer.SubclassWindow(ctrlSlots.m_hWnd);

	ctrlCollapsed.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_COLLAPSED);
	ctrlCollapsed.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlCollapsed.SetFont(WinUtil::systemFont, FALSE);
	ctrlCollapsed.SetWindowText(CTSTRING(EXPANDED_RESULTS));
	collapsedContainer.SubclassWindow(ctrlCollapsed.m_hWnd);

	if(BOOLSETTING(FREE_SLOTS_DEFAULT)) {
		ctrlSlots.SetCheck(true);
		onlyFree = true;
	}
	if(BOOLSETTING(SEARCH_SKIPLIST)) {
		ctrlSkipBool.SetCheck(true);
		UseSkiplist = true;
	}

	if(BOOLSETTING(EXPAND_DEFAULT)) {
		ctrlCollapsed.SetCheck(true);
		expandSR = true;
	}

	ctrlShowUI.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowUI.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowUI.SetCheck(1);
	showUIContainer.SubclassWindow(ctrlShowUI.m_hWnd);

	ctrlDoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_SEARCH);
	ctrlDoSearch.SetWindowText(CTSTRING(SEARCH));
	ctrlDoSearch.SetFont(WinUtil::systemFont);
	doSearchContainer.SubclassWindow(ctrlDoSearch.m_hWnd);

	ctrlPauseSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_SEARCH_PAUSE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
	ctrlPauseSearch.SetFont(WinUtil::systemFont);

	ctrlSearchBox.SetFont(WinUtil::systemFont, FALSE);
	ctrlSize.SetFont(WinUtil::systemFont, FALSE);
	ctrlMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlFiletype.SetFont(WinUtil::systemFont, FALSE);

	ctrlMode.AddString(CTSTRING(NORMAL));
	ctrlMode.AddString(CTSTRING(AT_LEAST));
	ctrlMode.AddString(CTSTRING(AT_MOST));
	ctrlMode.AddString(CTSTRING(EXACT_SIZE));
	ctrlMode.SetCurSel(1);
	
	ctrlSizeMode.AddString(CTSTRING(B));
	ctrlSizeMode.AddString(CTSTRING(KiB));
	ctrlSizeMode.AddString(CTSTRING(MiB));
	ctrlSizeMode.AddString(CTSTRING(GiB));
	if(initialSize == 0)
		ctrlSizeMode.SetCurSel(2);
	else
		ctrlSizeMode.SetCurSel(0);

	ctrlFiletype.AddString(CTSTRING(ANY));
	ctrlFiletype.AddString(CTSTRING(AUDIO));
	ctrlFiletype.AddString(CTSTRING(COMPRESSED));
	ctrlFiletype.AddString(CTSTRING(DOCUMENT));
	ctrlFiletype.AddString(CTSTRING(EXECUTABLE));
	ctrlFiletype.AddString(CTSTRING(PICTURE));
	ctrlFiletype.AddString(CTSTRING(VIDEO));
	ctrlFiletype.AddString(CTSTRING(DIRECTORY));
	ctrlFiletype.AddString(_T("TTH"));
	ctrlFiletype.SetCurSel(SETTING(LAST_SEARCH_FILETYPE));
	
	ctrlSkiplist.AddString(Text::toT(SETTING(SKIP_MSG_01)).c_str());
	ctrlSkiplist.AddString(Text::toT(SETTING(SKIP_MSG_02)).c_str());
	ctrlSkiplist.AddString(Text::toT(SETTING(SKIP_MSG_03)).c_str());


	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

	for(uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlResults.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlResults.setColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlResults.setVisible(SETTING(SEARCHFRAME_VISIBLE));
	
	if(BOOLSETTING(SORT_DIRS)) {
		ctrlResults.setSortColumn(COLUMN_FILENAME);
	} else {
		ctrlResults.setSortColumn(COLUMN_HITS);
		ctrlResults.setAscending(false);
	}
	ctrlResults.SetBkColor(WinUtil::bgColor);
	ctrlResults.SetTextBkColor(WinUtil::bgColor);
	ctrlResults.SetTextColor(WinUtil::textColor);
	ctrlResults.SetFont(WinUtil::systemFont, FALSE);	// use WinUtil::font instead to obey Appearace settings
	ctrlResults.setFlickerFree(WinUtil::bgBrush);
	
	ctrlHubs.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, LVSCW_AUTOSIZE, 0);
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	ctrlHubs.SetFont(WinUtil::systemFont, FALSE);	// use WinUtil::font instead to obey Appearace settings

	initHubs();

	UpdateLayout();

	if(!initialString.empty()) {
		if(SettingsManager::getInstance()->addSearchToHistory(initialString))
			ctrlSearchBox.InsertString(0, initialString.c_str());
		ctrlSearchBox.SetCurSel(0);
		ctrlMode.SetCurSel(initialMode);
		ctrlSize.SetWindowText(Util::toStringW(initialSize).c_str());
		ctrlFiletype.SetCurSel(initialType);
		onEnter();
	} else {
		SetWindowText(CTSTRING(SEARCH));
		::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), FALSE);
	}

	for(int j = 0; j < COLUMN_LAST; j++) {
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	ctrlFilterSel.SetCurSel(0);

	CRect rc(SETTING(SEARCH_LEFT), SETTING(SEARCH_TOP), SETTING(SEARCH_RIGHT), SETTING(SEARCH_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	SettingsManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	WinUtil::SetIcon(m_hWnd, _T("search.ico"));

	ctrlStatus.SetText(1, 0, SBT_OWNERDRAW);
	
	bHandled = FALSE;
	return 1;
}

LRESULT SearchFrame::onMeasure(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	bHandled = FALSE;
	
	if(wParam == IDC_FILETYPES) {
		bHandled = TRUE;
		return ListMeasure((MEASUREITEMSTRUCT *)lParam);
	} else if(((MEASUREITEMSTRUCT *)lParam)->CtlType == ODT_MENU) {
		bHandled = TRUE;
		return OMenu::onMeasureItem(hwnd, uMsg, wParam, lParam, bHandled);
	}
	
	return S_OK;
}

LRESULT SearchFrame::onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
	bHandled = FALSE;
	
	if(wParam == IDC_FILETYPES) {
		bHandled = TRUE;
		return ListDraw(dis);
	} else if(dis->CtlID == ATL_IDW_STATUS_BAR && dis->itemID == 1){
		if(searchStartTime > 0){
			bHandled = TRUE;

			RECT rc = dis->rcItem;
			int borders[3];

			ctrlStatus.GetBorders(borders);

			CDC dc(dis->hDC);

			uint64_t now = GET_TICK();
			uint64_t length = min((uint64_t)(rc.right - rc.left), (rc.right - rc.left) * (now - searchStartTime) / (searchEndTime - searchStartTime));

			OperaColors::FloodFill(dc, rc.left, rc.top,  rc.left + (LONG)length, rc.bottom, RGB(128,128,128), RGB(160,160,160));

			dc.SetBkMode(TRANSPARENT);

			uint64_t percent = (now - searchStartTime) * 100 / (searchEndTime - searchStartTime);
			tstring progress = percent >= 100 ? TSTRING(DONE) : Text::toT(Util::toString(percent) + "%");
			tstring buf = TSTRING(SEARCHING_FOR) + _T(" ") + target + _T(" ... ") + progress;

			int textHeight = WinUtil::getTextHeight(dc);
			LONG top = rc.top + (rc.bottom - rc.top - textHeight) / 2;

			dc.SetTextColor(RGB(255, 255, 255));
			RECT rc2 = rc;
			rc2.right = rc.left + (LONG)length;
			dc.ExtTextOut(rc.left + borders[2], top, ETO_CLIPPED, &rc2, buf.c_str(), buf.size(), NULL);
			

			dc.SetTextColor(WinUtil::textColor);
			rc2 = rc;
			rc2.left = rc.left + (LONG)length;
			dc.ExtTextOut(rc.left + borders[2], top, ETO_CLIPPED, &rc2, buf.c_str(), buf.size(), NULL);
			
			dc.Detach();
		}	
	} else if(dis->CtlType == ODT_MENU) {
		bHandled = TRUE;
		return OMenu::onDrawItem(hwnd, uMsg, wParam, lParam, bHandled);
	}
	
	return S_OK;
}


BOOL SearchFrame::ListMeasure( MEASUREITEMSTRUCT *mis) {
	mis->itemHeight = 16;
	return TRUE;
}


BOOL SearchFrame::ListDraw(DRAWITEMSTRUCT *dis) {
	TCHAR szText[MAX_PATH+1];
	
	switch(dis->itemAction) {
		case ODA_FOCUS:
			if(!(dis->itemState & 0x0200))
				DrawFocusRect(dis->hDC, &dis->rcItem);
			break;

		case ODA_SELECT:
		case ODA_DRAWENTIRE:
			ctrlFiletype.GetLBText(dis->itemID, szText);
			if(dis->itemState & ODS_SELECTED) {
				SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
			} else {			
				SetTextColor(dis->hDC, WinUtil::textColor);
				SetBkColor(dis->hDC, WinUtil::bgColor);
			}

			ExtTextOut(dis->hDC, dis->rcItem.left+22, dis->rcItem.top+1, ETO_OPAQUE, &dis->rcItem, szText, lstrlen(szText), 0);
			if(dis->itemState & ODS_FOCUS) {
				if(!(dis->itemState &  0x0200 ))
					DrawFocusRect(dis->hDC, &dis->rcItem);
			}

			ImageList_Draw(searchTypes, dis->itemID, dis->hDC, 
				dis->rcItem.left + 2, 
				dis->rcItem.top, 
				ILD_TRANSPARENT);

			break;
	}
	return TRUE;
}


void SearchFrame::onEnter() {
	StringList clients;
	
	// Change Default Settings If Changed
	if (onlyFree != BOOLSETTING(FREE_SLOTS_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::FREE_SLOTS_DEFAULT, onlyFree);

	if (UseSkiplist != BOOLSETTING(SEARCH_SKIPLIST))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_SKIPLIST, UseSkiplist);

	
		TCHAR *buf = new TCHAR[ctrlSkiplist.GetWindowTextLength()+1];
		ctrlSkiplist.GetWindowText(buf, ctrlSkiplist.GetWindowTextLength()+1);
		SettingsManager::getInstance()->set(SettingsManager::SKIPLIST_SEARCH, Text::fromT(buf));
		delete[] buf;
	

	if (expandSR != BOOLSETTING(EXPAND_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::EXPAND_DEFAULT, expandSR);
	if (!initialType && ctrlFiletype.GetCurSel() != SETTING(LAST_SEARCH_FILETYPE))
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_FILETYPE, ctrlFiletype.GetCurSel());

	int n = ctrlHubs.GetItemCount();
	for(int i = 1; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(Text::fromT(ctrlHubs.getItemData(i)->url));
		}
	}

	if(!clients.size())
		return;

	tstring s(ctrlSearch.GetWindowTextLength() + 1, _T('\0'));
	ctrlSearch.GetWindowText(&s[0], s.size());
	s.resize(s.size()-1);

	tstring size(ctrlSize.GetWindowTextLength() + 1, _T('\0'));
	ctrlSize.GetWindowText(&size[0], size.size());
	size.resize(size.size()-1);
		
	double lsize = Util::toDouble(Text::fromT(size));
	switch(ctrlSizeMode.GetCurSel()) {
		case 1:
			lsize*=1024.0; break;
		case 2:
			lsize*=1024.0*1024.0; break;
		case 3:
			lsize*=1024.0*1024.0*1024.0; break;
	}

	int64_t llsize = (int64_t)lsize;

	// delete all results which came in paused state
	for_each(pausedResults.begin(), pausedResults.end(), DeleteFunction());
	pausedResults.clear();

	ctrlResults.deleteAllItems();	
	
	::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), TRUE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
	
	{
		Lock l(cs);
		search = StringTokenizer<tstring>(s, ' ').getTokens();
		s.clear();
		//strip out terms beginning with -
		for(TStringList::iterator si = search.begin(); si != search.end(); ) {
			if(si->empty()) {
				si = search.erase(si);
				continue;
			}
			if ((*si)[0] != _T('-')) 
				s += *si + _T(' ');	
			++si;
		}

		s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);
		token = Util::toString(Util::rand());
	}
	
	if(s.empty())
		return;

	SearchManager::SizeModes mode((SearchManager::SizeModes)ctrlMode.GetCurSel());
	if(llsize == 0)
		mode = SearchManager::SIZE_DONTCARE;

	int ftype = ctrlFiletype.GetCurSel();
	exactSize1 = (mode == SearchManager::SIZE_EXACT);
	exactSize2 = llsize;		

	ctrlStatus.SetText(3, _T(""));
	ctrlStatus.SetText(4, _T(""));
	target = s;
	::InvalidateRect(m_hWndStatusBar, NULL, TRUE);

	droppedResults = 0;
	resultsCount = 0;
	running = true;

	isHash = (ftype == SearchManager::TYPE_TTH);

	// Add new searches to the last-search dropdown list
	if(SettingsManager::getInstance()->addSearchToHistory(s)) {
		if(ctrlSearchBox.GetCount() > SETTING(SEARCH_HISTORY)) 
			ctrlSearchBox.DeleteString(ctrlSearchBox.GetCount()-1);
		ctrlSearchBox.InsertString(0, s.c_str());
	}
	
	SetWindowText((TSTRING(SEARCH) + _T(" - ") + s).c_str());
	
	// stop old search
	ClientManager::getInstance()->cancelSearch((void*)this);	

	// Get ADC searchtype extensions if any is selected
	StringList extList;
	try {
		if(ftype == SearchManager::TYPE_ANY) {
			// Custom searchtype
			//Todo in AirDC++
			// disabled with current GUI extList = SettingsManager::getInstance()->getExtensions(Text::fromT(fileType->getText()));
		} else if(ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) {
			// Predefined searchtype
			extList = SettingsManager::getInstance()->getExtensions(string(1, '0' + ftype));
		}
	} catch(const SearchTypeException&) {
		ftype = SearchManager::TYPE_ANY;
	}

	{
		Lock l(cs);
		
		searchStartTime = GET_TICK();
		// more 5 seconds for transfering results
		searchEndTime = searchStartTime + SearchManager::getInstance()->search(clients, Text::fromT(s), llsize, 
			(SearchManager::TypeModes)ftype, mode, token, extList, Search::MANUAL, (void*)this) + 5000;

		waiting = true;
	}
	
	ctrlStatus.SetText(2, (TSTRING(TIME_LEFT) + _T(" ") + Util::formatSeconds((searchEndTime - searchStartTime) / 1000)).c_str());

	if(BOOLSETTING(CLEAR_SEARCH)) // Only clear if the search was sent
		ctrlSearch.SetWindowText(_T(""));

}

void SearchFrame::on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept {
	// Check that this is really a relevant search result...
	{
		Lock l(cs);

		if(search.empty()) {
			return;
		}

		if(!aResult->getToken().empty() && token != aResult->getToken()) {
			droppedResults++;
			PostMessage(WM_SPEAKER, FILTER_RESULT);
			return;
		}
		
		if(isHash) {
			if(aResult->getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(search[0])) != aResult->getTTH()) {
				droppedResults++;
				PostMessage(WM_SPEAKER, FILTER_RESULT);
				return;
			}
		} else {
			// match all here
			for(TStringIter j = search.begin(); j != search.end(); ++j) {
				if((*j->begin() != _T('-') && Util::findSubString(aResult->getFile(), Text::fromT(*j)) == -1) ||
					(*j->begin() == _T('-') && j->size() != 1 && Util::findSubString(aResult->getFile(), Text::fromT(j->substr(1))) != -1)
					) 
				{
					droppedResults++;
					PostMessage(WM_SPEAKER, FILTER_RESULT);
					return;
				}
	

			}
		}
	}

	if(UseSkiplist && Wildcard::patternMatch(aResult->getFileName(), SETTING(SKIPLIST_SEARCH), '|')) {
		droppedResults++;
		PostMessage(WM_SPEAKER, FILTER_RESULT);
		return;
	}

	// Reject results without free slots
	if((onlyFree && aResult->getFreeSlots() < 1) ||
	   (exactSize1 && (aResult->getSize() != exactSize2)))
	{
		droppedResults++;
		PostMessage(WM_SPEAKER, FILTER_RESULT);
		return;
	}


	SearchInfo* i = new SearchInfo(aResult);
	PostMessage(WM_SPEAKER, ADD_RESULT, (LPARAM)i);
}

void SearchFrame::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	Lock l(cs);
	
	if(waiting) {
		if(aTick < searchEndTime + 1000){
			TCHAR buf[64];
			_stprintf(buf, _T("%s %s"), CTSTRING(TIME_LEFT), Util::formatSeconds(searchEndTime > aTick ? (searchEndTime - aTick) / 1000 : 0).c_str());
			PostMessage(WM_SPEAKER, QUEUE_STATS, (LPARAM)new tstring(buf));		
		}
	
		if(aTick > searchEndTime) {
			waiting = false;
		}
	}
}

void SearchFrame::SearchInfo::view() {
	try {
		if(sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->add(Util::getTempPath() + sr->getFileName(),
				sr->getSize(), sr->getTTH(), HintedUser(sr->getUser(), sr->getHubURL()),
				QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::viewNfo() {
	string path = Util::getDir(Text::fromT(getText(COLUMN_PATH)), true, false);
	boost::regex reg;
	reg.assign("(.+\\.nfo)", boost::regex_constants::icase);
	if ((sr->getType() == SearchResult::TYPE_FILE) && (regex_match(sr->getFileName(), reg))) {
		try {
			QueueManager::getInstance()->add(Util::getTempPath() + sr->getFileName(),
				sr->getSize(), sr->getTTH(), HintedUser(sr->getUser(), sr->getHubURL()),
				QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
		} catch(const Exception&) {
		}
	} else {
		try {
			QueueManager::getInstance()->addList(HintedUser(sr->getUser(), sr->getHubURL()), QueueItem::FLAG_VIEW_NFO | QueueItem::FLAG_PARTIAL_LIST, path);
		} catch(const Exception&) {
			// Ignore for now...
		}
	}
}
void SearchFrame::SearchInfo::matchPartial() {
	string path = Util::getDir(Text::fromT(getText(COLUMN_PATH)), true, false);
	bool nmdc = sr->getUser()->isNMDC();
		try {
			QueueManager::getInstance()->addList(HintedUser(sr->getUser(), sr->getHubURL()), QueueItem::FLAG_MATCH_QUEUE | (nmdc? 0 : QueueItem::FLAG_RECURSIVE_LIST) | QueueItem::FLAG_PARTIAL_LIST, path);
		} catch(const Exception&) {
			//...
		}
}


void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt + si->getText(COLUMN_FILENAME));
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), HintedUser(si->sr->getUser(), si->sr->getHubURL()));
			
			const vector<SearchInfo*>& children = sf->getUserList().findChildren(si->getGroupCond());
			for(SearchInfo::Iter i = children.begin(); i != children.end(); i++) {
				SearchInfo* j = *i;
				try {
					QueueManager::getInstance()->add(Text::fromT(tgt + si->getText(COLUMN_FILENAME)), j->sr->getSize(), j->sr->getTTH(), HintedUser(j->getUser(), j->sr->getHubURL()));
				} catch(const Exception&) {
				}
			}
			if(WinUtil::isShift())
				QueueManager::getInstance()->setQIPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), HintedUser(si->sr->getUser(), si->sr->getHubURL()), Text::fromT(tgt),
				WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	try {
		QueueItem::Priority prio = WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT;
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			QueueManager::getInstance()->addDirectory(Text::fromT(si->getText(COLUMN_PATH)),
				HintedUser(si->sr->getUser(), si->sr->getHubURL()), Text::fromT(tgt), prio);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), HintedUser(si->sr->getUser(), si->sr->getHubURL()), 
				Text::fromT(tgt), prio);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	try {
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			string target = Text::fromT(tgt);
			QueueManager::getInstance()->add(target, si->sr->getSize(), 
				si->sr->getTTH(), HintedUser(si->sr->getUser(), si->sr->getHubURL()));

			if(WinUtil::isShift())
				QueueManager::getInstance()->setQIPriority(target, QueueItem::HIGHEST);
		} else {
			QueueManager::getInstance()->addDirectory(si->sr->getFile(), HintedUser(si->sr->getUser(), si->sr->getHubURL()), Text::fromT(tgt),
				WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	} catch(const Exception&) {
	}
}

void SearchFrame::SearchInfo::getList() {
	try {
		QueueManager::getInstance()->addList(HintedUser(sr->getUser(), sr->getHubURL()), QueueItem::FLAG_CLIENT_VIEW, Text::fromT(getText(COLUMN_PATH)));
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::browseList() {
	try {
		QueueManager::getInstance()->addList(HintedUser(sr->getUser(), sr->getHubURL()), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST, Text::fromT(getText(COLUMN_PATH)));
	} catch(const Exception&) {
		// Ignore for now...
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	if(firstTTH) {
		tth = si->getText(COLUMN_TTH);
		hasTTH = true;
		firstTTH = false;
	} else if(hasTTH) {
		if(tth != si->getText(COLUMN_TTH)) {
			hasTTH = false;
		}
	} 

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID(), si->sr->getHubURL());
		firstHubs = false;
	} else if(!hubs.empty()) {
		// we will merge hubs of all users to ensure we can use OP commands in all hubs
		StringList sl = ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID(), si->sr->getHubURL());
		hubs.insert( hubs.end(), sl.begin(), sl.end() );
		//Util::intersect(hubs, ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID()));
	}
}

LRESULT SearchFrame::onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		const SearchInfo* si = ctrlResults.getItemData(i);
		const SearchResultPtr& sr = si->sr;
	
		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->getText(COLUMN_FILENAME);
			if(WinUtil::browseFile(target, m_hWnd)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(target));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(WinUtil::browseDirectory(target, m_hWnd)) {
				WinUtil::addLastDir(target);
				ctrlResults.forEachSelectedT(SearchInfo::Download(target, this));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			WinUtil::addLastDir(target);
			ctrlResults.forEachSelectedT(SearchInfo::Download(target, this));
		}
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		WinUtil::addLastDir(target);
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(target));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert(wID >= IDC_DOWNLOAD_TARGET);
	size_t newId = (size_t)wID - IDC_DOWNLOAD_TARGET;

	if(newId < (int)WinUtil::lastDirs.size()) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(WinUtil::lastDirs[newId], this));
	} else {
		dcassert((newId - (int)WinUtil::lastDirs.size()) < targets.size());
		string tgt = targets[newId - (int)WinUtil::lastDirs.size()];
		if(!tgt.empty()) //if we for some reason counted wrong picking the target, dont bother to use empty string.
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(tgt)));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert((wID-IDC_DOWNLOAD_WHOLE_TARGET) < (int)WinUtil::lastDirs.size());
	ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole((WinUtil::lastDirs[wID-IDC_DOWNLOAD_WHOLE_TARGET])));
	return 0;
}

LRESULT SearchFrame::onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	dcassert(wID >= IDC_DOWNLOAD_FAVORITE_DIRS);
	int newId = wID - IDC_DOWNLOAD_FAVORITE_DIRS;

	int favShareSize = (int)FavoriteManager::getInstance()->getFavoriteDirs().size();
	if (SETTING(SHOW_SHARED_DIRS_FAV)) {
		favShareSize += (int)ShareManager::getInstance()->getGroupedDirectories().size();
	}

	if(newId < favShareSize) {
		int sel = -1;
		map<string, int64_t> countedDirs;
		while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const SearchResultPtr& sr = ctrlResults.getItemData(sel)->sr;
			auto s = countedDirs.find(sr->getFileName());
			if (s != countedDirs.end()) {
				if (s->second < sr->getSize()) {
					s->second = sr->getSize();
				}
			} else {
				countedDirs[sr->getFileName()] = sr->getSize();
			}
		}
		int64_t size = 0;
		for_each(countedDirs.begin(), countedDirs.end(), [&](pair<string, int64_t> p) { size += p.second; } );

		string target;
		if (WinUtil::getTarget(newId, target, size)) {
			ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(target), this));
		}
	} else {
		dcassert((newId - favShareSize) < (int)targets.size());
		string tgt = targets[newId - favShareSize];
		if(!tgt.empty())
		ctrlResults.forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(tgt)));
	}
	return 0;
}

LRESULT SearchFrame::onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	/* the space calculation isn't accurate because the size of the folder isn't known with files (but it works with folders) */
	int sel = -1;
	map<string, int64_t> countedDirs;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const SearchResultPtr& sr = ctrlResults.getItemData(sel)->sr;
		auto s = countedDirs.find(sr->getFileName());
		if (s != countedDirs.end()) {
			if (s->second < sr->getSize()) {
				s->second = sr->getSize();
			}
		} else {
			countedDirs[sr->getFileName()] = sr->getSize();
		}
	}
	int64_t size = 0;
	for_each(countedDirs.begin(), countedDirs.end(), [&](pair<string, int64_t> p) { size += p.second; } );

	string target;
	int newId = wID-IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS;
	if (WinUtil::getTarget(newId, target, size)) {
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(target)));
	}
	return 0;
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY)), this));
	}
	return 0;
}

LRESULT SearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(!closed) {
		ClientManager::getInstance()->cancelSearch((void*)this);
		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		SearchManager::getInstance()->removeListener(this);
 		ClientManager::getInstance()->removeListener(this);
		frames.erase(m_hWnd);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		ctrlResults.SetRedraw(FALSE);
		
		ctrlResults.deleteAllItems();
		
		ctrlResults.SetRedraw(TRUE);

		// delete all results which came in paused state
		for_each(pausedResults.begin(), pausedResults.end(), DeleteFunction());

		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			delete ctrlHubs.getItemData(i);
			}
		ctrlHubs.DeleteAllItems();

		CRect rc;
		if(!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
				
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
				
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::SEARCH_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::SEARCH_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::SEARCH_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::SEARCH_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		ctrlResults.saveHeaderOrder(SettingsManager::SEARCHFRAME_ORDER, SettingsManager::SEARCHFRAME_WIDTHS, 
			SettingsManager::SEARCHFRAME_VISIBLE);

		
		bHandled = FALSE;
	return 0;
	}
}

void SearchFrame::UpdateLayout(BOOL bResizeBars)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[5];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 420 ? 376 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
		
		w[0] = 15;
		w[1] = sr.right - tmp;
		w[2] = w[1] + (tmp-16) / 3;
		w[3] = w[2] + (tmp-16) / 3;
		w[4] = w[3] + (tmp-16) / 3;
		
		ctrlStatus.SetParts(5, w);

		// Layout showUI button in statusbar part #0
		ctrlStatus.GetRect(0, sr);
		ctrlShowUI.MoveWindow(sr);
	}

	if(showUI)
	{
		int const width = 220, spacing = 50, labelH = 16, comboH = 140, lMargin = 2, rMargin = 4;
		CRect rc = rect;

		rc.left += width;
		ctrlResults.MoveWindow(rc);

		// "Search for"
		rc.right = width - rMargin;
		rc.left = lMargin;
		rc.top += 25;
		rc.bottom = rc.top + comboH + 21;
		ctrlSearchBox.MoveWindow(rc);

		searchLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Purge"
		rc.right = rc.left + spacing;
		rc.left = lMargin;
		rc.top += 25;
		rc.bottom = rc.top + 21;
		ctrlPurge.MoveWindow(rc);

		// "Size"
		int w2 = width - rMargin - lMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH;
		rc.right = w2/3;
		ctrlMode.MoveWindow(rc);

		sizeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = rc.right + lMargin;
		rc.right += w2/3;
		rc.bottom = rc.top + 21;
		ctrlSize.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		rc.bottom = rc.top + comboH;
		ctrlSizeMode.MoveWindow(rc);

		// "File type"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH + 21;
		ctrlFiletype.MoveWindow(rc);
		//rc.bottom -= comboH;

		typeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Search filter"
		rc.left = lMargin;
		rc.right = (width - rMargin) / 2 - 3;		
		rc.top += spacing;
		rc.bottom = rc.top + 21;
		
		ctrlFilter.MoveWindow(rc);

		rc.left = (width - rMargin) / 2 + 3;
		rc.right = width - rMargin;
		rc.bottom = rc.top + comboH + 21;
		ctrlFilterSel.MoveWindow(rc);
		rc.bottom -= comboH;

		srLabel.MoveWindow(lMargin * 2, rc.top - labelH, width - rMargin, labelH-1);

		// "Search options"
		rc.left = lMargin+4;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom += spacing;

		optionLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);
		ctrlSlots.MoveWindow(rc);

		rc.top += 21;
		rc.bottom += 21;
		ctrlCollapsed.MoveWindow(rc);

		//Skiplist
		rc.left = lMargin;
		rc.right = width - rMargin;		
		rc.top += spacing;
		rc.bottom = rc.top + 21;

		ctrlSkiplist.MoveWindow(rc);
		ctrlSkipBool.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Hubs"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH;
		if (rc.bottom + labelH + 21 > rect.bottom) {
			rc.bottom = rect.bottom - labelH - 21;
			if (rc.bottom < rc.top + (labelH*3)/2)
				rc.bottom = rc.top + (labelH*3)/2;
		}

		ctrlHubs.MoveWindow(rc);

		hubsLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Pause Search"
		rc.right = width - rMargin;
		rc.left = rc.right - 110;
		rc.top = rc.bottom + labelH;
		rc.bottom = rc.top + 21;
		ctrlPauseSearch.MoveWindow(rc);

		// "Search"
		rc.left = lMargin;
		rc.right = rc.left + 100;
		ctrlDoSearch.MoveWindow(rc);
	}
	else
	{
		CRect rc = rect;
		ctrlResults.MoveWindow(rc);

		rc.SetRect(0,0,0,0);
		ctrlSearchBox.MoveWindow(rc);
		ctrlMode.MoveWindow(rc);
		ctrlPurge.MoveWindow(rc);
		ctrlSize.MoveWindow(rc);
		ctrlSizeMode.MoveWindow(rc);
		ctrlFiletype.MoveWindow(rc);
		ctrlPauseSearch.MoveWindow(rc);
	}

	POINT pt;
	pt.x = 10; 
	pt.y = 10;
	HWND hWnd = ctrlSearchBox.ChildWindowFromPoint(pt);
	if(hWnd != NULL && !ctrlSearch.IsWindow() && hWnd != ctrlSearchBox.m_hWnd) {
		ctrlSearch.Attach(hWnd); 
		searchContainer.SubclassWindow(ctrlSearch.m_hWnd);
	}	
}

void SearchFrame::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const SearchResultPtr& sr = ctrlResults.getItemData(sel)->sr;

		if(!sr->getUser()->isOnline())
			continue;

		if(uc.once()) {
			if(users.find(sr->getUser()->getCID()) != users.end())
				continue;
			users.insert(sr->getUser()->getCID());
		}

		ucParams["fileFN"] = sr->getFile();
		ucParams["fileSI"] = Util::toString(sr->getSize());
		ucParams["fileSIshort"] = Util::formatBytes(sr->getSize());
		if(sr->getType() == SearchResult::TYPE_FILE) {
			ucParams["fileTR"] = sr->getTTH().toBase32();
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		StringMap tmp = ucParams;
		ClientManager::getInstance()->userCommand(HintedUser(sr->getUser(), sr->getHubURL()), uc, tmp, true);
	}
}

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;

	if(hWnd == searchLabel.m_hWnd || hWnd == sizeLabel.m_hWnd || hWnd == optionLabel.m_hWnd || hWnd == typeLabel.m_hWnd
		|| hWnd == hubsLabel.m_hWnd || hWnd == ctrlSlots.m_hWnd || hWnd == ctrlSkipBool.m_hWnd || hWnd == ctrlCollapsed.m_hWnd || hWnd == srLabel.m_hWnd) {
		::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
		::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT));
		return (LRESULT)::GetSysColorBrush(COLOR_3DFACE);
	} else {
		::SetBkColor(hDC, WinUtil::bgColor);
		::SetTextColor(hDC, WinUtil::textColor);
		return (LRESULT)WinUtil::bgBrush;
	}
}

LRESULT SearchFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	switch(wParam) {
	case VK_TAB:
		if(uMsg == WM_KEYDOWN) {
			onTab(WinUtil::isShift());
		}
		break;
	case VK_RETURN:
		if( WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt() ) {
			bHandled = FALSE;
		} else {
			if(uMsg == WM_KEYDOWN) {
				onEnter();
			}
		}
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

void SearchFrame::onTab(bool shift) {
	HWND wnds[] = {
		ctrlSearch.m_hWnd, ctrlPurge.m_hWnd, ctrlMode.m_hWnd, ctrlSize.m_hWnd, ctrlSizeMode.m_hWnd, 
		ctrlFiletype.m_hWnd, ctrlSlots.m_hWnd, ctrlCollapsed.m_hWnd, ctrlDoSearch.m_hWnd, ctrlSearch.m_hWnd, 
		ctrlResults.m_hWnd
	};
	
	HWND focus = GetFocus();
	if(focus == ctrlSearchBox.m_hWnd)
		focus = ctrlSearch.m_hWnd;
	
	static const int size = sizeof(wnds) / sizeof(wnds[0]);
	int i;
	for(i = 0; i < size; i++) {
		if(wnds[i] == focus)
			break;
	}

	::SetFocus(wnds[(i + (shift ? -1 : 1)) % size]);
}

LRESULT SearchFrame::onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		const SearchResultPtr& sr = ctrlResults.getItemData(i)->sr;
		if(sr->getType() == SearchResult::TYPE_FILE) {
			WinUtil::searchHash(sr->getTTH());
		}
	}

	return 0;
}

LRESULT SearchFrame::onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		const SearchResultPtr& sr = ctrlResults.getItemData(i)->sr;
		if(sr->getType() == SearchResult::TYPE_FILE) {
			WinUtil::bitziLink(sr->getTTH());
		}
	}
	return 0;
}

void SearchFrame::addSearchResult(SearchInfo* si) {
	const SearchResultPtr& sr = si->sr;
    // Check previous search results for dupes
	if(si->sr->getTTH().data > 0 && useGrouping) {
		SearchInfoList::ParentPair* pp = ctrlResults.findParentPair(sr->getTTH());
		if(pp) {
			if((sr->getUser()->getCID() == pp->parent->getUser()->getCID()) && (sr->getFile() == pp->parent->sr->getFile())) {	 	
				delete si;
				return;	 	
			} 	
			for(vector<SearchInfo*>::const_iterator k = pp->children.begin(); k != pp->children.end(); k++){	 	
				if((sr->getUser()->getCID() == (*k)->getUser()->getCID()) && (sr->getFile() == (*k)->sr->getFile())) {	 	
					delete si;
					return;	 	
				} 	
			}	 	
		}
	} else {
		for(SearchInfoList::ParentMap::const_iterator s = ctrlResults.getParents().begin(); s != ctrlResults.getParents().end(); ++s) {
			SearchInfo* si2 = (*s).second.parent;
	        const SearchResultPtr& sr2 = si2->sr;
			if((sr->getUser()->getCID() == sr2->getUser()->getCID()) && (sr->getFile() == sr2->getFile())) {
				delete si;	 	
				return;	 	
			}
		}	 	
    }

	if(running) {
		bool resort = false;
		resultsCount++;

		if(ctrlResults.getSortColumn() == COLUMN_HITS && resultsCount % 15 == 0) {
			resort = true;
		}

		if(si->sr->getTTH().data > 0 && useGrouping) {
			ctrlResults.insertGroupedItem(si, expandSR);
		} else {
			SearchInfoList::ParentPair pp = { si, SearchInfoList::emptyVector };
			ctrlResults.insertItem(si, si->getImageIndex());
			ctrlResults.getParents().insert(make_pair(const_cast<TTHValue*>(&sr->getTTH()), pp));
		}

		if(!filter.empty())
			updateSearchList(si);

		if (BOOLSETTING(BOLD_SEARCH)) {
			setDirty();
		}
		ctrlStatus.SetText(3, (Util::toStringW(resultsCount) + _T(" ") + TSTRING(FILES)).c_str());

		if(resort) {
			ctrlResults.resort();
		}
	} else {
		// searching is paused, so store the result but don't show it in the GUI (show only information: visible/all results)
		pausedResults.push_back(si);
		ctrlStatus.SetText(3, (Util::toStringW(resultsCount) + _T("/") + Util::toStringW(pausedResults.size() + resultsCount) + _T(" ") + WSTRING(FILES)).c_str());
	}
}

LRESULT SearchFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
 	switch(wParam) {
	case ADD_RESULT:
		addSearchResult((SearchInfo*)(lParam));
		break;
	case FILTER_RESULT:
		ctrlStatus.SetText(4, (Util::toStringW(droppedResults) + _T(" ") + TSTRING(FILTERED)).c_str());
		break;
 	case HUB_ADDED:
 			onHubAdded((HubInfo*)(lParam));
		break;
 	case HUB_CHANGED:
 			onHubChanged((HubInfo*)(lParam));
		break;
 	case HUB_REMOVED:
 			onHubRemoved((HubInfo*)(lParam));
 		break;
	case QUEUE_STATS:
	{
		tstring* t = (tstring*)(lParam);
		ctrlStatus.SetText(2, (*t).c_str());
		
		::InvalidateRect(m_hWndStatusBar, NULL, TRUE);
		delete t;
	}
		break;
	}

	return 0;
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlResults && ctrlResults.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlResults, pt);
		}
		
		if(ctrlResults.GetSelectedCount() > 0) {

			OMenu resultsMenu, targetMenu, targetDirMenu, copyMenu, SearchMenu;
			SearchInfo::CheckTTH cs = ctrlResults.forEachSelectedT(SearchInfo::CheckTTH());

			copyMenu.CreatePopupMenu();
			targetDirMenu.CreatePopupMenu();
			targetMenu.CreatePopupMenu();
			resultsMenu.CreatePopupMenu();
			SearchMenu.CreatePopupMenu();

			copyMenu.InsertSeparatorFirst(TSTRING(COPY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(DIRECTORY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));

			SearchMenu.InsertSeparatorFirst(TSTRING(SEARCH_SITES));		
			SearchMenu.AppendMenu(MF_STRING, IDC_GOOGLE_TITLE, CTSTRING(SEARCH_GOOGLE_TITLE));
			SearchMenu.AppendMenu(MF_STRING, IDC_GOOGLE_FULL, CTSTRING(SEARCH_GOOGLE_FULL));
			SearchMenu.AppendMenu(MF_STRING, IDC_TVCOM, CTSTRING(SEARCH_TVCOM));
			SearchMenu.AppendMenu(MF_STRING, IDC_IMDB, CTSTRING(SEARCH_IMDB));
			SearchMenu.AppendMenu(MF_STRING, IDC_METACRITIC, CTSTRING(SEARCH_METACRITIC));
			SearchMenu.AppendMenu(MF_STRING, IDC_BITZI_LOOKUP, CTSTRING(BITZI_LOOKUP));

			if(ctrlResults.GetSelectedCount() > 1)
				resultsMenu.InsertSeparatorFirst(Util::toStringW(ctrlResults.GetSelectedCount()) + _T(" ") + TSTRING(FILES));
			else
				resultsMenu.InsertSeparatorFirst(Util::toStringW(((SearchInfo*)ctrlResults.getSelectedItem())->hits + 1) + _T(" ") + TSTRING(USERS));
				
			resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD, CTSTRING(DOWNLOAD));
			resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetMenu, CTSTRING(DOWNLOAD_TO));
			resultsMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIR, CTSTRING(DOWNLOAD_WHOLE_DIR));
			resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)targetDirMenu, CTSTRING(DOWNLOAD_WHOLE_DIR_TO));
			resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
			resultsMenu.AppendMenu(MF_STRING, IDC_VIEW_NFO, CTSTRING(VIEW_NFO));
			resultsMenu.AppendMenu(MF_STRING, IDC_MATCH, CTSTRING(MATCH_PARTIAL));
			resultsMenu.AppendMenu(MF_SEPARATOR);
			if((ctrlResults.GetSelectedCount() == 1) && ((SearchInfo*)ctrlResults.getSelectedItem()->isShareDupe() || (SearchInfo*)ctrlResults.getSelectedItem()->isQueueDupe() ||
				(SearchInfo*)ctrlResults.getSelectedItem()->isFinishedDupe())) {
				resultsMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
				resultsMenu.AppendMenu(MF_SEPARATOR);
			}
			resultsMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES_FILE));
			resultsMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES_DIR, CTSTRING(SEARCH_FOR_ALTERNATES_DIR));
			resultsMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)SearchMenu, CTSTRING(SEARCH_SITES));
			resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
			resultsMenu.AppendMenu(MF_SEPARATOR);
			appendUserItems(resultsMenu, Util::emptyString); // TODO: hubhint
			prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hubs);
			resultsMenu.AppendMenu(MF_SEPARATOR);
			resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
			resultsMenu.SetMenuDefaultItem(IDC_DOWNLOAD);

			int n = 0;

			targetMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_TO));

			//Append shared directories
			auto sharedDirs = ShareManager::getInstance()->getGroupedDirectories();
			if (SETTING(SHOW_SHARED_DIRS_FAV)) {
				if (!sharedDirs.empty()) {
					for(auto i = sharedDirs.begin(); i != sharedDirs.end(); i++) {
						targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->first).c_str());
						n++;
					}
					targetMenu.AppendMenu(MF_SEPARATOR);
				}
			}

			//Append favorite download dirs
			auto spl = FavoriteManager::getInstance()->getFavoriteDirs();
			if (spl.size() > 0) {
				for(auto i = spl.begin(); i != spl.end(); i++) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->first).c_str());
					n++;
				}
				targetMenu.AppendMenu(MF_SEPARATOR);
			}

			n = 0;
			targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOADTO, CTSTRING(BROWSE));
			if(WinUtil::lastDirs.size() > 0) {
				targetMenu.InsertSeparatorLast(TSTRING(PREVIOUS_FOLDERS));
				for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
					targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, i->c_str());
					n++;
				}
			}

			if(cs.hasTTH) {
				targets.clear();
				targets = QueueManager::getInstance()->getTargets(TTHValue(Text::fromT(cs.tth)));

				if(targets.size() > 0) {
					targetMenu.InsertSeparatorLast(TSTRING(ADD_AS_SOURCE));
					for(StringIter i = targets.begin(); i != targets.end(); ++i) {
						targetMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_TARGET + n, Text::toT(Util::getFileName(*i)).c_str());
						n++;
					}
				}
			}

			n = 0;
			targetDirMenu.InsertSeparatorFirst(TSTRING(DOWNLOAD_WHOLE_DIR_TO));

			//Append shared directories
			if (SETTING(SHOW_SHARED_DIRS_FAV)) {
				if (!sharedDirs.empty()) {
					for(auto i = sharedDirs.begin(); i != sharedDirs.end(); i++) {
						targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_FAVORITE_DIRS + n, Text::toT(i->first).c_str());
						n++;
					}
					targetDirMenu.AppendMenu(MF_SEPARATOR);
				}
			}
			//Append favorite download dirs
			if (spl.size() > 0) {
				for(auto i = spl.begin(); i != spl.end(); ++i) {
					targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + n, Text::toT(i->first).c_str());
					n++;
				}
				targetDirMenu.AppendMenu(MF_SEPARATOR);
			}

			n = 0;
			targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOADDIRTO, CTSTRING(BROWSE));
			if(WinUtil::lastDirs.size() > 0) {
				targetDirMenu.AppendMenu(MF_SEPARATOR);
				for(TStringIter i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i) {
					targetDirMenu.AppendMenu(MF_STRING, IDC_DOWNLOAD_WHOLE_TARGET + n, i->c_str());
					n++;
				}
			}		

//			resultsMenu.InsertSeparatorFirst(Text::toT(sr->getFileName()));
			resultsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

void SearchFrame::initHubs() {
	ctrlHubs.insertItem(new HubInfo(Util::emptyStringT, TSTRING(ONLY_WHERE_OP), false), 0);
	ctrlHubs.SetCheckState(0, false);

	ClientManager* clientMgr = ClientManager::getInstance();
	clientMgr->lock();
	clientMgr->addListener(this);

	const Client::List& clients = clientMgr->getClients();

	Client::Iter it;
	Client::Iter endIt = clients.end();
	for(it = clients.begin(); it != endIt; ++it) {
		Client* client = it->second;
		if (!client->isConnected())
			continue;

		onHubAdded(new HubInfo(Text::toT(client->getHubUrl()), Text::toT(client->getHubName()), client->getMyIdentity().isOp()));
	}

	clientMgr->unlock();
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);

}

void SearchFrame::onHubAdded(HubInfo* info) {
	int nItem = ctrlHubs.insertItem(info, 0);
	ctrlHubs.SetCheckState(nItem, (ctrlHubs.GetCheckState(0) ? info->op : true));
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubChanged(HubInfo* info) {
	int nItem = 0;
	int n = ctrlHubs.GetItemCount();
	for(; nItem < n; nItem++) {
		if(ctrlHubs.getItemData(nItem)->url == info->url)
			break;
	}
	if (nItem == n)
		return;

	delete ctrlHubs.getItemData(nItem);
	ctrlHubs.SetItemData(nItem, (DWORD_PTR)info);
	ctrlHubs.updateItem(nItem);

	if (ctrlHubs.GetCheckState(0))
		ctrlHubs.SetCheckState(nItem, info->op);

	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubRemoved(HubInfo* info) {
	int nItem = 0;
	int n = ctrlHubs.GetItemCount();
	for(; nItem < n; nItem++) {
		if(ctrlHubs.getItemData(nItem)->url == info->url)
			break;
	}

	delete info;

	if (nItem == n)
		return;

	delete ctrlHubs.getItemData(nItem);
	ctrlHubs.DeleteItem(nItem);
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

LRESULT SearchFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::getList);
	return 0;
}

LRESULT SearchFrame::onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlResults.forEachSelected(&SearchInfo::browseList);
	return 0;
}

LRESULT SearchFrame::onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!running) {
		running = true;

		// readd all results which came during pause state
		while(!pausedResults.empty()) {
			// start from the end because erasing front elements from vector is not efficient
			addSearchResult(pausedResults.back());
			pausedResults.pop_back();
		}

		// update controls texts
		ctrlStatus.SetText(3, (Util::toStringW(ctrlResults.GetItemCount()) + _T(" ") + TSTRING(FILES)).c_str());			
		ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
	} else {
		running = false;
		ctrlPauseSearch.SetWindowText(CTSTRING(CONTINUE_SEARCH));
	}
	return 0;
}

LRESULT SearchFrame::onItemChangedHub(int /* idCtrl */, LPNMHDR pnmh, BOOL& /* bHandled */) {
	NMLISTVIEW* lv = (NMLISTVIEW*)pnmh;
	if(lv->iItem == 0 && (lv->uNewState ^ lv->uOldState) & LVIS_STATEIMAGEMASK) {
		if (((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) {
			for(int iItem = 0; (iItem = ctrlHubs.GetNextItem(iItem, LVNI_ALL)) != -1; ) {
				const HubInfo* client = ctrlHubs.getItemData(iItem);
				if (!client->op)
					ctrlHubs.SetCheckState(iItem, false);
			}
		}
	}

	return 0;
}

LRESULT SearchFrame::onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ctrlSearchBox.ResetContent();
	lastSearches.clear();
	SettingsManager::getInstance()->clearSearchHistory();
	return 0;
}

LRESULT SearchFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	if(ctrlResults.GetSelectedCount() == 1) {
	int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
	dcassert(pos != -1);

	if ( pos >= 0 ) {
		const SearchResultPtr& sr = ctrlResults.getItemData(pos)->sr;
		switch (wID) {
			case IDC_COPY_NICK:
				sCopy = WinUtil::getNicks(sr->getUser(), sr->getHubURL());
				break;
			case IDC_COPY_FILENAME:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					sCopy = Util::getFileName(Text::toT(sr->getFileName()));
				} else {
					sCopy = Text::toT(sr->getFileName());
				}
				break;
			case IDC_COPY_DIR:
				sCopy = Text::toT(Util::getDir(Util::getFilePath(sr->getFile()), true, true));
				break;
			case IDC_COPY_PATH:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					sCopy = ((Util::getFilePath(Text::toT(sr->getFile()))) + (Util::getFileName(Text::toT(sr->getFile()))));
				} else {
					sCopy = Text::toT(sr->getFile());
				}
				break;
			case IDC_COPY_SIZE:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					sCopy = Util::formatBytesW(sr->getSize());
				} else {
					sCopy = Text::toT("Directories have unknown size in searchframe");
				}
				break;
			case IDC_COPY_LINK:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					WinUtil::copyMagnet(sr->getTTH(), sr->getFileName(), sr->getSize());
				} else {
					sCopy = Text::toT("Directories don't have Magnet links");
				}
				break;
			case IDC_COPY_TTH:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					sCopy = Text::toT(sr->getTTH().toBase32());
				} else {
					sCopy = Text::toT("Directories don't have TTH");
				}
				break;
			default:
				dcdebug("SEARCHFRAME DON'T GO HERE\n");
				return 0;
		}
		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
	}
	}else if (ctrlResults.GetSelectedCount() > 1) {
		int xsel = -1;
		while((xsel = ctrlResults.GetNextItem(xsel, LVNI_SELECTED)) != -1) {
			
		const SearchResultPtr& sr = ctrlResults.getItemData(xsel)->sr;
			switch (wID) {
				case IDC_COPY_NICK:
					sCopy +=  WinUtil::getNicks(sr->getUser(), sr->getHubURL());
					sCopy += Text::toT("\r\n");
					break;
				case IDC_COPY_FILENAME:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						sCopy += Util::getFileName(Text::toT(sr->getFileName()));
					} else {
						sCopy += Text::toT(sr->getFileName());
					}
					sCopy += Text::toT("\r\n");
					break;
				case IDC_COPY_DIR:
					sCopy += Text::toT(Util::getDir(Util::getFilePath(sr->getFile()), true, true));
					sCopy += Text::toT("\r\n");
					break;
				case IDC_COPY_SIZE:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						sCopy += Util::formatBytesW(sr->getSize());
					} else {
						sCopy += Text::toT("Directories have unknown size in searchframe");
					}
					sCopy += Text::toT("\r\n");
					break;

				case IDC_COPY_PATH:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						sCopy += ((Util::getFilePath(Text::toT(sr->getFile()))) + (Util::getFileName(Text::toT(sr->getFile()))));
					} else {
						sCopy += Text::toT(sr->getFile());
					}
					sCopy += Text::toT("\r\n");
					break;
				case IDC_COPY_LINK:
				if(sr->getType() == SearchResult::TYPE_FILE) {
					WinUtil::copyMagnet(sr->getTTH(), sr->getFileName(), sr->getSize());
				} else {
					sCopy = Text::toT("Directories don't have Magnet links");
					sCopy += Text::toT("\r\n");
					}
					
					break;
				case IDC_COPY_TTH:
					if(sr->getType() == SearchResult::TYPE_FILE) {
					sCopy += Text::toT(sr->getTTH().toBase32());
				} else {
					sCopy += Text::toT("Directories don't have TTH");
				}
					sCopy += Text::toT("\r\n");
					break;
				
				default:
					dcdebug("SEARCHFRAME DON'T GO HERE\n");
					return 0;
			}
			if (!sCopy.empty())
				WinUtil::setClipboard(sCopy);
		}
		}
	return S_OK;
}


LRESULT SearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		cd->clrText = WinUtil::textColor;	
		SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
		
		if(BOOLSETTING(DUPE_SEARCH)) {
			if(si->isShareDupe()) {
				cd->clrText = SETTING(DUPE_COLOR);
				cd->clrTextBk = SETTING(TEXT_DUPE_BACK_COLOR);
			} else if (si->isQueueDupe()) {
				cd->clrText = SETTING(QUEUE_COLOR);
				cd->clrTextBk = SETTING(TEXT_QUEUE_BACK_COLOR);
				if(si->sr->getType() == SearchResult::TYPE_FILE) {		
					targets = QueueManager::getInstance()->getTargets(TTHValue(si->sr->getTTH().toBase32()));
				}
			} else if (si->isFinishedDupe()) {
				BYTE r, b, g;
				DWORD textColor = SETTING(QUEUE_COLOR);
				DWORD bg = SETTING(TEXT_QUEUE_BACK_COLOR);

				r = static_cast<BYTE>(( static_cast<DWORD>(GetRValue(textColor)) + static_cast<DWORD>(GetRValue(bg)) ) / 2);
				g = static_cast<BYTE>(( static_cast<DWORD>(GetGValue(textColor)) + static_cast<DWORD>(GetGValue(bg)) ) / 2);
				b = static_cast<BYTE>(( static_cast<DWORD>(GetBValue(textColor)) + static_cast<DWORD>(GetBValue(bg)) ) / 2);
					
				cd->clrText = RGB(r, g, b);
				cd->clrTextBk = bg;
			}
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(BOOLSETTING(GET_USER_COUNTRY) && (ctrlResults.findColumn(cd->iSubItem) == COLUMN_IP)) {
			CRect rc;
			SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
			ctrlResults.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			if((WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) //WinXP & WinSvr2003
				|| (WinUtil::getOsMajor() >= 6)) //Vista & Win7
			{
				SetTextColor(cd->nmcd.hdc, cd->clrText);
				DrawThemeBackground(GetWindowTheme(ctrlResults.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );
			} else {
				COLORREF color;
				if(ctrlResults.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
					if(ctrlResults.m_hWnd == ::GetFocus()) {
						color = GetSysColor(COLOR_HIGHLIGHT);
						SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
						SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
					} else {
						color = GetBkColor(cd->nmcd.hdc);
						SetBkColor(cd->nmcd.hdc, color);
					}				
				} else {
					color = WinUtil::bgColor;
					SetBkColor(cd->nmcd.hdc, WinUtil::bgColor);
					SetTextColor(cd->nmcd.hdc, cd->clrText);
				}
				HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID, 0, color));
				HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
				Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

				DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
				DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));
			}

			TCHAR buf[256];
			ctrlResults.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				WinUtil::flagImages.Draw(cd->nmcd.hdc, si->getFlagIndex(), p, LVSIL_SMALL);
				top = rc.top + (rc.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
				::ExtTextOut(cd->nmcd.hdc, rc.left + 30, top + 1, ETO_CLIPPED, rc, buf, _tcslen(buf), NULL);
				return CDRF_SKIPDEFAULT;
			}
		}		
	}

	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT SearchFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!BOOLSETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = buf;
		delete[] buf;
	
		updateSearchList();
	}

	bHandled = false;

	return 0;
}
LRESULT SearchFrame::onSkiplist(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(wParam == VK_RETURN) {
		TCHAR *buf = new TCHAR[ctrlSkiplist.GetWindowTextLength()+1];
		ctrlSkiplist.GetWindowText(buf, ctrlSkiplist.GetWindowTextLength()+1);
		SettingsManager::getInstance()->set(SettingsManager::SKIPLIST_SEARCH, Text::fromT(buf));
		delete[] buf;
	
	}

	bHandled = false;

	return 0;
}

bool SearchFrame::parseFilter(FilterModes& mode, int64_t& size) {
	tstring::size_type start = (tstring::size_type)tstring::npos;
	tstring::size_type end = (tstring::size_type)tstring::npos;
	int64_t multiplier = 1;
	
	if(filter.compare(0, 2, _T(">=")) == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("<=")) == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("==")) == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, _T("!=")) == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filter[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filter[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filter[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == tstring::npos)
		return false;
	if(filter.length() <= start)
		return false;

	if((end = Util::findSubString(filter, _T("TiB"))) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filter, _T("GiB"))) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filter, _T("MiB"))) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filter, _T("KiB"))) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filter, _T("TB"))) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filter, _T("GB"))) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filter, _T("MB"))) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filter, _T("kB"))) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filter, _T("B"))) != tstring::npos) {
		multiplier = 1;
	}


	if(end == tstring::npos) {
		end = filter.length();
	}
	
	tstring tmpSize = filter.substr(start, end-start);
	size = static_cast<int64_t>(Util::toDouble(Text::fromT(tmpSize)) * multiplier);
	
	return true;
}

bool SearchFrame::matchFilter(SearchInfo* si, int sel, bool doSizeCompare, FilterModes mode, int64_t size) {
	bool insert = false;

	if(filter.empty()) {
		insert = true;
	} else if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == si->sr->getSize()); break;
			case GREATER_EQUAL: insert = (size <=  si->sr->getSize()); break;
			case LESS_EQUAL: insert = (size >=  si->sr->getSize()); break;
			case GREATER: insert = (size < si->sr->getSize()); break;
			case LESS: insert = (size > si->sr->getSize()); break;
			case NOT_EQUAL: insert = (size != si->sr->getSize()); break;
		}
	} else {
		try {
			boost::wregex reg(filter, boost::regex_constants::icase);
			tstring s = si->getText(static_cast<uint8_t>(sel));

			insert = boost::regex_search(s.begin(), s.end(), reg);
		} catch(...) {
			insert = true;
		}
	}
	return insert;
}
	

void SearchFrame::updateSearchList(SearchInfo* si) {
	int64_t size = -1;
	FilterModes mode = NONE;

	int sel = ctrlFilterSel.GetCurSel();
	bool doSizeCompare = sel == COLUMN_SIZE && parseFilter(mode, size);

	if(si != NULL) {
//		if(!matchFilter(si, sel, doSizeCompare, mode, size))
//			ctrlResults.deleteItem(si);
	} else {
		ctrlResults.SetRedraw(FALSE);
		ctrlResults.DeleteAllItems();

		for(SearchInfoList::ParentMap::const_iterator i = ctrlResults.getParents().begin(); i != ctrlResults.getParents().end(); ++i) {
			SearchInfo* si = (*i).second.parent;
			si->collapsed = true;
			if(matchFilter(si, sel, doSizeCompare, mode, size)) {
				dcassert(ctrlResults.findItem(si) == -1);
				int k = ctrlResults.insertItem(si, si->getImageIndex());

				const vector<SearchInfo*>& children = ctrlResults.findChildren(si->getGroupCond());
				if(!children.empty()) {
					if(si->collapsed) {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);	
					} else {
						ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);	
					}
				} else {
					ctrlResults.SetItemState(k, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);	
				}
			}
		}
		ctrlResults.SetRedraw(TRUE);
	}
}

LRESULT SearchFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	filter = buf;
	delete[] buf;
	
	updateSearchList();
	
	bHandled = false;

	return 0;
}

void SearchFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlResults.GetBkColor() != WinUtil::bgColor) {
		ctrlResults.SetBkColor(WinUtil::bgColor);
		ctrlResults.SetTextBkColor(WinUtil::bgColor);
		ctrlResults.setFlickerFree(WinUtil::bgBrush);
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlResults.GetTextColor() != WinUtil::textColor) {
		ctrlResults.SetTextColor(WinUtil::textColor);
		ctrlHubs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT SearchFrame::onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
tstring searchTerm;
tstring searchTermFull;


if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		const SearchInfo* si = ctrlResults.getItemData(i);
		const SearchResultPtr& sr = si->sr;

		searchTermFull = Text::toT(Util::getDir(Util::getFilePath(sr->getFile()), true, true));
		searchTerm = WinUtil::getTitle(searchTermFull);





		switch (wID) {
			case IDC_GOOGLE_TITLE:
				WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;

			case IDC_GOOGLE_FULL:
				WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTermFull))));
				break;

			case IDC_IMDB:
				WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;
			case IDC_TVCOM:
				WinUtil::openLink(_T("http://www.tv.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;
			case IDC_METACRITIC:
				WinUtil::openLink(_T("http://www.metacritic.com/search/all/") + Text::toT(Util::encodeURI(Text::fromT(searchTerm)) + "/results"));
				break;
		}
}
	searchTerm = Util::emptyStringT;
	searchTermFull = Util::emptyStringT;
	return S_OK;
}

LRESULT SearchFrame::onSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int pos = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(pos != -1);

		if ( pos >= 0 ) {
			const SearchResultPtr& sr = ctrlResults.getItemData(pos)->sr;
			WinUtil::search(Text::toT(Util::getDir(Util::getFilePath(sr->getFile()), true, true)), 0, false);
		}
	}
	return S_OK;
}
LRESULT SearchFrame::onOpenDupe(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlResults.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.getSelectedItem();

		try {
			tstring path;
		
			if(si->sr->getType() == SearchResult::TYPE_FILE) {
				if (si->isShareDupe()) {
					path = Text::toT(ShareManager::getInstance()->getRealPath(si->sr->getTTH()));
				} else if (si->isQueueDupe() || si->isFinishedDupe()) {
					StringList targets = QueueManager::getInstance()->getTargets(si->sr->getTTH());
					if (!targets.empty()) {
						path = Text::toT(targets.front());
					}
				}

				if (!path.empty())
					WinUtil::openFolder(path);
			} else {
				if (si->isShareDupe()) {
					path=ShareManager::getInstance()->getDirPath(si->sr->getFile());
				} else {
					path=QueueManager::getInstance()->getDirPath(si->sr->getFile());
				}

				if (!path.empty())
					WinUtil::openFolder(path);
			}
		} catch(const ShareException& se) {
			LogManager::getInstance()->message(se.getError());
		}
	}
	return 0;
}
/**
 * @file
 * $Id: SearchFrm.cpp 500 2010-06-25 22:08:18Z bigmuscle $
 */
