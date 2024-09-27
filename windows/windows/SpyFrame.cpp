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

#include <windows/SpyFrame.h>
#include <windows/SearchFrm.h>
#include <windows/WinUtil.h>

#include <airdcpp/search/SearchManager.h>
#include <airdcpp/search/SearchQuery.h>
#include <airdcpp/core/localization/ResourceManager.h>

namespace wingui {
string SpyFrame::id = "SpyFrame";
int SpyFrame::columnSizes[] = { 305, 70, 85 };
int SpyFrame::columnIndexes[] = { COLUMN_STRING, COLUMN_COUNT, COLUMN_TIME };
static ResourceManager::Strings columnNames[] = { ResourceManager::SEARCH_STRING, ResourceManager::COUNT, ResourceManager::TIME };

LRESULT SpyFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlSearches.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, IDC_RESULTS);
	ctrlSearches.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	ctrlSearches.SetBkColor(WinUtil::bgColor);
	ctrlSearches.SetTextBkColor(WinUtil::bgColor);
	ctrlSearches.SetTextColor(WinUtil::textColor);

	ctrlIgnoreTth.Create(ctrlStatus.m_hWnd, rcDefault, CTSTRING(IGNORE_TTH_SEARCHES), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlIgnoreTth.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlIgnoreTth.SetFont(WinUtil::systemFont);
	ctrlIgnoreTth.SetCheck(ignoreTth);
	ignoreTthContainer.SubclassWindow(ctrlIgnoreTth.m_hWnd);

	WinUtil::splitTokens(columnIndexes, SETTING(SPYFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(SPYFRAME_WIDTHS), COLUMN_LAST);
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_COUNT) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlSearches.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlSearches.setSort(COLUMN_COUNT, ExListViewCtrl::SORT_INT, false);

	WinUtil::SetIcon(m_hWnd, IDI_SEARCHSPY);


	SearchManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	bHandled = FALSE;
	return 1;
}

LRESULT SpyFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed){
		SearchManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		bHandled = TRUE;
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlSearches, SettingsManager::SPYFRAME_ORDER, SettingsManager::SPYFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		if (ignoreTth != SETTING(SPY_FRAME_IGNORE_TTH_SEARCHES))
			SettingsManager::getInstance()->set(SettingsManager::SPY_FRAME_IGNORE_TTH_SEARCHES, ignoreTth);

		WinUtil::setButtonPressed(IDC_SEARCH_SPY, false);
		bHandled = FALSE;
		return 0;
	}
}

LRESULT SpyFrame::onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlSearches.getSortColumn()) {
		if (!ctrlSearches.isAscending())
			ctrlSearches.setSort(-1, ctrlSearches.getSortType());
		else
			ctrlSearches.setSortDirection(false);
	} else {
		if(l->iSubItem == COLUMN_COUNT) {
			ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
		} else {
			ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING);
		}
	}
	return 0;
}

void SpyFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);

		int tmp = (sr.Width()) > 616 ? 516 : ((sr.Width() > 116) ? sr.Width()-100 : 16);

		w[0] = 150;
		w[1] = sr.right - tmp - 150;
		w[2] = w[1] + (tmp-16)*1/4;
		w[3] = w[1] + (tmp-16)*2/4;
		w[4] = w[1] + (tmp-16)*3/4;
		w[5] = w[1] + (tmp-16)*4/4;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlIgnoreTth.MoveWindow(sr);
	}

	ctrlSearches.MoveWindow(&rect);
}

void SpyFrame::updateStatus() noexcept {
	ctrlStatus.SetText(2, (TSTRING(TOTAL) + _T(" ") + Util::toStringW(total)).c_str());
	ctrlStatus.SetText(4, (TSTRING(HIT_COUNT) + _T(": ") + Util::toStringW(hits)).c_str());
	double ratio = total > 0 ? ((double)hits) / (double)total : 0.0;
	ctrlStatus.SetText(5, (TSTRING(HIT_RATIO) + _T(" ") + Util::toStringW(ratio)).c_str());
}

void SpyFrame::addList(const tstring& aString) noexcept {
	int j = ctrlSearches.find(aString);
	if (j == -1) {
		TStringList a;
		a.push_back(aString);
		a.push_back(Util::toStringW(1));
		a.push_back(Text::toT(Util::formatCurrentTime()));
		ctrlSearches.insert(a);
		if (ctrlSearches.GetItemCount() > 500) {
			ctrlSearches.DeleteItem(ctrlSearches.GetItemCount() - 1);
		}
	} else {
		TCHAR tmp[32];
		ctrlSearches.GetItemText(j, COLUMN_COUNT, tmp, 32);
		ctrlSearches.SetItemText(j, COLUMN_COUNT, Util::toStringW(Util::toInt(Text::fromT(tmp)) + 1).c_str());
		ctrlSearches.GetItemText(j, COLUMN_TIME, tmp, 32);
		ctrlSearches.SetItemText(j, COLUMN_TIME, Text::toT(Util::formatCurrentTime()).c_str());
		if (ctrlSearches.getSortColumn() == COLUMN_COUNT)
			ctrlSearches.resort();
		if (ctrlSearches.getSortColumn() == COLUMN_TIME)
			ctrlSearches.resort();
	}
}

LRESULT SpyFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if (wParam == INCOMING_SEARCH) {
		auto searchInfo = (IncomingSearchInfo*)lParam;

		total++;
		if (searchInfo->hasResults) {
			hits++;
		}

		// Not thread safe, but who cares really...
		perSecond[cur]++;

		addList(searchInfo->str);
		delete searchInfo;

		updateStatus();
	} else if(wParam == TICK_AVG) {
		float* x = (float*)lParam;
		ctrlStatus.SetText(3, (TSTRING(AVERAGE) + _T(" ") + Util::toStringW(*x)).c_str());
		delete x;
	} else if (wParam == OUTGOING_RESPONSES) {
		hits++;
		updateStatus();
	}

	return 0;
}

LRESULT SpyFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlSearches && ctrlSearches.GetSelectedCount() == 1) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlSearches, pt);
		}

		int i = ctrlSearches.GetNextItem(-1, LVNI_SELECTED);

		CMenu mnu;
		mnu.CreatePopupMenu();
		mnu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));

		searchString.resize(256);
		ctrlSearches.GetItemText(i, COLUMN_STRING, &searchString[0], searchString.size());

		mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		
		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT SpyFrame::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(strnicmp(searchString.c_str(), _T("TTH:"), 4) == 0)
		SearchFrame::openWindow(searchString.substr(4), 0, Search::SIZE_DONTCARE, SEARCH_TYPE_TTH);
	else
		SearchFrame::openWindow(searchString);
	return 0;
}

void SpyFrame::on(SearchManagerListener::IncomingSearch, Client*, const OnlineUserPtr&, const SearchQuery& aQuery, const SearchResultList& aResults, bool) noexcept {
	string searchStr;
	if (aQuery.root) {
		if (ignoreTth)
			return;

		searchStr = "TTH:" + (*aQuery.root).toBase32();
	} else {
		searchStr = aQuery.include.toString();
	}

	auto info = new IncomingSearchInfo({ Text::toT(searchStr), !aResults.empty() });
	PostMessage(WM_SPEAKER, INCOMING_SEARCH, (LPARAM)info);
}

void SpyFrame::on(TimerManagerListener::Second, uint64_t) noexcept {
	float* f = new float(0.0);
	for(int i = 0; i < AVG_TIME; ++i) {
		(*f) += (float)perSecond[i];
	}
	(*f) /= static_cast<float>(AVG_TIME);
		
	cur = (cur + 1) % AVG_TIME;
	perSecond[cur] = 0;
	PostMessage(WM_SPEAKER, TICK_AVG, (LPARAM)f);
}

void SpyFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	auto refresh = false;
	if (ctrlSearches.GetBkColor() != WinUtil::bgColor) {
		ctrlSearches.SetBkColor(WinUtil::bgColor);
		ctrlSearches.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}

	if (ctrlSearches.GetTextColor() != WinUtil::textColor) {
		ctrlSearches.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (refresh) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
}