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

#if !defined(STATS_FRAME_H)
#define STATS_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/TimerManager.h"

#include "FlatTabCtrl.h"
#include "WinUtil.h"

class StatsFrame : public MDITabChildWindowImpl<StatsFrame>, public StaticFrame<StatsFrame, ResourceManager::NETWORK_STATISTICS, IDC_NET_STATS>
{
public:
	StatsFrame() : width(0), height(0), timerId(0), twidth(0), lastTick(GET_TICK()), scrollTick(0),
		lastUp(Socket::getTotalUp()), lastDown(Socket::getTotalDown()), max(0), closed(false) 
	{ 
		backgr.CreateSolidBrush(WinUtil::bgColor);
		upload.CreatePen(PS_SOLID, 0, SETTING(UPLOAD_BAR_COLOR));
		download.CreatePen(PS_SOLID, 0, SETTING(DOWNLOAD_BAR_COLOR));
		foregr.CreatePen(PS_SOLID, 0, WinUtil::textColor);
	}

	~StatsFrame() { }

	static CFrameWndClassInfo& GetWndClassInfo() { 
		static CFrameWndClassInfo wc = { 
			{	
				sizeof(WNDCLASSEX), 0, StartWindowProc, 
				0, 0, NULL, NULL, NULL, NULL, NULL, _T("StatsFrame"), NULL 
			},
			NULL, NULL, IDC_ARROW, TRUE, 0, _T(""), IDR_NET_STATS 
		};
		
		return wc; 
	}

	typedef MDITabChildWindowImpl<StatsFrame> baseClass;
	BEGIN_MSG_MAP(StatsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	
private:
	// Pixels per second
	enum { PIX_PER_SEC = 2 };
	enum { LINE_HEIGHT = 10 };
	enum { AVG_SIZE = 5 };

	CBrush backgr;
	CPen upload;
	CPen download;
	CPen foregr;

	struct Stat {
		Stat() : scroll(0), speed(0) { }
		Stat(uint32_t aScroll, int64_t aSpeed) : scroll(aScroll), speed(aSpeed) { }
		uint32_t scroll;
		int64_t speed;
	};
	typedef deque<Stat> StatList;
	typedef StatList::const_iterator StatIter;
	typedef deque<int64_t> AvgList;
	typedef AvgList::const_iterator AvgIter;

	StatList up;
	AvgList upAvg;
	StatList down;
	AvgList downAvg;

	int width;
	int height;
	UINT_PTR timerId;
	int twidth;

	uint64_t lastTick;
	uint64_t scrollTick;
	int64_t lastUp;
	int64_t lastDown;

	int64_t max;
	bool closed;

	void drawLine(CDC& dc, StatIter begin, StatIter end, CRect& rc, CRect& crc);
	void addTick(int64_t bdiff, uint64_t tdiff, StatList& lst, AvgList& avg, int scroll);
};

#endif // !defined(STATS_FRAME_H)