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

#include "../client/Socket.h"

#include "StatsFrame.h"
#include "WinUtil.h"

LRESULT StatsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	timerId = SetTimer(1, 1000);
	
	SetFont(WinUtil::font);

	WinUtil::SetIcon(m_hWnd, _T("netstats.ico"));
	bHandled = FALSE;
	return 1;
}

LRESULT StatsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
	if(timerId != 0)
		KillTimer(timerId);

		closed = true;		
		WinUtil::setButtonPressed(IDC_NET_STATS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		bHandled = FALSE;
		return 0;
	}	
}

void StatsFrame::drawLine(CDC& dc, StatIter begin, StatIter end, CRect& rc, CRect& crc) {
	int x = crc.right;
	
	StatIter i;
	for(i = begin; i != end; ++i) {
		if((x - (int)i->scroll) < rc.right)
			break;
		x -= i->scroll;
	}
	if(i != end) {
		int y = (max == 0) ? 0 : (int)((i->speed * height) / max);
		dc.MoveTo(x, height - y);
		x -= i->scroll;
		++i;

		for(; i != end && x > twidth; ++i) {
			y = (max == 0) ? 0 : (int)((i->speed * height) / max);
			dc.LineTo(x, height - y);
			if(x < rc.left)
				break;
			x -= i->scroll;
		}
	}
}

LRESULT StatsFrame::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(GetUpdateRect(NULL)) {
		CPaintDC dc(m_hWnd);
		CRect rc(dc.m_ps.rcPaint);
		dcdebug("Update: %d, %d, %d, %d\n", rc.left, rc.top, rc.right, rc.bottom);

		dc.SelectBrush(backgr);
		dc.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), NULL, 0, 0, PATCOPY);

		CRect clientRC;
		GetClientRect(clientRC);
		
		dc.SetTextColor(WinUtil::textColor);
		dc.SetBkColor(WinUtil::bgColor);
	
		dc.SelectPen(foregr);
		dc.SelectFont(WinUtil::font);
		int lines = height / (WinUtil::fontHeight * LINE_HEIGHT);
		int lheight = height / (lines+1);

		for(int i = 0; i < lines; ++i) {
			int ypos = lheight * (i+1);
			if(ypos > WinUtil::fontHeight + 2) {
				dc.MoveTo(rc.left, ypos);
				dc.LineTo(rc.right, ypos);
			}
			
			if(rc.left <= twidth) {
				
				ypos -= WinUtil::fontHeight + 2;
				if(ypos < 0)
					ypos = 0;
				if(height == 0)
					height = 1;
				tstring txt = Util::formatBytesW(max * (height-ypos) / height) + _T("/s");
				int tw = WinUtil::getTextWidth(txt, m_hWnd);
				if(tw + 2 > twidth)
					twidth = tw + 2;
				dc.TextOut(1, ypos, txt.c_str());
			}
		}

		if(rc.left < twidth) {
			tstring txt = Util::formatBytesW(max) + _T("/s");
			int tw = WinUtil::getTextWidth(txt, m_hWnd);
			if(tw + 2 > twidth)
				twidth = tw + 2;
			dc.TextOut(1, 1, txt.c_str());
		}

		dc.SelectPen(upload);
		drawLine(dc, up.begin(), up.end(), rc, clientRC);

		dc.SelectPen(download);
		drawLine(dc, down.begin(), down.end(), rc, clientRC);

	}
	return 0;
}

void StatsFrame::addTick(int64_t bdiff, uint64_t tdiff, StatList& lst, AvgList& avg, int scroll) {
	while((int)lst.size() > ((width / PIX_PER_SEC) + 1) ) {
		lst.pop_back();
	}
	while(avg.size() > AVG_SIZE ) {
		avg.pop_back();
	}
	int64_t bspeed = bdiff * (int64_t)1000 / tdiff;
	avg.push_front(bspeed);

	bspeed = 0;

	for(AvgIter ai = avg.begin(); ai != avg.end(); ++ai) {
		bspeed += *ai;
	}

	bspeed /= avg.size();
	lst.push_front(Stat(scroll, bspeed));
}

LRESULT StatsFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	uint64_t tick = GET_TICK();
	uint64_t tdiff = tick - lastTick;
	if(tdiff == 0)
		return 0;

	uint64_t scrollms = (tdiff + scrollTick)*PIX_PER_SEC;
	uint64_t scroll = scrollms / 1000;

	if(scroll == 0)
		return 0;

	scrollTick = scrollms - (scroll * 1000);

	CRect rc;
	GetClientRect(rc);
	rc.left = twidth;
	ScrollWindow(-((int)scroll), 0, rc, rc);

	int64_t d = Socket::getTotalDown();
	int64_t ddiff = d - lastDown;
	int64_t u = Socket::getTotalUp();
	int64_t udiff = u - lastUp;

	addTick(ddiff, tdiff, down, downAvg, (int)scroll);
	addTick(udiff, tdiff, up, upAvg, (int)scroll);

	int64_t mspeed = 0;
	StatIter i;
	for(i = down.begin(); i != down.end(); ++i) {
		if(mspeed < i->speed)
			mspeed = i->speed;
	}
	for(i = up.begin(); i != up.end(); ++i) {
		if(mspeed < i->speed)
			mspeed = i->speed;
	}
	if(mspeed > max || ((max * 3 / 4) > mspeed) ) {
		max = mspeed;
		Invalidate();
	}

	lastTick = tick;
	lastUp = u;
	lastDown = d;
	return 0;
}

LRESULT StatsFrame::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	GetClientRect(rc);
	width = rc.Width();
	height = rc.Height() - 1;
	Invalidate();
	bHandled = FALSE;
	return 0;
}

void StatsFrame::UpdateLayout(BOOL /*bResizeBars*/ /* = TRUE */) {
	
}

/**
 * @file
 * $Id: StatsFrame.cpp 463 2009-10-01 16:30:22Z BigMuscle $
 */
