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

#ifndef DCPLUSPLUS_WIN32_SPLASHWINDOW_H
#define DCPLUSPLUS_WIN32_SPLASHWINDOW_H

#ifdef __ATLMISC_H__
#define __ATLTYPES_H__
#endif

#include <string>
#include "../client/typedefs.h"
#include <atlimage.h>
#include "ExCImage.h"
#include "Async.h"

using std::string;

class SplashWindow : public CWindowImpl<SplashWindow, CWindow>, public Async<SplashWindow> {
public:
	BEGIN_MSG_MAP(SplashImpl)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
	END_MSG_MAP()

	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void OnFinalMessage(HWND /*hWnd*/);

	SplashWindow();
	virtual ~SplashWindow();

	static void create();
	void destroy();

	void operator()(const string& status);
	void operator()(float progress);
private:
	void draw();
	void loadImage();
	tstring title;

	CEdit dummy;
	ExCImage img;

	int width;
	int height;

	tstring status;
	float progress = 0;
	bool closing = false;

	HFONT hFontTitle;
	HFONT hFontStatus;
};

#endif
