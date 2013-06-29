/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "Resource.h"

#include "MainFrm.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "BarShader.h"
#include "ResourceLoader.h"

#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/SearchManager.h"
#include "../client/Localization.h"
#include "../client/DirectoryListingManager.h"
#include "../client/GeoManager.h"

#include <boost/range/numeric.hpp>


int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_HITS, COLUMN_USERS, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_IP, COLUMN_TTH, COLUMN_DATE };
int SearchFrame::columnSizes[] = { 210, 80, 100, 50, 80, 100, 40, 70, 150, 80, 100, 150, 100 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE,  ResourceManager::HITS, ResourceManager::USER, ResourceManager::TYPE, ResourceManager::SIZE,
	ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION, 
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT, ResourceManager::DATE };

SearchFrame::FrameMap SearchFrame::frames;

void SearchFrame::openWindow(const tstring& str /* = Util::emptyString */, LONGLONG size /* = 0 */, SearchManager::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, const string& type /* = SEARCH_TYPE_ANY */) {
	SearchFrame* pChild = new SearchFrame();
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);

	frames.emplace(pChild->m_hWnd, pChild);
}

void SearchFrame::closeAll() {
	for(auto f: frames | map_keys)
		::PostMessage(f, WM_CLOSE, 0, 0);
}

SearchFrame::SearchFrame() : 
searchBoxContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	searchContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
	purgeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
	sizeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
	modeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	sizeModeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	fileTypeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	showUIContainer(WC_COMBOBOX, this, SHOWUI_MESSAGE_MAP),
	slotsContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	collapsedContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	doSearchContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	resultsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
	hubsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
	ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
	ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
	ctrlExcludedContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
	initialSize(0), initialMode(SearchManager::SIZE_ATLEAST), initialType(SEARCH_TYPE_ANY),
	showUI(true), onlyFree(false), closed(false), droppedResults(0), resultsCount(0),
	expandSR(false), exactSize1(false), exactSize2(0), searchEndTime(0), searchStartTime(0), waiting(false), statusDirty(false)
{	
	SearchManager::getInstance()->addListener(this);
	useGrouping = SETTING(GROUP_SEARCH_RESULTS);
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlSearchBox.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	ctrlSearchBox.SetExtendedUI();
	searchBoxContainer.SubclassWindow(ctrlSearchBox.m_hWnd);
	
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
	
	ctrlExcludedBool.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_USE_EXCLUDED);
	ctrlExcludedBool.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlExcludedBool.SetFont(WinUtil::systemFont, FALSE);
	ctrlExcludedBool.SetWindowText(CTSTRING(EXCLUDED_WORDS_DESC));

	ctrlExcluded.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	ctrlExcluded.SetFont(WinUtil::systemFont);
	ctrlExcludedContainer.SubclassWindow(ctrlExcluded.m_hWnd);

	WinUtil::appendHistory(ctrlSearchBox, SettingsManager::HISTORY_SEARCH);
	WinUtil::appendHistory(ctrlExcluded, SettingsManager::HISTORY_EXCLUDE);

	auto pos = ctrlExcluded.FindStringExact(0, Text::toT(SETTING(LAST_SEARCH_EXCLUDED)).c_str());
	if (pos != -1)
		ctrlExcluded.SetWindowText(Text::toT(SETTING(LAST_SEARCH_EXCLUDED)).c_str());

	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	sizeModeContainer.SubclassWindow(ctrlSizeMode.m_hWnd);

	ctrlFileType.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);

	fileTypeContainer.SubclassWindow(ctrlFileType.m_hWnd);

	
	ctrlResults.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_RESULTS);

	ctrlResults.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	resultsContainer.SubclassWindow(ctrlResults.m_hWnd);
	
	ctrlResults.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	hubsContainer.SubclassWindow(ctrlHubs.m_hWnd);	

	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);

	ctrlFilterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::systemFont);

	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);

	ctrlFilterSelContainer.SubclassWindow(ctrlFilterSel.m_hWnd);
	ctrlFilterSel.SetFont(WinUtil::systemFont);

	searchLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	searchLabel.SetFont(WinUtil::systemFont, FALSE);
	searchLabel.SetWindowText(CTSTRING(SEARCH_FOR));

	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CTSTRING(SIZE));

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

	if(SETTING(FREE_SLOTS_DEFAULT)) {
		ctrlSlots.SetCheck(true);
		onlyFree = true;
	}

	if(SETTING(SEARCH_USE_EXCLUDED)) {
		ctrlExcludedBool.SetCheck(true);
	}

	if(SETTING(EXPAND_DEFAULT)) {
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
	ctrlFileType.SetFont(WinUtil::systemFont, FALSE);

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

	ctrlFileType.fillList(!initialString.empty() ? initialType : SETTING(LAST_SEARCH_FILETYPE), WinUtil::textColor, WinUtil::bgColor);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

	for(uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlResults.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlResults.setColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlResults.setVisible(SETTING(SEARCHFRAME_VISIBLE));
	
	if(SETTING(SORT_DIRS)) {
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

	lastDisabledHubs.clear();
	if(SETTING(SEARCH_SAVE_HUBS_STATE)){
		StringTokenizer<string> st(SETTING(LAST_SEARCH_DISABLED_HUBS), _T(','));
		lastDisabledHubs = st.getTokens();
	}
	initHubs();

	UpdateLayout();

	if(!initialString.empty()) {
		ctrlSearch.SetWindowText(initialString.c_str());
		ctrlMode.SetCurSel(initialMode);
		ctrlSize.SetWindowText(Util::toStringW(initialSize).c_str());
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
	WinUtil::SetIcon(m_hWnd, IDI_SEARCH);

	ctrlStatus.SetText(1, 0, SBT_OWNERDRAW);
	
	::SetTimer(m_hWnd, 0, 200, 0);
	fixControls();
	bHandled = FALSE;
	return 1;
}

LRESULT SearchFrame::onUseExcluded(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void SearchFrame::fixControls() {
	bool useExcluded = ctrlExcludedBool.GetCheck() == TRUE;
	ctrlExcluded.EnableWindow(useExcluded);
}

LRESULT SearchFrame::onMeasure(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	bHandled = FALSE;
	
	if(wParam == IDC_FILETYPES) {
		bHandled = TRUE;
		return SearchTypeCombo::onMeasureItem(uMsg, wParam, lParam, bHandled);
	} else if(((MEASUREITEMSTRUCT *)lParam)->CtlType == ODT_MENU) {
		bHandled = TRUE;
		return OMenu::onMeasureItem(hwnd, uMsg, wParam, lParam, bHandled);
	}
	
	return FALSE;
}

LRESULT SearchFrame::onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HWND hwnd = 0;
	DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
	bHandled = FALSE;

	if(wParam == IDC_FILETYPES) {
		bHandled = TRUE;
		return SearchTypeCombo::onDrawItem(uMsg, wParam, lParam, bHandled);
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

void SearchFrame::onEnter() {
	StringList clients;
	
	// Change Default Settings If Changed
	if (onlyFree != SETTING(FREE_SLOTS_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::FREE_SLOTS_DEFAULT, onlyFree);

	if (expandSR != SETTING(EXPAND_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::EXPAND_DEFAULT, expandSR);

	if(SETTING(SEARCH_SAVE_HUBS_STATE)){
		lastDisabledHubs.clear();
		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			HubInfo* hub = ctrlHubs.getItemData(i);
			if(ctrlHubs.GetCheckState(i) == FALSE && i != 0)
				lastDisabledHubs.push_back(Text::fromT(hub->url));
		}
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_DISABLED_HUBS, Util::toString(",", lastDisabledHubs));
	}

	int n = ctrlHubs.GetItemCount();
	for(int i = 1; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(Text::fromT(ctrlHubs.getItemData(i)->url));
		}
	}

	if(!clients.size())
		return;

	/*tstring s(ctrlSearch.GetWindowTextLength() + 1, _T('\0'));
	ctrlSearch.GetWindowText(&s[0], s.size());
	s.resize(s.size()-1);
	if(s.empty())
		return;*/

	string s = WinUtil::addHistory(ctrlSearchBox, SettingsManager::HISTORY_SEARCH);
	if (s.empty())
		return;

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

	ctrlResults.SetRedraw(FALSE);
	ctrlResults.deleteAllItems();	
	ctrlResults.SetRedraw(TRUE);

	::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), TRUE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));

	string excluded;
	usingExcludes = ctrlExcludedBool.GetCheck() == TRUE;
	SettingsManager::getInstance()->set(SettingsManager::SEARCH_USE_EXCLUDED, usingExcludes);
	if (usingExcludes) {
		excluded = WinUtil::addHistory(ctrlExcluded, SettingsManager::HISTORY_EXCLUDE);
		if (!excluded.empty()) {
			SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_EXCLUDED, excluded);
		}
	}

	/*{
		Lock l(cs);
		s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);
	}*/

	SearchManager::SizeModes mode((SearchManager::SizeModes)ctrlMode.GetCurSel());
	if(llsize == 0)
		mode = SearchManager::SIZE_DONTCARE;

	exactSize1 = (mode == SearchManager::SIZE_EXACT);
	exactSize2 = llsize;		

	ctrlStatus.SetText(3, _T(""));
	ctrlStatus.SetText(4, _T(""));
	target = Text::toT(s);
	::InvalidateRect(m_hWndStatusBar, NULL, TRUE);

	droppedResults = 0;
	resultsCount = 0;
	running = true;

	if (ctrlSearchBox.GetCount())
		ctrlSearchBox.SetCurSel(0);
	SetWindowText((TSTRING(SEARCH) + _T(" - ") + target).c_str());
	
	// stop old search
	ClientManager::getInstance()->cancelSearch((void*)this);	



	// Get ADC search type extensions if any is selected
	StringList extList;
	int ftype=0;
	string typeName;

	try {
		SearchManager::getInstance()->getSearchType(ctrlFileType.GetCurSel(), ftype, extList, typeName);
	} catch(const SearchTypeException&) {
		ftype = SearchManager::TYPE_ANY;
	}

	if(ftype == SearchManager::TYPE_TTH) {
		s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }), s.end());
		if(s.size() != 39 || !Encoder::isBase32(s.c_str())) {
			ctrlStatus.SetText(1, CTSTRING(INVALID_TTH_SEARCH));
			return;
		}
	}
	ctrlStatus.SetText(1, 0, SBT_OWNERDRAW);

	if (initialString.empty() && typeName != SETTING(LAST_SEARCH_FILETYPE))
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_FILETYPE, typeName);



	// perform the search
	auto newSearch = AdcSearch::getSearch(s, excluded, exactSize2, ftype, mode, extList, AdcSearch::MATCH_FULL_PATH, false);
	if (newSearch) {
		WLock l(cs);
		curSearch.reset(newSearch);
		token = Util::toString(Util::rand());

		searchStartTime = GET_TICK();
		// more 5 seconds for transferring results
		searchEndTime = searchStartTime + SearchManager::getInstance()->search(clients, s, llsize, 
			(SearchManager::TypeModes)ftype, mode, token, extList, AdcSearch::parseSearchString(excluded), Search::MANUAL, (void*)this) + 5000;

		waiting = true;
	}
	


	ctrlStatus.SetText(2, (TSTRING(TIME_LEFT) + _T(" ") + Util::formatSecondsW((searchEndTime - searchStartTime) / 1000)).c_str());

	if(SETTING(CLEAR_SEARCH)) // Only clear if the search was sent
		ctrlSearch.SetWindowText(_T(""));

}

void SearchFrame::on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept {
	if(!aResult->getToken().empty()) {
		if (token != aResult->getToken()) {
			return;
		}

		if (curSearch && curSearch->itemType == AdcSearch::TYPE_FILE && aResult->getType() != SearchResult::TYPE_FILE) {
			callAsync([this] { onResultFiltered(); });
			return;
		}

		//no further validation, trust that the other client knows what he's sending... unless we are using excludes
		if (usingExcludes) {
			RLock l (cs);
			if (curSearch && curSearch->isExcluded(aResult->getFile())) {
				callAsync([this] { onResultFiltered(); });
				return;
			}
		}
	} else {
		// Check that this is really a relevant search result...
		RLock l(cs);
		if (!curSearch)
			return;

		bool valid = true;
		if (aResult->getType() == SearchResult::TYPE_DIRECTORY) {
			if (!curSearch->matchesDirectory(aResult->getFile())) {
				valid = false;
			}
		} else {
			if (!(curSearch->hasRoot ? curSearch->root == aResult->getTTH() : curSearch->matchesFileLower(Text::toLower(aResult->getFile()), aResult->getSize(), aResult->getDate()))) {
				valid = false;
			}
		}

		if (!valid) {
			callAsync([this] { onResultFiltered(); });
			return;
		}
	}

	// Reject results without free slots
	if((onlyFree && aResult->getFreeSlots() < 1) ||
	   (exactSize1 && (aResult->getSize() != exactSize2)))
	{
		callAsync([this] { onResultFiltered(); });
		return;
	}


	SearchInfo* i = new SearchInfo(aResult);
	callAsync([=] { addSearchResult(i); });
}

void SearchFrame::removeSelected() {
	int i = -1;
	WLock l(cs);
	while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlResults.removeGroupedItem(ctrlResults.getItemData(i));
	}
}

void SearchFrame::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	RLock l(cs);
	
	if(waiting) {
		if(aTick < searchEndTime + 1000){
			auto text = TSTRING(TIME_LEFT) + _T(" ") + Util::formatSecondsW(searchEndTime > aTick ? (searchEndTime - aTick) / 1000 : 0);
			callAsync([=] {
				ctrlStatus.SetText(2, text.c_str());
				::InvalidateRect(m_hWndStatusBar, NULL, TRUE);
			});
		}
	
		if(aTick > searchEndTime) {
			waiting = false;
		}
	}
}

SearchFrame::SearchInfo::SearchInfo(const SearchResultPtr& aSR) : sr(aSR), collapsed(true), parent(NULL), flagIndex(0), hits(0), dupe(DUPE_NONE) { 
	//check the dupe
	if(SETTING(DUPE_SEARCH)) {
		if (sr->getType() == SearchResult::TYPE_DIRECTORY)
			dupe = AirUtil::checkDirDupe(sr->getFile(), sr->getSize());
		else
			dupe = SettingsManager::lanMode ? AirUtil::checkFileDupe(sr->getFile(), sr->getSize()) : AirUtil::checkFileDupe(sr->getTTH(), sr->getFileName());
	}

	//get the ip info
	string ip = sr->getIP();
	if (!ip.empty()) {
		// Only attempt to grab a country mapping if we actually have an IP address
		string tmpCountry = GeoManager::getInstance()->getCountry(sr->getIP());
		if(!tmpCountry.empty()) {
			ip = tmpCountry + " (" + ip + ")";
			flagIndex = Localization::getFlagIndexByCode(tmpCountry.c_str());
		}
	}
	ipText = Text::toT(ip);
}


int SearchFrame::SearchInfo::getImageIndex() const {
	return sr->getType() == SearchResult::TYPE_FILE ? ResourceLoader::getIconIndex(Text::toT(sr->getFile())) : ResourceLoader::DIR_NORMAL;
}

int SearchFrame::SearchInfo::compareItems(const SearchInfo* a, const SearchInfo* b, uint8_t col) {
	if(!a->sr || !b->sr)
		return 0;
	
	switch(col) {
		// I think its nicer to sort the names too, otherwise could do it with typecolumn
		case COLUMN_FILENAME: 
			if(a->sr->getType() == b->sr->getType())
				return lstrcmpi(a->getText(COLUMN_FILENAME).c_str(), b->getText(COLUMN_FILENAME).c_str());
			else 
				return ( a->sr->getType() == SearchResult::TYPE_DIRECTORY ) ? -1 : 1;

		case COLUMN_TYPE: 
			if(a->sr->getType() == b->sr->getType())
				return lstrcmpi(a->getText(COLUMN_TYPE).c_str(), b->getText(COLUMN_TYPE).c_str());
			else
				return(a->sr->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
		/*case COLUMN_FILES: 
			if(a->sr->getType() == b->sr->getType())
				return compare(a->sr->getFileCount(), b->sr->getFileCount());
			else
				return a->sr->getType() == SearchResult::TYPE_DIRECTORY ? 1 : -1;*/

		case COLUMN_HITS: return compare(a->hits, b->hits);
		case COLUMN_SLOTS: 
			if(a->sr->getFreeSlots() == b->sr->getFreeSlots())
				return compare(a->sr->getSlots(), b->sr->getSlots());
			else
				return compare(a->sr->getFreeSlots(), b->sr->getFreeSlots());
		case COLUMN_SIZE:
		case COLUMN_EXACT_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
		/*case COLUMN_USERS:
			if (a->hits != 1 || b->hits != 1)
				return compare(a->hits, b->hits);
			else
				return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());*/
		case COLUMN_DATE: return compare(a->sr->getDate(), b->sr->getDate());
		default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

const tstring SearchFrame::SearchInfo::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_FILENAME:
			if(sr->getType() == SearchResult::TYPE_FILE) {
	    		return Text::toT(Util::getFileName(sr->getFile()));    
			} else {
				return Text::toT(sr->getFileName());
			}
		/*case COLUMN_FILES: 
			if (sr->getFileCount() >= 0)
				return TSTRING_F(X_FILES, sr->getFileCount());
			else
				return Util::emptyStringW;*/
		/*case COLUMN_USERS:
			if (hits > 1)
				return Util::toStringW(hits) + _T(' ') + TSTRING(USERS);
			else
				return WinUtil::getNicks(sr->getUser());*/
		case COLUMN_HITS: return hits == 0 ? Util::emptyStringT : Util::toStringW(hits + 1) + _T(' ') + TSTRING(USERS);
		case COLUMN_USERS: return WinUtil::getNicks(sr->getUser());
		case COLUMN_TYPE:
			if(sr->getType() == SearchResult::TYPE_FILE) {
				tstring type = Text::toT(Util::getFileExt(Text::fromT(getText(COLUMN_FILENAME))));
				if(!type.empty() && type[0] == _T('.'))
					type.erase(0, 1);
				return type;
			} else {
				return TSTRING(DIRECTORY);
			}
		case COLUMN_SIZE: 
			if(sr->getType() == SearchResult::TYPE_FILE) {
				return Util::formatBytesW(sr->getSize());
			} else {
				return sr->getSize() > 0 ? Util::formatBytesW(sr->getSize()) : Util::emptyStringT;
			}				
		case COLUMN_PATH:
			if(sr->getType() == SearchResult::TYPE_FILE) {
				return Text::toT(Util::getFilePath(sr->getFile()));
			} else {
				return Text::toT(sr->getFile());
			}
		case COLUMN_SLOTS: return Text::toT(sr->getSlotString());
		case COLUMN_CONNECTION:
			/*if (hits > 1) {
				//auto p = ctrlResults.
			} else {*/
				return Text::toT(sr->getConnectionStr());
			//}
		case COLUMN_HUB: 
			/*if (hits > 1)
				return Util::emptyStringW;
			else*/
				return WinUtil::getHubNames(sr->getUser());
		case COLUMN_EXACT_SIZE: return sr->getSize() > 0 ? Util::formatExactSize(sr->getSize()) : Util::emptyStringT;
		case COLUMN_IP: 
			/*if (hits > 1)
				return Util::emptyStringW;
			else*/
				return ipText;
		case COLUMN_TTH: return (sr->getType() == SearchResult::TYPE_FILE && !SettingsManager::lanMode) ? Text::toT(sr->getTTH().toBase32()) : Util::emptyStringT;
		case COLUMN_DATE: return Util::getDateTimeW(sr->getDate());
		default: return Util::emptyStringT;
	}
}

void SearchFrame::performAction(std::function<void (const SearchInfo* aInfo)> f, bool /*oncePerParent false*/) {
	ctrlResults.filteredForEachSelectedT([&](const SearchInfo* si) {
		/*if (si->hits > 1) {
			//perform only for the children
			const auto& children = ctrlResults.findChildren(si->getGroupCond());
			if (oncePerParent && !children.empty()) {
				f(children.front());
			} else {
				boost::for_each(children, f);
			}
		} else {*/
			//perform for the parent
			f(si);
		//}
	});
}


/* Action handlers */

void SearchFrame::handleOpenItem(bool isClientView) {
	auto open = [=](const SearchInfo* si) {
		try {
			if(si->sr->getType() == SearchResult::TYPE_FILE) {
				QueueManager::getInstance()->addOpenedItem(si->sr->getFileName(), si->sr->getSize(), si->sr->getTTH(), si->sr->getUser(), isClientView);
			}
		} catch(const Exception&) {
		}
	};

	performAction(open, true);
}

void SearchFrame::handleViewNfo() {
	auto viewNfo = [=](const SearchInfo* si) {
		if (si->sr->getType() == SearchResult::TYPE_FILE && Util::getFileExt(si->sr->getFileName()) == ".nfo") {
			try {
				QueueManager::getInstance()->addOpenedItem(si->sr->getFileName(), si->sr->getSize(), si->sr->getTTH(), si->sr->getUser(), true);
			} catch(const Exception&) {
				// Ignore for now...
			}
		} else {
			string path = Util::getReleaseDir(si->sr->getFilePath(), false);
			try {
				QueueManager::getInstance()->addList(si->sr->getUser(), QueueItem::FLAG_VIEW_NFO | QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_RECURSIVE_LIST, path);
			} catch(const Exception&) {
				// Ignore for now...
			}
		}
	};

	performAction(viewNfo, true);
}

void SearchFrame::handleDownload(const string& aTarget, QueueItemBase::Priority p, bool useWhole, TargetUtil::TargetType aTargetType, bool isSizeUnknown) {
	ctrlResults.filteredForEachSelectedT([&](const SearchInfo* si) {
		//get the target for file downloads (file names may differ for grouped results)
		string fileTarget = aTarget;
		if (!useWhole && aTarget[aTarget.length()-1] == PATH_SEPARATOR)
			fileTarget += si->sr->getFileName();

		bool fileDownload = si->sr->getType() == SearchResult::TYPE_FILE && !useWhole;
		auto download = [&](const SearchResultPtr& aSR) {
			if (fileDownload) {
				WinUtil::addFileDownload(fileTarget, aSR->getSize(), aSR->getTTH(), aSR->getUser(), aSR->getDate(), 0, p);
			} else {
				DirectoryListingManager::getInstance()->addDirectoryDownload(aSR->getFilePath(), aSR->getUser(), aTarget, aTargetType, isSizeUnknown ? ASK_USER : NO_CHECK, p);
			}
		};

		if (si->hits >= 1) {
			//perform also for the children
			SearchResultList results;

			results.push_back(si->sr);
			const auto& children = ctrlResults.findChildren(si->getGroupCond());
			for (auto si: children)
				results.push_back(si->sr);

			SearchResult::pickResults(results, SETTING(MAX_AUTO_MATCH_SOURCES));

			boost::for_each(results, download);
		} else {
			//perform for the parent
			download(si->sr);
		}
	});
}

void SearchFrame::handleGetList(bool isPartial) {
	auto getList = [&, isPartial](const SearchInfo* si) {
		try {
			QueueManager::getInstance()->addList(si->sr->getUser(), QueueItem::FLAG_CLIENT_VIEW | (isPartial ? QueueItem::FLAG_PARTIAL_LIST : 0), si->sr->getFilePath());
		} catch(const Exception&) {
			// Ignore for now...
		}
	};

	performAction(getList, true);
}

void SearchFrame::handleMatchPartial() {
	auto matchPartial = [&](const SearchInfo* si) {
		string path = Util::getReleaseDir(si->sr->getFilePath(), false);
		try {
			QueueManager::getInstance()->addList(si->sr->getUser(), QueueItem::FLAG_MATCH_QUEUE | (si->sr->getUser().user->isNMDC() ? 0 : QueueItem::FLAG_RECURSIVE_LIST) | QueueItem::FLAG_PARTIAL_LIST, path);
		} catch(const Exception&) {
			//...
		}
	};

	performAction(matchPartial, true);
}

void SearchFrame::handleSearchDir() {
	if(ctrlResults.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.getSelectedItem();
		WinUtil::searchAny(Text::toT(Util::getReleaseDir(si->sr->getFile(), true)));
	}
}

void SearchFrame::handleOpenFolder() {
	if(ctrlResults.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.getSelectedItem();
		try {
			tstring path;
			if(si->sr->getType() == SearchResult::TYPE_DIRECTORY) {
				path = Text::toT(AirUtil::getDirDupePath(si->getDupe(), si->sr->getFile()));
			} else {
				path = Text::toT(AirUtil::getDupePath(si->getDupe(), si->sr->getTTH()));
			}

			if (!path.empty())
				WinUtil::openFolder(path);
		} catch(const ShareException& se) {
			LogManager::getInstance()->message(se.getError(), LogManager::LOG_ERROR);
		}
	}
}

void SearchFrame::handleSearchTTH() {
	if(ctrlResults.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.getSelectedItem();
		if(si->sr->getType() == SearchResult::TYPE_FILE) {
			WinUtil::searchHash(si->sr->getTTH(), si->sr->getFileName(), si->sr->getSize());
		}
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
		hubs = ClientManager::getInstance()->getHubUrls(si->sr->getUser());
		firstHubs = false;
	} else if(!hubs.empty()) {
		// we will merge hubs of all users to ensure we can use OP commands in all hubs
		StringList sl = ClientManager::getInstance()->getHubUrls(si->sr->getUser());
		hubs.insert( hubs.end(), sl.begin(), sl.end() );
		//Util::intersect(hubs, ClientManager::getInstance()->getHubs(si->sr->getUser()->getCID()));
	}
}

bool SearchFrame::showDirDialog(string& fileName) {
	if(ctrlResults.GetSelectedCount() == 1) {
		int i = ctrlResults.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		const SearchInfo* si = ctrlResults.getItemData(i);
		
		if (si->sr->getType() == SearchResult::TYPE_DIRECTORY)
			return true;
		else {
			fileName = si->sr->getFileName();
			return false;
		}
	}

	return true;
}


void SearchFrame::appendDownloadItems(OMenu& aMenu, bool hasFiles) {
	aMenu.appendItem(CTSTRING(DOWNLOAD), [this] { onDownload(SETTING(DOWNLOAD_DIRECTORY), false); }, OMenu::FLAG_DEFAULT);

	auto targetMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_TO), true);
	appendDownloadTo(*targetMenu, false);
	appendPriorityMenu(aMenu, false);

	if (hasFiles) {
		aMenu.appendItem(CTSTRING(DOWNLOAD_WHOLE_DIR), [this] { onDownload(SETTING(DOWNLOAD_DIRECTORY), true); });
		auto targetMenuWhole = aMenu.createSubMenu(TSTRING(DOWNLOAD_WHOLE_DIR_TO), true);
		appendDownloadTo(*targetMenuWhole, true);
	}
}

int64_t SearchFrame::getDownloadSize(bool /*isWhole*/) {
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

	return boost::accumulate(countedDirs | map_values, (int64_t)0);
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		handleDownload(SETTING(DOWNLOAD_DIRECTORY), WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT, false, TargetUtil::TARGET_PATH, false);
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
		lastDisabledHubs.clear();
		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			HubInfo* hub = ctrlHubs.getItemData(i);
			if(ctrlHubs.GetCheckState(i) == FALSE && i != 0)
				lastDisabledHubs.push_back(Text::fromT(hub->url));

			delete hub;
		}
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_DISABLED_HUBS, Util::toString(",", lastDisabledHubs));
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
		ctrlFileType.MoveWindow(rc);
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

		//Excluded words
		rc.left = lMargin;
		rc.right = width - rMargin;		
		rc.top += spacing;
		rc.bottom = rc.top + 21;

		ctrlExcluded.MoveWindow(rc);
		ctrlExcludedBool.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

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
		ctrlFileType.MoveWindow(rc);
		ctrlPauseSearch.MoveWindow(rc);
		ctrlExcluded.MoveWindow(rc);
		ctrlExcludedBool.MoveWindow(rc);
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

	auto ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const SearchResultPtr& sr = ctrlResults.getItemData(sel)->sr;

		if(!sr->getUser().user->isOnline())
			continue;

		if(uc.once()) {
			if(users.find(sr->getUser().user->getCID()) != users.end())
				continue;
			users.insert(sr->getUser().user->getCID());
		}


		ucParams["fileFN"] = [sr] { return sr->getFile(); };
		ucParams["fileSI"] = [sr] { return Util::toString(sr->getSize()); };
		ucParams["fileSIshort"] = [sr] { return Util::formatBytes(sr->getSize()); };
		if(sr->getType() == SearchResult::TYPE_FILE) {
			ucParams["fileTR"] = [sr] { return sr->getTTH().toBase32(); };
		}
		ucParams["fileMN"] = [sr] { return WinUtil::makeMagnet(sr->getTTH(), sr->getFile(), sr->getSize()); };

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		ClientManager::getInstance()->userCommand(sr->getUser(), uc, tmp, true);
	}
}

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;

	if(hWnd == searchLabel.m_hWnd || hWnd == sizeLabel.m_hWnd || hWnd == optionLabel.m_hWnd || hWnd == typeLabel.m_hWnd
		|| hWnd == hubsLabel.m_hWnd || hWnd == ctrlSlots.m_hWnd || hWnd == ctrlExcludedBool.m_hWnd || hWnd == ctrlCollapsed.m_hWnd || hWnd == srLabel.m_hWnd) {
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
		ctrlFileType.m_hWnd, ctrlSlots.m_hWnd, ctrlCollapsed.m_hWnd, ctrlDoSearch.m_hWnd, ctrlSearch.m_hWnd, 
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

void SearchFrame::addSearchResult(SearchInfo* si) {
	const SearchResultPtr& sr = si->sr;

    // Check previous search results for dupes
	if(si->sr->getTTH().data > 0 && useGrouping && (!si->getUser()->isNMDC() || si->sr->getType() == SearchResult::TYPE_FILE)) {
		SearchInfoList::ParentPair* pp = ctrlResults.findParentPair(sr->getTTH());
		if(pp) {
			if((sr->getUser() == pp->parent->getUser()) && (sr->getFile() == pp->parent->sr->getFile())) {	 	
				delete si;
				return;	 	
			} 	
			for(auto c: pp->children){	 	
				if((sr->getUser() == c->getUser()) && (sr->getFile() == c->sr->getFile())) {	 	
					delete si;
					return;	 	
				} 	
			}	 	
		}
	} else {
		for(auto p: ctrlResults.getParents() | map_values) {
			SearchInfo* si2 = p.parent;
			const SearchResultPtr& sr2 = si2->sr;
			if((sr->getUser() == sr2->getUser()) && (sr->getFile() == sr2->getFile())) {
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

		if(si->sr->getTTH().data > 0 && useGrouping && (!si->getUser()->isNMDC() || si->sr->getType() == SearchResult::TYPE_FILE)) {
			ctrlResults.insertGroupedItem(si, expandSR);
		} else {
			SearchInfoList::ParentPair pp = { si, SearchInfoList::emptyVector };
			ctrlResults.insertItem(si, si->getImageIndex());
			ctrlResults.getParents().emplace(const_cast<TTHValue*>(&sr->getTTH()), pp);
		}

		if(!filter.empty())
			updateSearchList(si);

		if (SETTING(BOLD_SEARCH)) {
			setDirty();
		}

		if(resort) {
			ctrlResults.resort();
		}
	} else {
		// searching is paused, so store the result but don't show it in the GUI (show only information: visible/all results)
		pausedResults.push_back(si);
	}

	statusDirty = true;
}

LRESULT SearchFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (statusDirty) {
		statusDirty = false;
		if (running) {
			ctrlStatus.SetText(3, (Util::toStringW(resultsCount) + _T(" ") + TSTRING(FILES)).c_str());
		} else {
			ctrlStatus.SetText(3, (Util::toStringW(resultsCount) + _T("/") + Util::toStringW(pausedResults.size() + resultsCount) + _T(" ") + WSTRING(FILES)).c_str());
		}
	}

	return 0;
}

void SearchFrame::onResultFiltered() {
	droppedResults++;
	ctrlStatus.SetText(4, (Util::toStringW(droppedResults) + _T(" ") + TSTRING(FILTERED)).c_str());
}

void SearchFrame::on(ClientConnected, const Client* c) noexcept { 
	callAsync([=] { onHubAdded(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp())); });
}

void SearchFrame::on(ClientUpdated, const Client* c) noexcept { 
	callAsync([=] { onHubChanged(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp())); });
}

void SearchFrame::on(ClientDisconnected, const string& aHubUrl) noexcept { 
	callAsync([=] { onHubRemoved(Text::toT(aHubUrl)); });
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlResults && ctrlResults.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlResults, pt);
		}
		
		if(ctrlResults.GetSelectedCount() > 0) {
			bool hasFiles=false, hasDupes=false, hasNmdcDirsOnly=true;

			int sel = -1;
			while((sel = ctrlResults.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				SearchInfo* si = ctrlResults.getItemData(sel);
				if (si->sr->getType() == SearchResult::TYPE_FILE) {
					hasFiles = true;
					hasNmdcDirsOnly = false;
				} else if (!si->sr->getUser().user->isSet(User::NMDC)) {
					hasNmdcDirsOnly = false;
				}

				if (si->isDupe())
					hasDupes = true;
			}

			OMenu resultsMenu, copyMenu;
			SearchInfo::CheckTTH cs = ctrlResults.forEachSelectedT(SearchInfo::CheckTTH());

			copyMenu.CreatePopupMenu();
			resultsMenu.CreatePopupMenu();

			copyMenu.InsertSeparatorFirst(TSTRING(COPY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(NICK));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(DIRECTORY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));

			if(ctrlResults.GetSelectedCount() > 1)
				resultsMenu.InsertSeparatorFirst(Util::toStringW(ctrlResults.GetSelectedCount()) + _T(" ") + TSTRING(FILES));
			else
				resultsMenu.InsertSeparatorFirst(Util::toStringW(((SearchInfo*)ctrlResults.getSelectedItem())->hits + 1) + _T(" ") + TSTRING(USERS));

			targets.clear();
			if (hasFiles && cs.hasTTH) {
				targets = QueueManager::getInstance()->getTargets(TTHValue(Text::fromT(cs.tth)));
			}

			appendDownloadMenu(resultsMenu, DownloadBaseHandler::SEARCH, hasFiles, hasNmdcDirsOnly);

			resultsMenu.AppendMenu(MF_SEPARATOR);

			if (hasFiles && (!hasDupes || ctrlResults.GetSelectedCount() == 1)) {
				resultsMenu.appendItem(TSTRING(VIEW_AS_TEXT), [&] { handleOpenItem(true); });
				resultsMenu.appendItem(TSTRING(OPEN), [&] { handleOpenItem(false); });
			}

			if((ctrlResults.GetSelectedCount() == 1) && hasDupes) {
				resultsMenu.appendItem(TSTRING(OPEN_FOLDER), [&] { handleOpenFolder(); });
				resultsMenu.AppendMenu(MF_SEPARATOR);
			}

			resultsMenu.appendItem(TSTRING(VIEW_NFO), [&] { handleViewNfo(); });
			resultsMenu.appendItem(TSTRING(MATCH_PARTIAL), [&] { handleMatchPartial(); });

			resultsMenu.AppendMenu(MF_SEPARATOR);
			if (hasFiles)
				resultsMenu.appendItem(SettingsManager::lanMode ? TSTRING(SEARCH_FOR_ALTERNATES) : TSTRING(SEARCH_TTH), [&] { handleSearchTTH(); });

			resultsMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [&] { handleSearchDir(); });

			WinUtil::appendSearchMenu(resultsMenu, [=](const WebShortcut* ws) {
				performAction([=](const SearchInfo* ii) { 
					WinUtil::searchSite(ws, ii->sr->getFile()); 
				}, true);
			});

			resultsMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
			resultsMenu.AppendMenu(MF_SEPARATOR);

			appendUserItems(resultsMenu, false);
			prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, cs.hubs);
			resultsMenu.AppendMenu(MF_SEPARATOR);

			resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

			resultsMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
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
	clientMgr->lockRead();
	clientMgr->addListener(this);

	const auto& clients = clientMgr->getClients();
	for(auto c: clients | map_values) {
		if (!c->isConnected())
			continue;

		onHubAdded(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp()));
	}

	clientMgr->unlockRead();
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);

}

void SearchFrame::onHubAdded(HubInfo* info) {
	int nItem = ctrlHubs.insertItem(info, 0);
	BOOL enable = TRUE;
	if(ctrlHubs.GetCheckState(0))
		enable = info->op;
	else
		enable = lastDisabledHubs.empty() ? TRUE : find(lastDisabledHubs.begin(), lastDisabledHubs.end(), Text::fromT(info->url)) == lastDisabledHubs.end() ? TRUE : FALSE;
	ctrlHubs.SetCheckState(nItem, enable);
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

void SearchFrame::onHubRemoved(tstring&& aHubUrl) {
	int nItem = 0;
	int n = ctrlHubs.GetItemCount();
	for(; nItem < n; nItem++) {
		if(ctrlHubs.getItemData(nItem)->url == aHubUrl)
			break;
	}

	if (nItem == n)
		return;

	delete ctrlHubs.getItemData(nItem);
	ctrlHubs.DeleteItem(nItem);
	ctrlHubs.SetColumnWidth(0, LVSCW_AUTOSIZE);
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
				ctrlHubs.SetCheckState(iItem, client->op);
			}
		}
	}

	return 0;
}

LRESULT SearchFrame::onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) 
{
	ctrlSearchBox.ResetContent();
	SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_SEARCH);
	return 0;
}

LRESULT SearchFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	if (ctrlResults.GetSelectedCount() >= 1) {
		int xsel = ctrlResults.GetNextItem(-1, LVNI_SELECTED);

		for (;;) {
			const SearchResultPtr& sr = ctrlResults.getItemData(xsel)->sr;
			switch (wID) {
				case IDC_COPY_NICK:
					sCopy +=  WinUtil::getNicks(sr->getUser());
					break;
				case IDC_COPY_FILENAME:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						sCopy += Util::getFileName(Text::toT(sr->getFileName()));
					} else {
						sCopy += Text::toT(sr->getFileName());
					}
					break;
				case IDC_COPY_DIR:
					sCopy += Text::toT(Util::getReleaseDir(sr->getFile(), true));
					break;
				case IDC_COPY_SIZE:
					sCopy += Util::formatBytesW(sr->getSize());
					break;
				case IDC_COPY_PATH:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						sCopy += ((Util::getFilePath(Text::toT(sr->getFile()))) + (Util::getFileName(Text::toT(sr->getFile()))));
					} else {
						sCopy += Text::toT(sr->getFile());
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
						sCopy += Text::toT(sr->getTTH().toBase32());
					} else {
						sCopy += Text::toT("Directories don't have TTH");
					}
					break;
				default:
					dcdebug("SEARCHFRAME DON'T GO HERE\n");
					return 0;
			}

			xsel = ctrlResults.GetNextItem(xsel, LVNI_SELECTED);
			if (xsel == -1) {
				break;
			}

			sCopy += Text::toT("\r\n");
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
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
		
		if(SETTING(DUPE_SEARCH)) {
			cd->clrText = WinUtil::getDupeColor(si->getDupe());
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(SETTING(GET_USER_COUNTRY) && (ctrlResults.findColumn(cd->iSubItem) == COLUMN_IP)) {
			CRect rc;
			SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
			if (si->hits > 1)
				return CDRF_DODEFAULT;

			ctrlResults.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			SetTextColor(cd->nmcd.hdc, cd->clrText);
			DrawThemeBackground(GetWindowTheme(ctrlResults.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );

			TCHAR buf[256];
			ctrlResults.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				ResourceLoader::flagImages.Draw(cd->nmcd.hdc, si->getFlagIndex(), p, LVSIL_SMALL);
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
	if(!SETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
		ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
		filter = buf;
		delete[] buf;
	
		updateSearchList();
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

		for(auto aSI: ctrlResults.getParents() | map_values) {
			SearchInfo* si = aSI.parent;
			si->collapsed = true;
			if(matchFilter(si, sel, doSizeCompare, mode, size)) {
				dcassert(ctrlResults.findItem(si) == -1);
				int k = ctrlResults.insertItem(si, si->getImageIndex());

				const auto& children = ctrlResults.findChildren(si->getGroupCond());
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