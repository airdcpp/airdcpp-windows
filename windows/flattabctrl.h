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

#if !defined(FLAT_TAB_CTRL_H)
#define FLAT_TAB_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/SettingsManager.h"
#include "../client/ResourceManager.h"

#include "WinUtil.h"
#include "memdc.h"
#include "OMenu.h"
#include "resource.h"

enum {
	FT_FIRST = WM_APP + 700,
	/** This will be sent when the user presses a tab. WPARAM = HWND */
	FTM_SELECTED,
	/** The number of rows changed */
	FTM_ROWS_CHANGED,
	/** Set currently active tab to the HWND pointed by WPARAM */
	FTM_SETACTIVE,
	/** Display context menu and return TRUE, or return FALSE for the default one */
	FTM_CONTEXTMENU,
	/** Close window with postmessage... */
	WM_REALLY_CLOSE
};

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE FlatTabCtrlImpl : public CWindowImpl< T, TBase, TWinTraits> {
public:

	enum { FT_EXTRA_SPACE = 30 };

	FlatTabCtrlImpl() : closing(NULL), rows(1), height(0), active(NULL), moving(NULL), inTab(false) {
		black.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	}
	virtual ~FlatTabCtrlImpl() { }

	static LPCTSTR GetWndClassName()
	{
		return _T("FlatTabCtrl");
	}

	void addTab(HWND hWnd, HICON hIcon = NULL) {
		TabInfo* i = new TabInfo(hWnd, hIcon);
		dcassert(getTabInfo(hWnd) == NULL);
		tabs.push_back(i);
		viewOrder.push_back(hWnd);
		nextTab = --viewOrder.end();
		active = i;
		update();
		calcRows(false);
		Invalidate();
	}

	void removeTab(HWND aWnd) {
		TabInfo::ListIter i;
		for(i = tabs.begin(); i != tabs.end(); ++i) {
			if((*i)->hWnd == aWnd)
				break;
		}

		dcassert(i != tabs.end());
		TabInfo* ti = *i;
		if(active == ti)
			active = NULL;
		delete ti;
		tabs.erase(i);
		viewOrder.remove(aWnd);
		nextTab = viewOrder.end();
		if(!viewOrder.empty())
			--nextTab;

		update();
		calcRows(false);
		Invalidate();
	}

	void startSwitch() {
		nextTab = --viewOrder.end();
		inTab = true;
	}

	void endSwitch() {
		inTab = false;
		if(active)
		setTop(active->hWnd);
	}

	HWND getNext() {
		if(viewOrder.empty())
			return NULL;
		if(nextTab == viewOrder.begin()) {
			nextTab = --viewOrder.end();
		} else {
			--nextTab;
		}
		return *nextTab;
	}
	HWND getPrev() {
		if(viewOrder.empty())
			return NULL;
		nextTab++;
		if(nextTab == viewOrder.end()) {
			nextTab = viewOrder.begin();
		}
		return *nextTab;
	}

	void setActive(HWND aWnd) {
		if(!inTab)
			setTop(aWnd);

		TabInfo* ti = getTabInfo(aWnd);
		dcassert(ti != NULL);
		active = ti;
		ti->dirty = false;
		ti->notification = false;
		calcRows(false);
		Invalidate();
	}

	void setTop(HWND aWnd) {
		viewOrder.remove(aWnd);
		viewOrder.push_back(aWnd);
		nextTab = --viewOrder.end();
	}

	void setDirty(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		dcassert(ti != NULL);
		bool inval = ti->update();

		if(active != ti) {
			if(!ti->dirty) {
				ti->dirty = true;
				inval = true;
			}
		}

		if(inval) {
			calcRows(false);
			Invalidate();
		}
	}

	void setIcon(HWND aWnd, HICON setIconVal) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->hIcon = setIconVal;
			Invalidate();
		}
	}

	void setDisconnected(HWND aWnd, bool disconnected) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			if( ti->disconnected != disconnected){
				ti->disconnected = disconnected;
				Invalidate();
			}
		}
	}

	void setNotify(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			if(ti != active) {
				ti->notification = true;
				Invalidate();
			}
		}
	}

	void updateText(HWND aWnd, LPCTSTR text) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->updateText(text);
			calcRows(false);
			Invalidate();
		}
	}

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp)
		MESSAGE_HANDLER(WM_MBUTTONUP, onMButtonUp)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_CHEVRON, onChevron)
		COMMAND_RANGE_HANDLER(IDC_SELECT_WINDOW, IDC_SELECT_WINDOW+tabs.size(), onSelectWindow)
	END_MSG_MAP()

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int row = getRows() - ((yPos / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked
				HWND hWnd = GetParent();
				if(hWnd) {
					if(wParam & MK_SHIFT || wParam & MK_XBUTTON1)
						::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
					else
						moving = t;
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		if (moving) {
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			int row = getRows() - ((yPos / getTabHeight()) + 1);

			bool moveLast = true;

			for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
				TabInfo* t = *i;
			
				if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
					// Bingo, this was clicked
					HWND hWnd = GetParent();
					if(hWnd) {
						if(t == moving)
							::SendMessage(hWnd, FTM_SELECTED, (WPARAM)t->hWnd, 0);
						else{
							//check if the pointer is on the left or right half of the tab
							//to determine where to insert the tab
							moveTabs(t, xPos > (t->xpos + (t->getWidth()/2)));
						}
					}
					moveLast = false;
					break;
				}
			}
			if(moveLast)
				moveTabs(tabs.back(), true);
			moving = NULL;
		}
		return 0;
	}

	LRESULT onMButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		int xPos = GET_X_LPARAM(lParam); 
		int yPos = GET_Y_LPARAM(lParam); 
		int row = getRows() - ((yPos / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked
				HWND hWnd = GetParent();
				if(hWnd) {
					::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };		// location of mouse click

		ScreenToClient(&pt);
		int xPos = pt.x;
		int row = getRows() - ((pt.y / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked, check if the owner wants to handle it...
				if(!::SendMessage(t->hWnd, FTM_CONTEXTMENU, 0, lParam)) {
					closing = t->hWnd;
					ClientToScreen(&pt);
					OMenu mnu;
					mnu.CreatePopupMenu();
					mnu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
					mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, m_hWnd);
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(::IsWindow(closing))
			::SendMessage(closing, WM_CLOSE, 0, 0);
		return 0;
	}

	void SwitchTo(bool next = true){
		TabInfo::ListIter i = tabs.begin();
		for(; i != tabs.end(); ++i){
			if((*i)->hWnd == active->hWnd){
				if(next){
					++i;
					if(i == tabs.end())
						i = tabs.begin();
				} else{
					if(i == tabs.begin())
						i = tabs.end();
					--i;
				}
				setActive((*i)->hWnd);
				::SetWindowPos((*i)->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
				break;
			}
		}
		
	}
	void SwitchWindow(int aWindow){
		TabInfo::ListIter i = tabs.begin();

		//find the right tab
		for(int j = 0; i != tabs.end() && j < aWindow; ++i, ++j);

		//check that there actually is enough tabs =)
		if(i != tabs.end()){
			setActive((*i)->hWnd);
			::SetWindowPos((*i)->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		}
	}

	int getTabHeight() { return height; }
	int getHeight() { return (getRows() * getTabHeight())+1; }
	int getFill() { return (getTabHeight() + 1) / 2; }

	int getRows() { return rows; }

	void calcRows(bool inval = true) {
		CRect rc;
		GetClientRect(rc);
		int r = 1;
		int w = 0;
		bool notify = false;
		bool needInval = false;

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* ti = *i;
			if( (r != 0) && ((w + ti->getWidth() ) > rc.Width()) ) {
				if(r >= SETTING(MAX_TAB_ROWS)) {
					notify |= (rows != r);
					rows = r;
					r = 0;
					chevron.EnableWindow(TRUE);
				} else {
					r++;
					w = 0;
				}
			}
			ti->xpos = w;
			needInval |= (ti->row != (r-1));
			ti->row = r-1;
			w += ti->getWidth();
		}

		if(r != 0) {
			chevron.EnableWindow(FALSE);
			notify |= (rows != r);
			rows = r;
		}

		if(notify) {
			::SendMessage(GetParent(), FTM_ROWS_CHANGED, 0, 0);
		}
		if(needInval && inval)
			Invalidate();
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		chevron.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			BS_PUSHBUTTON , 0, IDC_CHEVRON);
		chevron.SetWindowText(_T("\u00bb"));

		mnu.CreatePopupMenu();

		CDCHandle dc(::GetDC(m_hWnd));
		HFONT oldfont = dc.SelectFont(WinUtil::font);
		height = WinUtil::getTextHeight(dc) + 7;
		dc.SelectFont(oldfont);
		::ReleaseDC(m_hWnd, dc);

		return 0;
	}

	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		calcRows();
		SIZE sz = { LOWORD(lParam), HIWORD(lParam) };
		chevron.MoveWindow(sz.cx-14, 1, 14, getHeight());
		return 0;
	}
	
	LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return FALSE;
	}

	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		RECT rc;
		bool drawActive = false;
		RECT crc;
		GetClientRect(&crc);

		if(GetUpdateRect(&rc, FALSE)) {
			CPaintDC dc(m_hWnd);
			dc.SetBkColor(SETTING(TAB_INACTIVE_BG));
			CMemDC memDC(&dc);
			
			HFONT oldfont = memDC.SelectFont(WinUtil::font);

			for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
				TabInfo* t = *i;

				if(t->row != -1 && t->xpos < rc.right && t->xpos + t->getWidth()  >= rc.left ) {
					if(t != active) {
						drawTab(memDC, t, t->xpos, t->row);
					} else {
						drawActive = true;
					}
				}
			}

			HPEN oldpen = memDC.SelectPen(::CreatePen(PS_SOLID, 1, SETTING(TAB_INACTIVE_BORDER)));
			for(int r = 0; r < rows; r++) {
				memDC.MoveTo(rc.left, r*getTabHeight());
				memDC.LineTo(rc.right, r*getTabHeight());
			}
			::DeleteObject(memDC.SelectPen(oldpen));
			
			if(drawActive) {
				dcassert(active);
				drawTab(memDC, active, active->xpos, active->row, true);
				HPEN pen = memDC.SelectPen(::CreatePen(PS_SOLID, 2, GetSysColor(COLOR_BTNFACE)));
				int y = (rows - active->row -1) * getTabHeight();
				memDC.MoveTo(active->xpos, y);
				memDC.LineTo(active->xpos + active->getWidth(), y);
				DeleteObject(memDC.SelectPen(pen));
			}
			memDC.SelectFont(oldfont);
			memDC.Paint();
		}
		return 0;
	}

	LRESULT onChevron(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		while(mnu.GetMenuItemCount() > 0) {
			mnu.RemoveMenu(0, MF_BYPOSITION);
		}
		int n = 0;
		CRect rc;
		GetClientRect(&rc);
		CMenuItemInfo mi;
		mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
		mi.fType = MFT_STRING | MFT_RADIOCHECK;

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* ti = *i;
			if(ti->row == -1) {
				mi.dwTypeData = (LPTSTR)ti->name.c_str();
				mi.dwItemData = (ULONG_PTR)ti->hWnd;
				mi.fState = MFS_ENABLED | (ti->dirty ? MFS_CHECKED : 0);
				mi.wID = IDC_SELECT_WINDOW + n;
				mnu.InsertMenuItem(n++, TRUE, &mi);
			} 
		}

		POINT pt;
		chevron.GetClientRect(&rc);
		pt.x = rc.right - rc.left;
		pt.y = 0;
		chevron.ClientToScreen(&pt);

		mnu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return 0;
	}

	LRESULT onSelectWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;

		mnu.GetMenuItemInfo(wID, FALSE, &mi);
		HWND hWnd = GetParent();
		if(hWnd) {
			SendMessage(hWnd, FTM_SELECTED, (WPARAM)mi.dwItemData, 0);
		}
		return 0;
	}

	void update(){
		TabInfo::ListIter i = tabs.begin();
		int j = 1;
		for(; i != tabs.end(); ++i, ++j){
			if(j <= 10)
				(*i)->wCode = j % 10;
			else
				(*i)->wCode = -1;
		}
	}

private:
	class TabInfo {
	public:

		typedef vector<TabInfo*> List;
		typedef typename List::iterator ListIter;

		TabInfo(HWND aWnd, HICON icon) : hWnd(aWnd), len(0), xpos(0), row(0), 
			dirty(false), hIcon(icon), disconnected(false), notification(false), wCode(-1),
			MAX_LENGTH(SETTING(TAB_SIZE)){ 
			memset(&size, 0, sizeof(size));
			memset(&boldSize, 0, sizeof(boldSize));
			update();
		}

		const unsigned int MAX_LENGTH;
		HWND hWnd;
		size_t len;
		tstring name;
		SIZE size;
		SIZE boldSize;
		int xpos;
		int row;
		bool dirty;
		int wCode;
		
		//this is used for pm where user is offline too
		bool disconnected;
		bool notification;

		HICON hIcon;

		bool update() {
			TCHAR *name2 = new TCHAR[MAX_LENGTH+2];
			::GetWindowText(hWnd, name2, MAX_LENGTH+1);
			bool ret = updateText(name2);
			delete[] name2;
			return ret;
		}

		bool updateText(LPCTSTR text) {
			len = _tcslen(text);
			if(len >= MAX_LENGTH && MAX_LENGTH > 7) {
				name.assign(text, MAX_LENGTH - 3);
				name.append(_T("..."));
			} else if(len >= MAX_LENGTH){
				name.assign(text, MAX_LENGTH);
			} else {
				name.assign(text, len);
			}
			len = name.length();
			CDC dc(::GetDC(hWnd));
			HFONT f = dc.SelectFont(WinUtil::font);
			dc.GetTextExtent(name.c_str(), len, &size);
			dc.SelectFont(WinUtil::boldFont);
			dc.GetTextExtent(name.c_str(), len, &boldSize);
			dc.SelectFont(f);
			::ReleaseDC(hWnd, dc);
			return true;
		}

		int getWidth() {
			return ( ( dirty && !BOOLSETTING(BLEND_TABS) ) ? boldSize.cx : size.cx) + ((wCode == -1) ? FT_EXTRA_SPACE -7: FT_EXTRA_SPACE) - (BOOLSETTING(TAB_SHOW_ICONS) ? 0 : 16);
			
		}
	};

	void moveTabs(TabInfo* aTab, bool after){
		if(moving == NULL)
			return;

		TabInfo::ListIter i, j;
		//remove the tab we're moving
		for(j = tabs.begin(); j != tabs.end(); ++j){
			if((*j) == moving){
				tabs.erase(j);
				break;
			}
		}

		//find the tab we're going to insert before or after
		for(i = tabs.begin(); i != tabs.end(); ++i){
			if((*i) == aTab){
				if(after)
					++i;
				break;
			}
		}

		tabs.insert(i, moving);
				
		update();
		calcRows(false);
		Invalidate();
	}

	HWND closing;
	CButton chevron;
	CMenu mnu;

	int rows;
	int height;

	TabInfo* active;
	TabInfo* moving;
	typename TabInfo::List tabs;
	CPen black;

	typedef list<HWND> WindowList;
	typedef WindowList::iterator WindowIter;

	WindowList viewOrder;
	WindowIter nextTab;

	bool inTab;

	TabInfo* getTabInfo(HWND aWnd) {
		for(TabInfo::ListIter i	= tabs.begin(); i != tabs.end(); ++i) {
			if((*i)->hWnd == aWnd)
				return *i;
		}
		return NULL;
	}

	/**
	 * Draws a tab
	 * @return The width of the tab
	 */
	void drawTab(CDC& dc, TabInfo* tab, int pos, int row, bool aActive = false) {

		int ypos = (getRows() - row - 1) * getTabHeight();

		//variables to store the old tools
		HPEN oldPen;
		HBRUSH oldBrush;
		COLORREF oldTextColor;

		//create the background brush
		HBRUSH brBackground;
		if(aActive)
			brBackground = ::CreateSolidBrush(SETTING(TAB_ACTIVE_BG));
		else if(tab->disconnected)
			brBackground = ::CreateSolidBrush(SETTING(TAB_INACTIVE_BG_DISCONNECTED));
		else if(tab->notification)
			brBackground = ::CreateSolidBrush(SETTING(TAB_INACTIVE_BG_NOTIFY));
		else if(tab->dirty && BOOLSETTING(BLEND_TABS)){
			COLORREF bgBase = SETTING(TAB_INACTIVE_BG);
			int mod = (HLS_L(RGB2HLS(bgBase)) >= 128) ? - SETTING(TAB_DIRTY_BLEND) : SETTING(TAB_DIRTY_BLEND);
			brBackground = ::CreateSolidBrush( HLS_TRANSFORM(bgBase, mod, 0) );
		} else {
			brBackground = ::CreateSolidBrush( SETTING(TAB_INACTIVE_BG) );
		}
		
		//Create the pen used to draw the borders
		HPEN borderPen = ::CreatePen(PS_SOLID, 1, aActive ? SETTING(TAB_ACTIVE_BORDER) : SETTING(TAB_INACTIVE_BORDER));

		oldPen = dc.SelectPen(borderPen);
		
		CRect rc(pos, ypos, pos + tab->getWidth(), ypos + getTabHeight());
		
		//paint the background
		oldBrush = dc.SelectBrush(brBackground);
		dc.FillRect(rc, brBackground);
		
		//draw the borders
		if(aActive) {
			//draw right line
			dc.MoveTo(rc.right , rc.top);
			dc.LineTo(rc.right , rc.bottom);
			
			//draw the bottom line
			dc.MoveTo(rc.left, rc.bottom);
			dc.LineTo(rc.right, rc.bottom);
		} else {
			//draw right line
			dc.MoveTo(rc.right , rc.bottom - 2);
			dc.LineTo(rc.right , rc.top + 2);
			
			//draw left line
			dc.MoveTo(rc.left, rc.bottom - 2);
			dc.LineTo(rc.left, rc.top + 2);
		}

		dc.SetBkMode(TRANSPARENT);
		
		//if the tab is inactive, use a light gray color on the text
		oldTextColor = dc.SetTextColor(aActive ? SETTING(TAB_ACTIVE_TEXT) : SETTING(TAB_INACTIVE_TEXT));
	
		int mapMode = dc.SetMapMode(MM_TEXT);

		//Draw the icon
		if(tab->hIcon != NULL && BOOLSETTING(TAB_SHOW_ICONS)) {
			dc.DrawIconEx(pos+3, ypos + (getTabHeight() / 2) - 8 , tab->hIcon, 16, 16, 0, NULL, DI_NORMAL | DI_COMPAT);
		}
		
		int spacing = BOOLSETTING(TAB_SHOW_ICONS) ? 20 : 2;
		if(tab->wCode != -1){
			HFONT f = dc.SelectFont(WinUtil::tabFont);
			dc.TextOut(pos + spacing, ypos +3, Util::toStringW(tab->wCode).c_str(), 1);
			dc.SelectFont(f);

			spacing += WinUtil::getTextWidth(m_hWnd, WinUtil::tabFont) + 2;
		}

		if( tab->dirty && !BOOLSETTING(BLEND_TABS) ) {
			HFONT f = dc.SelectFont(WinUtil::boldFont);
			dc.TextOut(pos + spacing, ypos + 3, tab->name.c_str(), tab->name.length());
			dc.SelectFont(f);		
		} else {
			dc.TextOut(pos + spacing, ypos + 3, tab->name.c_str(), tab->name.length());
		}
	
		dc.SetMapMode(mapMode);

		//restore the old tools
		dc.SetTextColor(oldTextColor);
		dc.SelectPen(oldPen);
		dc.SelectBrush(oldBrush);

		//cleanup
		DeleteObject(brBackground);
		DeleteObject(borderPen);
	}
};

class FlatTabCtrl : public FlatTabCtrlImpl<FlatTabCtrl> {
public:
	DECLARE_FRAME_WND_CLASS_EX(GetWndClassName(), 0, 0, COLOR_3DFACE);
};

template <class T, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits>
class ATL_NO_VTABLE MDITabChildWindowImpl : public CMDIChildWindowImpl<T, TBase, TWinTraits> {
public:

	MDITabChildWindowImpl() : created(false) { }
	FlatTabCtrl* getTab() { return WinUtil::tabCtrl; }

	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }

 	typedef MDITabChildWindowImpl<T, TBase, TWinTraits> thisClass;
	typedef CMDIChildWindowImpl<T, TBase, TWinTraits> baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SYSCOMMAND, onSysCommand)
		MESSAGE_HANDLER(WM_FORWARDMSG, onForwardMsg)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_MDIACTIVATE, onMDIActivate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTEXT, onSetText)
		MESSAGE_HANDLER(WM_REALLY_CLOSE, onReallyClose)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
	DWORD dwStyle = 0, DWORD dwExStyle = 0,
	UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		if(nMenuID != 0)
#if (_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(ATL::_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#else //!(_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#endif //!(_ATL_VER >= 0x0700)

		if(m_hMenu == NULL)
			m_hMenu = WinUtil::mainMenu;

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;	// force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		// If the currently active MDI child is maximized, we want to create this one maximized too
		ATL::CWindow wndParent = hWndParent;
		BOOL bMaximized = FALSE;

		if(MDIGetActive(&bMaximized) == NULL)
			bMaximized = BOOLSETTING(MDI_MAXIMIZED);

		if(bMaximized)
			wndParent.SetRedraw(FALSE);

		HWND hWnd = CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, (UINT)0U, atom, lpCreateParam);

		if(bMaximized)
		{
			// Maximize and redraw everything
			if(hWnd != NULL)
				MDIMaximize(hWnd);
			wndParent.SetRedraw(TRUE);
			wndParent.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
			::SetFocus(GetMDIFrame());	// focus will be set back to this window
		}
		else if(hWnd != NULL && ::IsWindowVisible(m_hWnd) && !::IsChild(hWnd, ::GetFocus()))
		{
			::SetFocus(hWnd);
		}

		return hWnd;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif
	}

	// All MDI windows must have this in wtl it seems to handle ctrl-tab and so on...
	LRESULT onForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		return baseClass::PreTranslateMessage((LPMSG)lParam);
	}

	LRESULT onSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == SC_NEXTWINDOW) {
			HWND next = getTab()->getNext();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		} else if(wParam == SC_PREVWINDOW) {
			HWND next = getTab()->getPrev();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onCreate(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		HICON hIcon = (HICON)::SendMessage(m_hWnd, WM_GETICON, ICON_SMALL, 0);
		getTab()->addTab(m_hWnd, hIcon);
		created = true;
		return 0;
	}

	LRESULT onMDIActivate(UINT /*uMsg*/, WPARAM /*wParam */, LPARAM lParam, BOOL& bHandled) {
		dcassert(getTab());
		if((m_hWnd == (HWND)lParam))
			getTab()->setActive(m_hWnd);

		bHandled = FALSE;
		return 1; 
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->removeTab(m_hWnd);
		if(m_hMenu == WinUtil::mainMenu)
			m_hMenu = NULL;

		BOOL bMaximized = FALSE;
		if(::SendMessage(m_hWndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized) != NULL)
			SettingsManager::getInstance()->set(SettingsManager::MDI_MAXIMIZED, (bMaximized>0));

		return 0;
	}

	LRESULT onReallyClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		MDIDestroy(m_hWnd);
		return 0;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled */) {
		PostMessage(WM_REALLY_CLOSE);
		return 0;
	}

	LRESULT onSetText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		if(created) {
			getTab()->updateText(m_hWnd, (LPCTSTR)lParam);
		}
		return 0;
	}

	LRESULT onKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			getTab()->startSwitch();
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == VK_CONTROL) {
			getTab()->endSwitch();
		}
		bHandled = FALSE;
		return 0;

	}

	void setDirty() {
		dcassert(getTab());
		getTab()->setDirty(m_hWnd);
	}
	
	void setDisconnected(bool dis = false) {
		dcassert(getTab());
		getTab()->setDisconnected(m_hWnd, dis);
	}
	
	void setNotify(){
		dcassert(getTab());
		getTab()->setNotify(m_hWnd);
	}

	void setIcon(HICON setIconVal) {
		dcassert(getTab());
		getTab()->setIcon(m_hWnd, setIconVal);
	}

private:
	bool created;
};

#endif // !defined(FLAT_TAB_CTRL_H)
