/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#include <windows/stdafx.h>
#include <windows/Resource.h>

#include <windows/frames/filelist/DirectoryListingFrm.h>
#include <windows/MainFrm.h>
#include <windows/frames/search/SearchFrm.h>
#include <windows/components/BarShader.h>
#include <windows/ResourceLoader.h>
#include <windows/util/Wildcards.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/filelist/DirectoryListingManager.h>
#include <airdcpp/util/DupeUtil.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/search/Search.h>
#include <airdcpp/search/SearchInstance.h>
#include <airdcpp/search/SearchManager.h>
#include <airdcpp/search/SearchQuery.h>
#include <airdcpp/util/text/StringTokenizer.h>
#include <airdcpp/core/timer/TimerManager.h>
#include <airdcpp/util/ValueGenerator.h>

#include <airdcpp/modules/HighlightManager.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>

#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/map.hpp>

namespace wingui {
string SearchFrame::id = "Search";
int SearchFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_RELEVANCE, COLUMN_HITS, COLUMN_USERS, COLUMN_TYPE, COLUMN_SIZE,
	COLUMN_DATE, COLUMN_PATH, COLUMN_SLOTS, COLUMN_CONNECTION, 
	COLUMN_HUB, COLUMN_EXACT_SIZE, COLUMN_IP, COLUMN_TTH };
int SearchFrame::columnSizes[] = { 210, 50, 50, 100, 60, 80, 
	100, 100, 40, 80, 
	150, 80, 110, 150 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::RELEVANCE, ResourceManager::HIT_COUNT, ResourceManager::USER, ResourceManager::TYPE_CONTENT, ResourceManager::SIZE,
	ResourceManager::DATE, ResourceManager::PATH, ResourceManager::SLOTS, ResourceManager::CONNECTION,
	ResourceManager::HUB, ResourceManager::EXACT_SIZE, ResourceManager::IP_BARE, ResourceManager::TTH_ROOT };

static SettingsManager::BoolSetting filterSettings [] = { SettingsManager::FILTER_SEARCH_SHARED, SettingsManager::FILTER_SEARCH_QUEUED, SettingsManager::FILTER_SEARCH_INVERSED, SettingsManager::FILTER_SEARCH_TOP, 
	SettingsManager::FILTER_SEARCH_PARTIAL_DUPES, SettingsManager::FILTER_SEARCH_RESET_CHANGE };

static ColumnType columnTypes[] = { COLUMN_TEXT, COLUMN_NUMERIC_OTHER, COLUMN_NUMERIC_OTHER, COLUMN_TEXT, COLUMN_TEXT, COLUMN_SIZE, 
	COLUMN_TIME, COLUMN_TEXT, COLUMN_NUMERIC_OTHER, COLUMN_SPEED, 
	COLUMN_TEXT, COLUMN_SIZE, COLUMN_TEXT, COLUMN_TEXT };


SearchFrame::FrameMap SearchFrame::frames;

void SearchFrame::openWindow(const tstring& str /* = Util::emptyString */, LONGLONG size /* = 0 */, Search::SizeModes mode /* = SearchManager::SIZE_ATLEAST */, const string& type /* = SEARCH_TYPE_ANY */) {
	auto instance = SearchManager::getInstance()->createSearchInstance(WinUtil::ownerId);

	SearchFrame* pChild = new SearchFrame(instance);
	pChild->setInitial(str, size, mode, type);
	pChild->CreateEx(WinUtil::mdiClient);

	frames.emplace(pChild->m_hWnd, pChild);
}

void SearchFrame::closeAll() {
	for(auto f: frames | views::keys)
		::PostMessage(f, WM_CLOSE, 0, 0);
}

SearchFrame::SearchFrame(const SearchInstancePtr& aSearch) :
	search(aSearch),
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
	dateContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP),
	dateUnitContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
	excludedBoolContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	excludedContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP),
	aschContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
	initialType(SEARCH_TYPE_ANY),
	ctrlResults(this, COLUMN_LAST, [this] { updateSearchList(); }, filterSettings, COLUMN_LAST)
{	
	search->addListener(this);
}

SearchFrame::~SearchFrame() {
	SearchManager::getInstance()->removeSearchInstance(search->getToken());
}

void SearchFrame::createColumns() {
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(SEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SEARCHFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_EXACT_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlResults.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j, columnTypes[j]);
	}
}

size_t SearchFrame::getTotalListItemCount() const {
	return resultsCount;
}

LRESULT SearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
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
	
	ctrlSizeMode.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	modeContainer.SubclassWindow(ctrlSizeMode.m_hWnd);

	ctrlSize.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	sizeContainer.SubclassWindow(ctrlSize.m_hWnd);

	ctrlDate.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE);
	dateContainer.SubclassWindow(ctrlDate.m_hWnd);

	ctrlDateUnit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	dateUnitContainer.SubclassWindow(ctrlDateUnit.m_hWnd);
	
	ctrlExcludedBool.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_USE_EXCLUDED);
	ctrlExcludedBool.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlExcludedBool.SetFont(WinUtil::systemFont, FALSE);
	ctrlExcludedBool.SetWindowText(CTSTRING(EXCLUDED_WORDS_DESC));
	excludedBoolContainer.SubclassWindow(ctrlExcludedBool.m_hWnd);

	ctrlExcluded.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	ctrlExcluded.SetFont(WinUtil::systemFont);
	excludedContainer.SubclassWindow(ctrlExcluded.m_hWnd);

	ActionUtil::appendHistory(ctrlSearchBox, SettingsManager::HISTORY_SEARCH);
	ActionUtil::appendHistory(ctrlExcluded, SettingsManager::HISTORY_EXCLUDE);

	auto pos = ctrlExcluded.FindStringExact(0, Text::toT(SETTING(LAST_SEARCH_EXCLUDED)).c_str());
	if (pos != -1)
		ctrlExcluded.SetWindowText(Text::toT(SETTING(LAST_SEARCH_EXCLUDED)).c_str());

	ctrlSizeUnit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	sizeModeContainer.SubclassWindow(ctrlSizeUnit.m_hWnd);

	ctrlFileType.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED, WS_EX_CLIENTEDGE, IDC_FILETYPES);

	fileTypeContainer.SubclassWindow(ctrlFileType.m_hWnd);

	ctrlResults.Create(m_hWnd);
	resultsContainer.SubclassWindow(ctrlResults.list.m_hWnd);
	ctrlResults.list.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_NOCOLUMNHEADER, WS_EX_CLIENTEDGE, IDC_HUB);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	hubsContainer.SubclassWindow(ctrlHubs.m_hWnd);	

	searchLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	searchLabel.SetFont(WinUtil::systemFont, FALSE);
	searchLabel.SetWindowText(CTSTRING(SEARCH_FOR));

	sizeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	sizeLabel.SetFont(WinUtil::systemFont, FALSE);
	sizeLabel.SetWindowText(CTSTRING(SIZE));

	typeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	typeLabel.SetFont(WinUtil::systemFont, FALSE);
	typeLabel.SetWindowText(CTSTRING(FILE_TYPE));

	optionLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	optionLabel.SetFont(WinUtil::systemFont, FALSE);
	optionLabel.SetWindowText(CTSTRING(SEARCH_OPTIONS));

	hubsLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	hubsLabel.SetFont(WinUtil::systemFont, FALSE);
	hubsLabel.SetWindowText(CTSTRING(HUBS));

	dateLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	dateLabel.SetFont(WinUtil::systemFont, FALSE);
	dateLabel.SetWindowText(CTSTRING(MAXIMUM_AGE));

	ctrlSlots.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FREESLOTS);
	ctrlSlots.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlSlots.SetFont(WinUtil::systemFont, FALSE);
	ctrlSlots.SetWindowText(CTSTRING(ONLY_FREE_SLOTS));
	slotsContainer.SubclassWindow(ctrlSlots.m_hWnd);

	ctrlRequireAsch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_ASCH_ONLY);
	ctrlRequireAsch.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlRequireAsch.SetFont(WinUtil::systemFont, FALSE);
	ctrlRequireAsch.EnableWindow(FALSE);

	aschLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	aschLabel.SetFont(WinUtil::systemFont, FALSE);

	auto label = TSTRING(SEARCH_SUPPORTED_ONLY) + _T(" *");
	ctrlRequireAsch.SetWindowText(label.c_str());
	aschContainer.SubclassWindow(ctrlRequireAsch.m_hWnd);

	ctrlCollapsed.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_COLLAPSED);
	ctrlCollapsed.SetButtonStyle(BS_AUTOCHECKBOX, FALSE);
	ctrlCollapsed.SetFont(WinUtil::systemFont, FALSE);
	ctrlCollapsed.SetWindowText(CTSTRING(EXPANDED_RESULTS));
	collapsedContainer.SubclassWindow(ctrlCollapsed.m_hWnd);

	if(SETTING(FREE_SLOTS_DEFAULT)) {
		ctrlSlots.SetCheck(true);
		search->setFreeSlotsOnly(true);
	}

	if(SETTING(SEARCH_USE_EXCLUDED)) {
		ctrlExcludedBool.SetCheck(true);
	}

	if(SETTING(EXPAND_DEFAULT)) {
		ctrlCollapsed.SetCheck(true);
		expandSR = true;
	}

	if (SETTING(SEARCH_ASCH_ONLY)) {
		ctrlRequireAsch.SetCheck(true);
		aschOnly = true;
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
	ctrlSizeMode.SetFont(WinUtil::systemFont, FALSE);
	ctrlSizeUnit.SetFont(WinUtil::systemFont, FALSE);
	ctrlFileType.SetFont(WinUtil::systemFont, FALSE);
	ctrlDate.SetFont(WinUtil::systemFont, FALSE);
	ctrlDateUnit.SetFont(WinUtil::systemFont, FALSE);
	
	ActionUtil::appendSizeCombos(ctrlSizeUnit, ctrlSizeMode);
	ActionUtil::appendDateUnitCombo(ctrlDateUnit, 1);

	ctrlFileType.fillList(!initialString.empty() ? initialType : SETTING(LAST_SEARCH_FILETYPE), WinUtil::textColor, WinUtil::bgColor);

	ctrlResults.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);

	ctrlResults.list.setVisible(SETTING(SEARCHFRAME_VISIBLE));
	
	if(SETTING(SORT_DIRS)) {
		ctrlResults.list.setSortColumn(COLUMN_FILENAME);
	} else {
		ctrlResults.list.setSortColumn(COLUMN_RELEVANCE);
		ctrlResults.list.setAscending(false);
	}

	ctrlResults.list.SetBkColor(WinUtil::bgColor);
	ctrlResults.list.SetTextBkColor(WinUtil::bgColor);
	ctrlResults.list.SetTextColor(WinUtil::textColor);
	ctrlResults.list.SetFont(WinUtil::listViewFont, FALSE);
	ctrlResults.list.setFlickerFree(WinUtil::bgBrush);
	ctrlResults.list.addCopyHandler(COLUMN_IP, &ColumnInfo::filterCountry);
	
	ctrlHubs.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, 0, 0);
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
		ctrlSizeMode.SetCurSel(initialMode);
		ctrlSize.SetWindowText(WinUtil::toStringW(initialSize).c_str());
		SetWindowText((TSTRING(SEARCH) + _T(" - ") + initialString).c_str());
		callAsync([=] { onEnter(); });
	} else {
		SetWindowText(CTSTRING(SEARCH));
		::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), FALSE);
	}

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
	created = true;

	updateHubInfoString();
	return 1;
}

LRESULT SearchFrame::onUseExcluded(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	if (ctrlExcludedBool.GetCheck() == TRUE) { // The user just enabled, set the focus to the combobox
		ctrlExcluded.SetFocus();
	}

	return 0;
}

void SearchFrame::fixControls() {
	bool useExcluded = ctrlExcludedBool.GetCheck() == TRUE;
	ctrlExcluded.EnableWindow(useExcluded);
	ctrlExcluded.SetEditSel(-1,0); // deselect text
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


void SearchFrame::reset() {
	// delete all results which came in paused state
	for_each(pausedResults.begin(), pausedResults.end(), std::default_delete<SearchInfo>());
	pausedResults.clear();

	ctrlResults.list.SetRedraw(FALSE);
	ctrlResults.list.deleteAllItems();
	ctrlResults.list.SetRedraw(TRUE);

	::EnableWindow(GetDlgItem(IDC_SEARCH_PAUSE), TRUE);
	ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));

	ctrlStatus.SetText(3, _T(""));
	ctrlStatus.SetText(4, _T(""));

	// droppedResults = 0;
	resultsCount = 0;
}

void SearchFrame::onEnter() {
	// Change Default Settings If Changed
	if (search->getFreeSlotsOnly() != SETTING(FREE_SLOTS_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::FREE_SLOTS_DEFAULT, search->getFreeSlotsOnly());

	if (expandSR != SETTING(EXPAND_DEFAULT))
		SettingsManager::getInstance()->set(SettingsManager::EXPAND_DEFAULT, expandSR);

	if (aschOnly != SETTING(SEARCH_ASCH_ONLY))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_ASCH_ONLY, aschOnly);

	if(SETTING(SEARCH_SAVE_HUBS_STATE)){
		lastDisabledHubs.clear();
		for(int i = 0; i < ctrlHubs.GetItemCount(); i++) {
			HubInfo* hub = ctrlHubs.getItemData(i);
			if(ctrlHubs.GetCheckState(i) == FALSE && i != 0)
				lastDisabledHubs.push_back(Text::fromT(hub->url));
		}
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_DISABLED_HUBS, Util::toString(",", lastDisabledHubs));
	}

	StringList clients;
	int n = ctrlHubs.GetItemCount();
	for(int i = 1; i < n; i++) {
		if(ctrlHubs.GetCheckState(i)) {
			clients.push_back(Text::fromT(ctrlHubs.getItemData(i)->url));
		}
	}

	if(clients.empty())
		return;

	auto token = Util::toString(ValueGenerator::rand());
	auto s = make_shared<Search>(Priority::HIGH, token);

	s->query = ActionUtil::addHistory(ctrlSearchBox, SettingsManager::HISTORY_SEARCH);
	if (s->query.empty() || SearchQuery::parseSearchString(s->query).empty()) {
		ctrlStatus.SetText(1, CTSTRING(ENTER_SEARCH_STRING));
		return;
	}

	s->setLegacySize(WinUtil::parseSize(ctrlSize, ctrlSizeUnit), static_cast<Search::SizeModes>(ctrlSizeMode.GetCurSel()));

	auto date = WinUtil::parseDate(ctrlDate, ctrlDateUnit);
	if (date > 0) {
		s->minDate = date;
	}

	s->aschOnly = date > 0 && aschOnly;

	usingExcludes = ctrlExcludedBool.GetCheck() == TRUE;
	SettingsManager::getInstance()->set(SettingsManager::SEARCH_USE_EXCLUDED, usingExcludes);
	if (usingExcludes) {
		auto excluded = ActionUtil::addHistory(ctrlExcluded, SettingsManager::HISTORY_EXCLUDE);
		if (!excluded.empty()) {
			SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_EXCLUDED, excluded);
		}

		s->excluded = SearchQuery::parseSearchString(excluded);
	}

	// Get ADC search type extensions if any is selected
	string typeId;
	try {
		auto& typeManager = SearchManager::getInstance()->getSearchTypes();
		typeManager.getSearchType(ctrlFileType.GetCurSel(), s->fileType, s->exts, typeId);
	} catch(const SearchTypeException&) {
		dcassert(0);
	}

	if(s->fileType == Search::TYPE_TTH) {
		s->query.erase(std::remove_if(s->query.begin(), s->query.end(), [](char c) { 
			return c == ' ' || c == '\t' || c == '\r' || c == '\n'; 
		}), s->query.end());

		if(s->query.size() != 39 || !Encoder::isBase32(s->query.c_str())) {
			ctrlStatus.SetText(1, CTSTRING(INVALID_TTH_SEARCH));
			return;
		}
	}
	ctrlStatus.SetText(1, 0, SBT_OWNERDRAW);

	if (initialString.empty() && (typeId != SETTING(LAST_SEARCH_FILETYPE)))
		SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_FILETYPE, typeId);

	// perform the search
	search->hubSearch(clients, s);
	ctrlStatus.SetText(2, (TSTRING(TIME_LEFT) + _T(" ") + Util::formatSecondsW((searchEndTime - searchStartTime) / 1000)).c_str());
}


void SearchFrame::onSearchStarted(const string& aSearchString, uint64_t aQueueTime) noexcept {
	target = Text::toT(aSearchString);

	searchStartTime = GET_TICK();
	searchEndTime = searchStartTime + aQueueTime + 5000; // more 5 seconds for transferring results
	waiting = true;
	firstResultTime = 0;

	if (!SETTING(CLEAR_SEARCH)) {
		ctrlSearch.SetWindowText(target.c_str());
		ctrlSearch.SetSelAll();
	} else {
		ctrlSearch.SetWindowText(_T(""));
	}

	ctrlSizeMode.SetCurSel(initialMode);
	ctrlSize.SetWindowText(WinUtil::toStringW(initialSize).c_str());
	SetWindowText((TSTRING(SEARCH) + _T(" - ") + target).c_str());
	::InvalidateRect(m_hWndStatusBar, NULL, TRUE);

	running = true; 
	initialString = target;

	SetWindowText((TSTRING(SEARCH) + _T(" - ") + target).c_str());
}

LRESULT SearchFrame::onEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL & bHandled) {
	if (hWndCtl == ctrlDate.m_hWnd) {
		ctrlRequireAsch.EnableWindow(ctrlDate.LineLength() > 0);
	}

	bHandled = FALSE;
	return FALSE;
}

void SearchFrame::on(SearchInstanceListener::GroupedResultAdded, const GroupedSearchResultPtr& aResult) noexcept {
	auto i = new SearchInfo(aResult);
	callAsync([=] { addSearchResult(i); });
}

void SearchFrame::on(SearchInstanceListener::ChildResultAdded, const GroupedSearchResultPtr& aParent, const SearchResultPtr& aChild) noexcept {
	auto i = new SearchInfo(aParent, aChild);
	callAsync([=] { addSearchResult(i); });
}

void SearchFrame::on(SearchInstanceListener::Reset) noexcept {
	callAsync([=] { reset(); });
}


void SearchFrame::on(SearchInstanceListener::ResultFiltered) noexcept {
	callAsync([=] { statusDirty = true; });
}

void SearchFrame::on(SearchInstanceListener::HubSearchQueued, const string& /*aSearchToken*/, uint64_t aQueueTime, size_t /*aQueuedCount*/) noexcept {
	auto params = search->getCurrentParams();
	if (params) {
		callAsync([=] {
			onSearchStarted(params->query, aQueueTime);
		});
	}
}

void SearchFrame::removeSelected() {
	int i = -1;
	WLock l(cs);
	while( (i = ctrlResults.list.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlResults.list.removeGroupedItem(ctrlResults.list.getItemData(i));
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

SearchFrame::SearchInfo::SearchInfo(const GroupedSearchResultPtr& aGroupedResult, const SearchResultPtr& aSR) : groupedResult(aGroupedResult), sr(aSR) {
	setCountryInfo(FormatUtil::toCountryInfo(aSR->getIP()));
}

SearchFrame::SearchInfo::SearchInfo(const GroupedSearchResultPtr& aGroupedResult) : groupedResult(aGroupedResult) {
	setCountryInfo(FormatUtil::toCountryInfo(aGroupedResult->getBaseUserIP()));
}

const UserPtr& SearchFrame::SearchInfo::getUser() const noexcept {
	return getHintedUser().user;
}

const string& SearchFrame::SearchInfo::getHubUrl() const noexcept {
	return getHintedUser().hint;
}

StringList SearchFrame::SearchInfo::getDupePaths() const {
	if (isDirectory()) {
		return DupeUtil::getAdcDirectoryDupePaths(getDupe(), getAdcPath());
	} else {
		return DupeUtil::getFileDupePaths(getDupe(), getTTH());
	}
}

int SearchFrame::SearchInfo::getImageIndex() const {
	return !isDirectory() ? ResourceLoader::getIconIndex(Text::toT(getAdcPath())) : ResourceLoader::DIR_NORMAL;
}

int SearchFrame::SearchInfo::compareItems(const SearchInfo* a, const SearchInfo* b, uint8_t col) {
	switch(col) {
		// I think its nicer to sort the names too, otherwise could do it with typecolumn
		case COLUMN_FILENAME: 
			if (a->isDirectory() != b->isDirectory()) {
				return a->isDirectory() ? -1 : 1;
			}

			return lstrcmpi(a->getText(COLUMN_FILENAME).c_str(), b->getText(COLUMN_FILENAME).c_str());
		case COLUMN_RELEVANCE:
			return compare(a->getTotalRelevance(), b->getTotalRelevance());
		case COLUMN_TYPE: 
			if (a->isDirectory() != b->isDirectory()) {
				return a->isDirectory() ? -1 : 1;
			}

			if (a->groupedResult->isDirectory()) {
				return DirectoryContentInfo::Sort(a->getContentInfo(), b->getContentInfo());
			}

			return lstrcmpi(a->getText(COLUMN_TYPE).c_str(), b->getText(COLUMN_TYPE).c_str());
		case COLUMN_HITS: return compare(a->getHits(), b->getHits());
		case COLUMN_SLOTS: 
			if (a->getFreeSlots() != b->getFreeSlots()) {
				return compare(a->getTotalSlots(), b->getTotalSlots());
			}

			return compare(a->getFreeSlots(), b->getFreeSlots());
		case COLUMN_SIZE:
		case COLUMN_EXACT_SIZE: return compare(a->groupedResult->getSize(), b->groupedResult->getSize());
		case COLUMN_DATE: return compare(a->getDate(), b->getDate());
		default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

const tstring SearchFrame::SearchInfo::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_FILENAME: return Text::toT(getFileName());
		case COLUMN_RELEVANCE: return WinUtil::toStringW(getTotalRelevance());
		case COLUMN_HITS: return sr ? Util::emptyStringW : TSTRING_F(X_USERS, groupedResult->getHits());
		case COLUMN_USERS: return FormatUtil::getNicks(getHintedUser());
		case COLUMN_TYPE:
			if (!isDirectory()) {
				return WinUtil::formatFileType(getAdcPath());
			} else {
				return WinUtil::formatFolderContent(getContentInfo());
			}
		case COLUMN_SIZE: 
			if (!isDirectory()) {
				return Util::formatBytesW(getSize());
			} else {
				return getSize() > 0 ? Util::formatBytesW(getSize()) : Util::emptyStringT;
			}				
		case COLUMN_PATH: return Text::toT(getAdcFilePath());
		case COLUMN_SLOTS: return Text::toT(SearchResult::formatSlots(getFreeSlots(), getTotalSlots()));
		case COLUMN_CONNECTION: {
			if (sr && sr->isNMDC()) {
				return Text::toT(sr->getConnectionStr());
			}

			return Util::formatConnectionSpeedW(getConnectionNumeric());
		};
		case COLUMN_HUB: return FormatUtil::getHubNames(getHintedUser());
		case COLUMN_EXACT_SIZE: return getSize() > 0 ? Util::formatExactSizeW(getSize()) : Util::emptyStringT;
		case COLUMN_IP: return countryInfo.text;
		case COLUMN_TTH: return !isDirectory() ? Text::toT(getTTH().toBase32()) : Util::emptyStringT;
		case COLUMN_DATE: return WinUtil::formatDateTimeW(getDate());
		default: return Util::emptyStringT;
	}
}

void SearchFrame::performAction(std::function<void (const SearchInfo* aInfo)> f, bool /*oncePerParent false*/) {
	ctrlResults.list.filteredForEachSelectedT([&](const SearchInfo* si) {
		/*if (si->hits > 1) {
			//perform only for the children
			const auto& children = ctrlResults.list.findChildren(si->getGroupCond());
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
		if (!si->isDirectory()) {
			ActionUtil::openTextFile(si->getFileName(), si->getSize(), si->getTTH(), si->getHintedUser(), isClientView);
		}
	};

	performAction(open, true);
}

void SearchFrame::handleViewNfo() {
	auto viewNfo = [=](const SearchInfo* si) {
		if (si->isDirectory() && si->getUser()->isSet(User::ASCH)) {
			ActionUtil::findNfo(si->getAdcPath(), si->getHintedUser());
		}
	};

	performAction(viewNfo, true);
}

void SearchFrame::handleDownload(const string& aTarget, Priority p, bool useWhole) {
	ctrlResults.list.filteredForEachSelectedT([&](const SearchInfo* aSI) {
		bool fileDownload = !aSI->isDirectory() && !useWhole;

		// names/case sizes may differ for grouped results
		optional<string> targetName;
		auto download = [&](const SearchResultPtr& aSR) {
			if (fileDownload) {
				if (!targetName) {
					targetName = PathUtil::isDirectoryPath(aTarget) ? aTarget + aSI->getFileName() : aTarget;
				}
				ActionUtil::addFileDownload(*targetName, aSR->getSize(), aSR->getTTH(), aSR->getUser(), aSR->getDate(), 0, p);
			} else {
				if (!targetName) {
					// Only pick the directory name, different paths are always needed
					targetName = aSR->getType() == SearchResult::Type::DIRECTORY ? aSR->getFileName() : PathUtil::getAdcLastDir(aSR->getAdcFilePath());
				}

				MainFrame::getMainFrame()->addThreadedTask([=] {
					try {
						auto listData = FilelistAddData(aSR->getUser(), this, aSR->getAdcFilePath());
						DirectoryListingManager::getInstance()->addDirectoryDownloadHookedThrow(listData, *targetName, aTarget, p, DirectoryDownload::ErrorMethod::LOG);
					} catch (const Exception& e) {
						callAsync([=] {
							ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
						});
					}
				});
			}
		};

		if (!aSI->getSR()) {
			auto results = aSI->getGroupedResult()->pickDownloadResults();
			ranges::for_each(results, download);
		} else {
			download(aSI->getSR());
		}
	});
}

void SearchFrame::handleGetList(ListType aType) {
	auto getList = [&, aType](const SearchInfo* si) {
		int flags = aType == TYPE_PARTIAL ? QueueItem::FLAG_PARTIAL_LIST : 0;
		if (aType == TYPE_MIXED && !ActionUtil::allowGetFullList(si->getHintedUser())) {
			flags = QueueItem::FLAG_PARTIAL_LIST;
		}

		DirectoryListingFrame::openWindow(si->getHintedUser(), QueueItem::FLAG_CLIENT_VIEW | flags, si->getAdcFilePath());
	};

	performAction(getList, true);
}

void SearchFrame::handleMatchPartial() {
	auto matchPartial = [&](const SearchInfo* si) {
		MainFrame::getMainFrame()->addThreadedTask([
			user = si->getHintedUser(),
			flags = QueueItem::FLAG_MATCH_QUEUE | (si->getHintedUser().user->isNMDC() ? 0 : QueueItem::FLAG_RECURSIVE_LIST) | QueueItem::FLAG_PARTIAL_LIST,
			path = DupeUtil::getAdcReleaseDir(si->getAdcFilePath(), false),
			caller = this
		] {
			try {
				auto listData = FilelistAddData(user, caller, path);
				QueueManager::getInstance()->addListHooked(listData, flags);
			} catch (const Exception&) {
				//...
			}
		});
	};

	performAction(matchPartial, true);
}

void SearchFrame::handleSearchDir() {
	if(ctrlResults.list.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.list.getSelectedItem();
		ActionUtil::search(Text::toT(DupeUtil::getAdcReleaseDir(si->getAdcPath(), true)), true);
	}
}

void SearchFrame::handleSearchTTH() {
	if(ctrlResults.list.GetSelectedCount() == 1) {
		const SearchInfo* si = ctrlResults.list.getSelectedItem();
		if (!si->isDirectory()) {
			ActionUtil::searchHash(si->getTTH(), si->getFileName(), si->getSize());
		}
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	if(firstTTH) {
		tth = si->getTTH();
		firstTTH = false;
	} else if(tth) {
		if (tth != si->getTTH()) {
			tth.reset();
		}
	} 

	if (firstPath) {
		path = si->getAdcPath();
		firstPath = false;
	} else if (path) {
		if (DupeUtil::getAdcDirectoryName(*path).first != DupeUtil::getAdcDirectoryName(si->getAdcFilePath()).first) {
			path.reset();
		}
	}

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubUrls(si->getHintedUser());
		firstHubs = false;
	} else if(!hubs.empty()) {
		// we will merge hubs of all users to ensure we can use OP commands in all hubs
		StringList sl = ClientManager::getInstance()->getHubUrls(si->getHintedUser());
		hubs.insert(hubs.end(), sl.begin(), sl.end());
	}
}

bool SearchFrame::showDirDialog(string& fileName) {
	if(ctrlResults.list.GetSelectedCount() == 1) {
		int i = ctrlResults.list.GetNextItem(-1, LVNI_SELECTED);
		dcassert(i != -1);
		const SearchInfo* si = ctrlResults.list.getItemData(i);
		
		if (si->isDirectory())
			return true;
		else {
			fileName = si->getFileName();
			return false;
		}
	}

	return true;
}

int64_t SearchFrame::getDownloadSize(bool /*isWhole*/) {
	int sel = -1;
	map<string, int64_t> countedDirs;
	while((sel = ctrlResults.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const auto si = ctrlResults.list.getItemData(sel);
		auto s = countedDirs.find(si->getFileName());
		if (s != countedDirs.end()) {
			if (s->second < si->getSize()) {
				s->second = si->getSize();
			}
		} else {
			countedDirs[si->getFileName()] = si->getSize();
		}
	}

	return boost::accumulate(countedDirs | boost::adaptors::map_values, (int64_t)0);
}

LRESULT SearchFrame::onDoubleClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)pnmh;
	
	if (item->iItem != -1) {
		CRect rect;
		ctrlResults.list.GetItemRect(item->iItem, rect, LVIR_ICON);

		// if double click on state icon, ignore...
		if (item->ptAction.x < rect.left)
			return 0;

		onDownload(SETTING(DOWNLOAD_DIRECTORY), false, ctrlResults.list.getItemData(item->iItem)->getUser()->isNMDC(), WinUtil::isShift() ? Priority::HIGHEST : Priority::DEFAULT);
	}
	return 0;
}

LRESULT SearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(!closed) {
		ClientManager::getInstance()->cancelSearch((void*)this);
		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
 		ClientManager::getInstance()->removeListener(this);

		search->removeListener(this);

		frames.erase(m_hWnd);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		ctrlResults.list.SetRedraw(FALSE);
		
		ctrlResults.list.deleteAllItems();
		
		ctrlResults.list.SetRedraw(TRUE);

		// delete all results which came in paused state
		for_each(pausedResults.begin(), pausedResults.end(), std::default_delete<SearchInfo>());
		lastDisabledHubs.clear();
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

		ctrlResults.list.saveHeaderOrder(SettingsManager::SEARCHFRAME_ORDER, SettingsManager::SEARCHFRAME_WIDTHS, 
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

	int const width = 240, spacing = 50, labelH = 16, comboH = 140, lMargin = 2, rMargin = 4;
	if(showUI)
	{
		CRect rc = rect;
		//rc.bottom -= 26;
		//rc.left += width;
		//ctrlResults.list.MoveWindow(rc);

		// "Search for"
		rc.right = width - rMargin;
		rc.left = lMargin;
		rc.top += 25;
		rc.bottom = rc.top + comboH + 21;
		ctrlSearchBox.MoveWindow(rc);

		searchLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Purge"
		rc.left = lMargin;
		rc.right = rc.left + 100;
		rc.top += 25;
		rc.bottom = rc.top + 21;
		ctrlPurge.MoveWindow(rc);

		// "Search"
		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		ctrlDoSearch.MoveWindow(rc);

		// "Size"
		int w2 = width - rMargin - lMargin;
		rc.left = lMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH;
		rc.right = w2/3;
		ctrlSizeMode.MoveWindow(rc);

		sizeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = rc.right + lMargin;
		rc.right += w2/3;
		rc.bottom = rc.top + 21;
		ctrlSize.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = width - rMargin;
		rc.bottom = rc.top + comboH;
		ctrlSizeUnit.MoveWindow(rc);
		
		// "File type"
		rc.left = lMargin;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH + 21;
		ctrlFileType.MoveWindow(rc);
		//rc.bottom -= comboH;

		typeLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		// "Date"
		rc.left = lMargin + 4;
		rc.right = width - rMargin;
		rc.top += spacing;
		rc.bottom = rc.top + comboH;

		dateLabel.MoveWindow(rc.left, rc.top - labelH, width - rMargin, labelH - 1);

		rc.left = lMargin;
		rc.right = w2 / 2;
		rc.bottom = rc.top + 21;
		ctrlDate.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right += w2 / 2;
		//rc.bottom = rc.top + comboH;
		ctrlDateUnit.MoveWindow(rc);

		rc.left = lMargin + 4;
		rc.right = width - rMargin;
		rc.top = rc.bottom+5;
		rc.bottom = rc.top + 21;

		ctrlRequireAsch.MoveWindow(rc);

		// "Search options"
		rc.left = lMargin+4;
		rc.right = width - rMargin;
		rc.top += spacing;
		//rc.bottom += spacing;
		rc.bottom = rc.top + 17;

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
		ctrlHubs.SetColumnWidth(0, rc.Width()-4);

		hubsLabel.MoveWindow(rc.left + lMargin, rc.top - labelH, width - rMargin, labelH-1);

		rc.left = lMargin + 4;
		rc.right = width - rMargin;
		rc.top = rc.bottom + 5;
		rc.bottom = rc.top + WinUtil::getTextHeight(aschLabel.m_hWnd, WinUtil::systemFont)*2;

		aschLabel.MoveWindow(rc);

		// "Pause Search"
		rc.right = width - rMargin;
		rc.left = rc.right - 110;
		rc.top = rc.bottom + labelH;
		rc.bottom = rc.top + 21;
		ctrlPauseSearch.MoveWindow(rc);
	}
	else
	{
		CRect rc = rect;
		//rc.bottom -= 26;
		//ctrlResults.list.MoveWindow(rc);

		rc.SetRect(0,0,0,0);
		ctrlSearchBox.MoveWindow(rc);
		ctrlSizeMode.MoveWindow(rc);
		ctrlPurge.MoveWindow(rc);
		ctrlSize.MoveWindow(rc);
		ctrlSizeUnit.MoveWindow(rc);
		ctrlFileType.MoveWindow(rc);
		ctrlPauseSearch.MoveWindow(rc);
		ctrlExcluded.MoveWindow(rc);
		ctrlExcludedBool.MoveWindow(rc);
		ctrlDate.MoveWindow(rc);
		ctrlDateUnit.MoveWindow(rc);
	}

	CRect rc = rect;
	if (showUI) {
		rc.left += width;
	}
	ctrlResults.MoveWindow(&rc);


	POINT pt;
	pt.x = 10; 
	pt.y = 10;
	HWND hWnd = ctrlSearchBox.ChildWindowFromPoint(pt);
	if(hWnd != NULL && !ctrlSearch.IsWindow() && hWnd != ctrlSearchBox.m_hWnd) {
		ctrlSearch.Attach(hWnd); 
		searchContainer.SubclassWindow(ctrlSearch.m_hWnd);
	}
}

LRESULT SearchFrame::onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	// draw the background
	WTL::CDCHandle dc(reinterpret_cast<HDC>(wParam));
	RECT rc;
	GetClientRect(&rc);
	dc.FillRect(&rc, GetSysColorBrush(COLOR_3DFACE));

	// draw the borders
	HGDIOBJ oldPen = SelectObject(dc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_APPWORKSPACE)));
	//MoveToEx(dc, rc.left, rc.top, (LPPOINT) NULL);
	//LineTo(dc, rc.left, rc.top + 40);

	MoveToEx(dc, rc.left, rc.top, (LPPOINT) NULL);
	LineTo(dc, rc.right, rc.top);
	DeleteObject(SelectObject(dc, oldPen));
	return TRUE;
}

void SearchFrame::runUserCommand(UserCommand& uc) {
	if (!ActionUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = ctrlResults.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const auto si = ctrlResults.list.getItemData(sel);

		if(!si->getUser()->isOnline())
			continue;

		if(uc.once()) {
			if(users.find(si->getUser()->getCID()) != users.end())
				continue;
			users.insert(si->getUser()->getCID());
		}


		ucParams["fileFN"] = [si] { return PathUtil::toNmdcFile(si->getAdcPath()); };
		ucParams["fileSI"] = [si] { return Util::toString(si->getSize()); };
		ucParams["fileSIshort"] = [si] { return Util::formatBytes(si->getSize()); };
		if (!si->isDirectory()) {
			ucParams["fileTR"] = [si] { return si->getTTH().toBase32(); };
		}
		ucParams["fileMN"] = [si] { return ActionUtil::makeMagnet(si->getTTH(), si->getFileName(), si->getSize()); };

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		UserCommandManager::getInstance()->userCommand(si->getHintedUser(), uc, tmp, true);
	}
}

LRESULT SearchFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;

	if(hWnd == searchLabel.m_hWnd || hWnd == sizeLabel.m_hWnd || hWnd == optionLabel.m_hWnd || hWnd == typeLabel.m_hWnd
		|| hWnd == hubsLabel.m_hWnd || hWnd == ctrlSlots.m_hWnd || hWnd == ctrlExcludedBool.m_hWnd || hWnd == ctrlCollapsed.m_hWnd || hWnd == dateLabel || hWnd == ctrlRequireAsch || hWnd == aschLabel) {
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
			onTab();
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

void SearchFrame::onTab() {
	HWND wnds[] = {
		ctrlSearch.m_hWnd, ctrlPurge.m_hWnd, ctrlSizeMode.m_hWnd, ctrlSize.m_hWnd, ctrlSizeUnit.m_hWnd, 
		ctrlFileType.m_hWnd, ctrlDate.m_hWnd, ctrlDateUnit.m_hWnd, ctrlRequireAsch.m_hWnd, ctrlSlots.m_hWnd, ctrlCollapsed.m_hWnd, 
		ctrlExcludedBool.m_hWnd, ctrlExcluded.m_hWnd,
		ctrlDoSearch.m_hWnd, ctrlSearch.m_hWnd, ctrlResults.list.m_hWnd
	};
	
	HWND focus = GetFocus();
	if(focus == ctrlSearchBox.m_hWnd)
		focus = ctrlSearch.m_hWnd;
	
	static const int size = sizeof(wnds) / sizeof(wnds[0]);
	WinUtil::handleTab(focus, wnds, size);
}

void SearchFrame::addSearchResult(SearchInfo* si) {
	if (running) {
		resultsCount++;
		if (si->getTTH() && (!si->getUser()->isNMDC() || !si->isDirectory())) {
			ctrlResults.list.insertGroupedItem(si, expandSR);
		} else {
			SearchInfoList::ParentPair pp = { si, SearchInfoList::emptyVector };
			ctrlResults.list.insertItem(si, si->getImageIndex());
			ctrlResults.list.getParents().emplace(const_cast<TTHValue*>(&si->getTTH()), pp);
		}

		updateSearchList(si);

		if (resultCycleStart == 0) {
			auto tick = GET_TICK();
			resultCycleStart = tick;
			if (firstResultTime == 0) {
				firstResultTime = tick;
			}

			// only disable within 1 seconds from the first result so we can browse the received results without freezes
			if (firstResultTime + 1000 > tick) {
				ctrlResults.list.SetRedraw(FALSE);
			}

			collecting = true;
		}
		cycleResults++;
	} else {
		// searching is paused, so store the result but don't show it in the GUI (show only information: visible/all results)
		pausedResults.push_back(si);
		statusDirty = true;
	}
}

LRESULT SearchFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (collecting) {
		auto tick = GET_TICK();
		//LogManager::getInstance()->message(Util::toString((cycleTicks / cycleResults)) + " " + Util::toString(cycleTicks) + " " + Util::toString(cycleResults), LogMessage::SEV_INFO);
		if (cycleResults > 0 && ((tick - resultCycleStart) / cycleResults) <= 4) { // 250 results per second
			// keep on collecting...
			ctrlStatus.SetText(3, CTSTRING(COLLECTING_RESULTS));
			resultCycleStart = 0;
			cycleResults = 0;
			return 0;
		}

		resultCycleStart = 0;
		cycleResults = 0;

		ctrlResults.list.resort();
		if (SETTING(BOLD_SEARCH)) {
			setDirty();
		}

		ctrlResults.list.SetRedraw(TRUE);
		collecting = false;
	}

	if (statusDirty) {
		statusDirty = false;

		tstring text;
		auto curCount = ctrlResults.list.getTotalItemCount();
		if (curCount != static_cast<size_t>(resultsCount)) {
			text = WinUtil::toStringW(curCount) + _T("/") + WinUtil::toStringW(resultsCount) + _T(" ") + TSTRING(FILES);
		} else if (running || pausedResults.size() == 0) {
			text = WinUtil::toStringW(resultsCount) + _T(" ") + TSTRING(FILES);
		} else {
			text = WinUtil::toStringW(resultsCount) + _T("/") + WinUtil::toStringW(pausedResults.size() + resultsCount) + _T(" ") + WSTRING(FILES);
		}

		ctrlStatus.SetText(3, text.c_str());

		ctrlStatus.SetText(4, (WinUtil::toStringW(search->getFilteredResultCount()) + _T(" ") + TSTRING(FILTERED)).c_str());
	}

	return 0;
}

LRESULT SearchFrame::onFreeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	search->setFreeSlotsOnly(ctrlSlots.GetCheck() == 1);
	return 0;
}

void SearchFrame::on(ClientConnected, const ClientPtr& c) noexcept {
	callAsync([=] { onHubAdded(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp())); });
}

void SearchFrame::on(ClientUpdated, const ClientPtr& c) noexcept {
	callAsync([=] { onHubChanged(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp())); });
}

void SearchFrame::on(ClientDisconnected, const string& aHubUrl) noexcept { 
	callAsync([=] { onHubRemoved(Text::toT(aHubUrl)); });
}

LRESULT SearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlResults.list && ctrlResults.list.GetSelectedCount() > 0) {
		auto pt = WinUtil::getMenuPosition({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, ctrlResults.list);
		if (!pt) {
			bHandled = FALSE;
			return FALSE;
		}
		
		if(ctrlResults.list.GetSelectedCount() > 0) {
			bool hasFiles = false, hasDupes = false, hasNmdcDirsOnly = true, hasAsch = false, hasDirectories = false;

			int sel = -1;
			while((sel = ctrlResults.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				SearchInfo* si = ctrlResults.list.getItemData(sel);
				if (!si->isDirectory()) {
					hasFiles = true;
					hasNmdcDirsOnly = false;
				} else {
					hasDirectories = true;
					if (!si->getHintedUser().user->isSet(User::NMDC)) {
						hasNmdcDirsOnly = false;
					}
				}

				if (si->isDupe()) {
					hasDupes = true;
				}

				if (si->getUser()->isSet(User::ASCH)) {
					hasAsch = true;
				}
			}

			ShellMenu resultsMenu;
			SearchInfo::CheckTTH tthInfo = ctrlResults.list.forEachSelectedT(SearchInfo::CheckTTH());

			resultsMenu.CreatePopupMenu();

			if(ctrlResults.list.GetSelectedCount() > 1)
				resultsMenu.InsertSeparatorFirst(TSTRING_F(X_FILES, ctrlResults.list.GetSelectedCount()));
			else
				resultsMenu.InsertSeparatorFirst(TSTRING_F(X_USERS, ctrlResults.list.getSelectedItem()->getHits()));

			appendDownloadMenu(resultsMenu, hasFiles ? DownloadBaseHandler::TYPE_BOTH : DownloadBaseHandler::TYPE_PRIMARY, hasNmdcDirsOnly, hasFiles ? tthInfo.tth : nullopt, tthInfo.path);

			if (hasFiles && (!hasDupes || ctrlResults.list.GetSelectedCount() == 1)) {
				resultsMenu.appendSeparator();
				resultsMenu.appendItem(TSTRING(VIEW_AS_TEXT), [&] { handleOpenItem(true); });
				resultsMenu.appendItem(TSTRING(OPEN), [&] { handleOpenItem(false); });
			}

			if (hasDirectories && hasAsch) {
				resultsMenu.appendItem(TSTRING(VIEW_NFO), [&] { handleViewNfo(); });
			}
			
			resultsMenu.appendItem(TSTRING(MATCH_PARTIAL), [&] { handleMatchPartial(); });

			resultsMenu.appendSeparator();
			if (hasFiles)
				resultsMenu.appendItem(TSTRING(SEARCH_TTH), [&] { handleSearchTTH(); });

			resultsMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [&] { handleSearchDir(); });

			SearchInfoList::MenuItemList customItems {
				{ TSTRING(MAGNET_LINK), &handleCopyMagnet },
				{ TSTRING(DIRECTORY), &handleCopyDirectory }
			};

			ctrlResults.list.appendCopyMenu(resultsMenu, customItems);

			appendUserItems(resultsMenu, false);
			prepareMenu(resultsMenu, UserCommand::CONTEXT_SEARCH, tthInfo.hubs);
			resultsMenu.appendSeparator();

			{
				vector<TTHValue> tokens;
				ctrlResults.list.forEachSelectedT([&tokens](const SearchInfo* ii) {
					if (!ii->parent) {
						tokens.push_back(ii->getTTH());
					}
				});

				EXT_CONTEXT_MENU_ENTITY(resultsMenu, GroupedSearchResult, tokens, search);
			}

			resultsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

			if ((ctrlResults.list.GetSelectedCount() == 1) && hasDupes) {
				StringList localPaths;
				performAction([&](const SearchInfo* ii) {
					localPaths = ii->getDupePaths();
				});
				resultsMenu.appendShellMenu(localPaths);
			}

			resultsMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, *pt);
			return TRUE; 
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

void SearchFrame::initHubs() {
	ctrlHubs.insertItem(new HubInfo(Util::emptyStringT, TSTRING(ONLY_WHERE_OP), false), 0);
	ctrlHubs.SetCheckState(0, false);

	auto cm = ClientManager::getInstance();
	{
		RLock l(cm->getCS());
		cm->addListener(this);

		const auto& clients = cm->getClientsUnsafe();
		for (auto& c : clients | views::values) {
			if (!c->isConnected())
				continue;

			onHubAdded(new HubInfo(Text::toT(c->getHubUrl()), Text::toT(c->getHubName()), c->getMyIdentity().isOp()));
		}
	}
}

void SearchFrame::updateHubInfoString() {
	if (!created) // deadlock when the hubs are initialized
		return;

	OrderedStringSet clients;
	int n = ctrlHubs.GetItemCount();
	for (int i = 1; i < n; i++) {
		if (ctrlHubs.GetCheckState(i)) {
			clients.insert(Text::fromT(ctrlHubs.getItemData(i)->url));
		}
	}

	size_t total = 0, asch = 0;
	ClientManager::getInstance()->forEachOnlineUser([&](const OnlineUserPtr& aUser) {
		if (clients.contains(aUser->getHubUrl())) {
			total++;
			if (aUser->getUser()->isSet(User::ASCH))
				asch++;
		}
	}, true);

	tstring txt = _T("* ") + TSTRING_F(ASCH_SUPPORT_COUNT, asch % total);
	aschLabel.SetWindowText(txt.c_str());
}

void SearchFrame::onHubAdded(HubInfo* info) {
	int nItem = ctrlHubs.insertItem(info, 0);
	BOOL enable = TRUE;
	if(ctrlHubs.GetCheckState(0))
		enable = info->op;
	else
		enable = lastDisabledHubs.empty() ? TRUE : ranges::find(lastDisabledHubs, Text::fromT(info->url)) == lastDisabledHubs.end() ? TRUE : FALSE;
	ctrlHubs.SetCheckState(nItem, enable);

	updateHubInfoString();
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

	updateHubInfoString();
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
	updateHubInfoString();
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
		ctrlPauseSearch.SetWindowText(CTSTRING(PAUSE_SEARCH));
	} else {
		running = false;
		ctrlPauseSearch.SetWindowText(CTSTRING(CONTINUE_SEARCH));
	}

	statusDirty = true;
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

	updateHubInfoString();
	return 0;
}

LRESULT SearchFrame::onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlSearchBox.ResetContent();
	SettingsManager::getInstance()->clearHistory(SettingsManager::HISTORY_SEARCH);
	return 0;
}

tstring SearchFrame::handleCopyMagnet(const SearchInfo* si) {
	if (!si->isDirectory()) {
		return Text::toT(ActionUtil::makeMagnet(si->getTTH(), si->getFileName(), si->getSize()));
	} else {
		return Text::toT("Directories don't have Magnet links");
	}
}

tstring SearchFrame::handleCopyDirectory(const SearchInfo* si) {
	return Text::toT(DupeUtil::getAdcReleaseDir(si->getAdcPath(), true));
}

LRESULT SearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		cd->clrText = WinUtil::textColor;	
		SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;
		
		if(SETTING(DUPE_SEARCH)) {
			auto c = WinUtil::getDupeColors(si->getDupe());
			cd->clrText = c.first;
			cd->clrTextBk = c.second;
		}
		//has dupe color = no matching
		if (SETTING(USE_HIGHLIGHT) && !si->isDupe()) {

			ColorList *cList = HighlightManager::getInstance()->getList();
			for (ColorIter i = cList->begin(); i != cList->end(); ++i) {
				ColorSettings* colorSetting = &(*i);
				if (colorSetting->getContext() == HighlightManager::CONTEXT_SEARCH) {
					if (colorSetting->usingRegexp()) {
						try {
							//have to have $Re:
							if (boost::regex_search(Text::toT(si->getFileName()), colorSetting->regexp)) {
								if (colorSetting->getHasFgColor()) { cd->clrText = colorSetting->getFgColor(); }
								if (colorSetting->getHasBgColor()) { cd->clrTextBk = colorSetting->getBgColor(); }
								break;
							}
						}
						catch (...) {}
					} else {
						if (Wildcard::patternMatch(Text::utf8ToAcp(si->getFileName()), Text::utf8ToAcp(Text::fromT(colorSetting->getMatch())), '|')){
							if (colorSetting->getHasFgColor()) { cd->clrText = colorSetting->getFgColor(); }
							if (colorSetting->getHasBgColor()) { cd->clrTextBk = colorSetting->getBgColor(); }
							break;
						}
					}
				}
			}
		}

		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(SETTING(GET_USER_COUNTRY) && (ctrlResults.list.findColumn(cd->iSubItem) == COLUMN_IP)) {
			CRect rc;
			SearchInfo* si = (SearchInfo*)cd->nmcd.lItemlParam;

			ctrlResults.list.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

			SetTextColor(cd->nmcd.hdc, cd->clrText);
			DrawThemeBackground(GetWindowTheme(ctrlResults.list.m_hWnd), cd->nmcd.hdc, LVP_LISTITEM, 3, &rc, &rc );

			TCHAR buf[256];
			ctrlResults.list.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
			buf[255] = 0;
			if(_tcslen(buf) > 0) {
				rc.left += 2;
				LONG top = rc.top + (rc.Height() - 15)/2;
				if((top - rc.top) < 2)
					top = rc.top + 1;

				POINT p = { rc.left, top };
				ResourceLoader::flagImages.Draw(cd->nmcd.hdc, si->getCountryInfo().flagIndex, p, LVSIL_SMALL);
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

LRESULT SearchFrame::onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	//handle focus switch
	if (uMsg == WM_CHAR && wParam == VK_TAB) {
		onTab();
		return 0;
	}
	bHandled = FALSE;
	return 0;
}	

void SearchFrame::updateSearchList(SearchInfo* si) {
	auto filterInfoF = [&](int column) { return Text::fromT(si->getText(column)); };
	auto filterNumericF = [&](int column) -> double { 
		switch (column) {
			case COLUMN_RELEVANCE: return si->getTotalRelevance();
			case COLUMN_HITS: return si->getHits();
			case COLUMN_SLOTS: return si->getFreeSlots();
			case COLUMN_SIZE:
			case COLUMN_EXACT_SIZE: return si->getSize();
			case COLUMN_DATE: return si->getDate();
			case COLUMN_CONNECTION: return si->getConnectionNumeric();
			default: dcassert(0); return 0;
		}
	};
	auto filterPrep = ctrlResults.filter.prepare(filterInfoF, filterNumericF);

	if(si) {
		if (!ctrlResults.checkDupe(si->getDupe()) || (!ctrlResults.filter.empty() && !ctrlResults.filter.match(filterPrep))) {
			ctrlResults.list.deleteItem(si);
		}
	} else {
		ctrlResults.list.SetRedraw(FALSE);
		ctrlResults.list.DeleteAllItems();

		for(auto aSI: ctrlResults.list.getParents() | views::values) {
			si = aSI.parent;
			si->collapsed = true;
			if (ctrlResults.checkDupe(si->getDupe()) && (ctrlResults.filter.empty() || ctrlResults.filter.match(filterPrep))) {
				dcassert(ctrlResults.list.findItem(si) == -1);
				int k = ctrlResults.list.insertItem(si, si->getImageIndex());

				const auto& children = ctrlResults.list.findChildren(si->getGroupCond());
				if(!children.empty()) {
					if(si->collapsed) {
						ctrlResults.list.SetItemState(k, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);	
					} else {
						ctrlResults.list.SetItemState(k, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);	
					}
				} else {
					ctrlResults.list.SetItemState(k, INDEXTOSTATEIMAGEMASK(0), LVIS_STATEIMAGEMASK);	
				}
			}
		}

		ctrlResults.list.SetRedraw(TRUE);
	}

	statusDirty = true;
}

void SearchFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlResults.list.GetBkColor() != WinUtil::bgColor) {
		ctrlResults.list.SetBkColor(WinUtil::bgColor);
		ctrlResults.list.SetTextBkColor(WinUtil::bgColor);
		ctrlResults.list.setFlickerFree(WinUtil::bgBrush);
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlResults.list.GetTextColor() != WinUtil::textColor) {
		ctrlResults.list.SetTextColor(WinUtil::textColor);
		ctrlHubs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (ctrlResults.list.GetFont() != WinUtil::listViewFont){
		ctrlResults.list.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
}