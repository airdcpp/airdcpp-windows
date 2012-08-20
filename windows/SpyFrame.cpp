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

#include "SpyFrame.h"
#include "SearchFrm.h"
#include "WinUtil.h"

#include "../client/ShareManager.h"
#include "../client/ResourceManager.h"
#include "../client/ConnectionManager.h"

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

	ShareManager::getInstance()->setHits(0);

	WinUtil::SetIcon(m_hWnd, _T("searchspy.ico"));


	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	bHandled = FALSE;
	return 1;
}

LRESULT SpyFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed){
		ClientManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		bHandled = TRUE;
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlSearches, SettingsManager::SPYFRAME_ORDER, SettingsManager::SPYFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		if (ignoreTth != BOOLSETTING(SPY_FRAME_IGNORE_TTH_SEARCHES))
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
			ctrlSearches.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
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

LRESULT SpyFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == SEARCH) {
		tstring* x = (tstring*)lParam;

		total++;

		// Not thread safe, but who cares really...
		perSecond[cur]++;

		int j = ctrlSearches.find(*x);
		if(j == -1) {
			TStringList a;
			a.push_back(*x);
			a.push_back(Util::toStringW(1));
			a.push_back(Text::toT(Util::getTimeString()));			
			ctrlSearches.insert(a);
			if(ctrlSearches.GetItemCount() > 500) {
				ctrlSearches.DeleteItem(ctrlSearches.GetItemCount() - 1);
			}
		} else {
			TCHAR tmp[32];
			ctrlSearches.GetItemText(j, COLUMN_COUNT, tmp, 32);
			ctrlSearches.SetItemText(j, COLUMN_COUNT, Util::toStringW(Util::toInt(Text::fromT(tmp))+1).c_str());
			ctrlSearches.GetItemText(j, COLUMN_TIME, tmp, 32);
			ctrlSearches.SetItemText(j, COLUMN_TIME, Text::toT(Util::getTimeString()).c_str());
			if(ctrlSearches.getSortColumn() == COLUMN_COUNT )
				ctrlSearches.resort();
			if(ctrlSearches.getSortColumn() == COLUMN_TIME )
				ctrlSearches.resort();
		}
		delete x;

		ctrlStatus.SetText(2, (TSTRING(TOTAL) + _T(" ") + Util::toStringW(total)).c_str());
		ctrlStatus.SetText(4, (TSTRING(HITS) + _T(" ") + Util::toStringW(ShareManager::getInstance()->getHits())).c_str());
		double ratio = total > 0 ? ((double)ShareManager::getInstance()->getHits()) / (double)total : 0.0;
		ctrlStatus.SetText(5, (TSTRING(HIT_RATIO) + _T(" ") + Util::toStringW(ratio)).c_str());
	} else if(wParam == TICK_AVG) {
		float* x = (float*)lParam;
		ctrlStatus.SetText(3, (TSTRING(AVERAGE) + _T(" ") + Util::toStringW(*x)).c_str());
		delete x;
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
		SearchFrame::openWindow(searchString.substr(4), 0, SearchManager::SIZE_DONTCARE, SEARCH_TYPE_TTH);
	else
		SearchFrame::openWindow(searchString);
	return 0;
}

void SpyFrame::on(ClientManagerListener::IncomingSearch, const string& s) noexcept {
	if(ignoreTth && s.compare(0, 4, "TTH:") == 0)
		return;
	tstring* x = new tstring(Text::toT(s));
	tstring::size_type i = 0;
	while( (i=x->find(_T('$'))) != string::npos) {
		(*x)[i] = _T(' ');
	}
	PostMessage(WM_SPEAKER, SEARCH, (LPARAM)x);
}

void SpyFrame::on(ClientManagerListener::IncomingADCSearch, const AdcCommand& adc) noexcept {
	string searchString;
	if (adc.getParam("TR", 0, searchString)) {
		if (ignoreTth)
			return;

		//make it consistent with the nmdc format...
		searchString = "TTH:" + searchString;
	} else {
		StringList tmp;
		if (adc.getParam("AN", 0, tmp)) {
			searchString = Util::toString(" ", tmp);
		} else {
			return;
		}
	}

	tstring* x = new tstring(Text::toT(searchString));
	PostMessage(WM_SPEAKER, SEARCH, (LPARAM)x);
}

void SpyFrame::on(TimerManagerListener::Second, uint64_t) noexcept {
	float* f = new float(0.0);
	for(int i = 0; i < AVG_TIME; ++i) {
		(*f) += (float)perSecond[i];
	}
	(*f) /= AVG_TIME;
		
	cur = (cur + 1) % AVG_TIME;
	perSecond[cur] = 0;
	PostMessage(WM_SPEAKER, TICK_AVG, (LPARAM)f);
}

void SpyFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlSearches.GetBkColor() != WinUtil::bgColor) {
		ctrlSearches.SetBkColor(WinUtil::bgColor);
		ctrlSearches.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlSearches.GetTextColor() != WinUtil::textColor) {
		ctrlSearches.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/**
 * @file
 * $Id: SpyFrame.cpp 469 2009-12-29 21:13:40Z bigmuscle $
 */
