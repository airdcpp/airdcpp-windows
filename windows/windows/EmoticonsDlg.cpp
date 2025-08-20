/* 
 * Copyright (C) Rm.
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

#include <windows/EmoticonsDlg.h>
#include <windows/util/WinUtil.h>
#include <math.h>

#include <windows/EmoticonsManager.h>

namespace wingui {
#define EMOTICONS_ICONMARGIN 8

WNDPROC EmoticonsDlg::m_MFCWndProc = 0;
EmoticonsDlg* EmoticonsDlg::m_pDialog = NULL;
vector<HBITMAP> EmoticonsDlg::bitmapList;

LRESULT EmoticonsDlg::onEmoticonClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	::GetWindowText(hWndCtl, buf, 255);
	result = buf;
	// pro ucely testovani emoticon packu...
	if ((GetKeyState(VK_SHIFT) & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) {
		auto mgr = EmoticonsManager::getInstance();
		const auto& Emoticons = mgr->getEmoticonsList();
		result = _T("");
		string lastEmotionPath = "";
		for (Emoticon::Iter pEmotion = Emoticons.begin(); pEmotion != Emoticons.end(); ++pEmotion) {
			if (lastEmotionPath != (*pEmotion)->getEmoticonBmpPath()) result += (*pEmotion)->getEmoticonText() + _T(" ");
			lastEmotionPath = (*pEmotion)->getEmoticonBmpPath();
		}
	}
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT EmoticonsDlg::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	WNDPROC temp = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(EmoticonsDlg::m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(NewWndProc)));
	if (!m_MFCWndProc) m_MFCWndProc = temp;
	m_pDialog = this;
	::EnableWindow(WinUtil::mainWnd, true);

	auto mgr = EmoticonsManager::getInstance();
	if(mgr->getUseEmoticons() && SETTING(EMOTICONS_FILE)!="Disabled") {

		const auto& Emoticons = mgr->getEmoticonsList();

		if(Emoticons.empty()) {
			PostMessage(WM_CLOSE);
			return 0;
		}

		unsigned int pocet = 0;
		string lastEmotionPath = "";
		Emoticon::Iter pEmotion;
		for(pEmotion = Emoticons.begin(); pEmotion != Emoticons.end(); pEmotion++)
		{
			if ((*pEmotion)->getEmoticonBmpPath() != lastEmotionPath) pocet++;
			lastEmotionPath = (*pEmotion)->getEmoticonBmpPath();
		}

		// x, y jen pro for cyklus
		unsigned int i = (unsigned int)sqrt((double)Emoticons.size());
		unsigned int nXfor = i;
		unsigned int nYfor = i;
		if ((i*i) == (int)Emoticons.size()) {
			nXfor, nYfor = i;
		}
		else {
			nXfor = i+1;
			if ((i*nXfor) < Emoticons.size()) nYfor = i+1;
			else nYfor = i;
		}

		// x, y pro korektni vkladani ikonek za sebou
		i = (unsigned int)sqrt((double)pocet);
		unsigned int nX = i;
		unsigned int nY = i;
		if ((i*i) == pocet) {
			nX, nY = i;
		}
		else {
			nX = i+1;
			if ((i*nX) < pocet) nY = i+1;
			else nY = i;
		}

		BITMAP bm = {0};
		HBITMAP hbm = (*Emoticons.begin())->getEmoticonBmp(GetSysColor(COLOR_BTNFACE));
		GetObject(hbm, sizeof(BITMAP), &bm);
		int bW = bm.bmWidth;
		DeleteObject(hbm);

		pos.bottom = pos.top - 3;
		pos.left = pos.right - nX * (bW + EMOTICONS_ICONMARGIN) - 2;
		pos.top = pos.bottom - nY * (bW + EMOTICONS_ICONMARGIN) - 2;
		EmoticonsDlg::MoveWindow(pos);

		pEmotion = Emoticons.begin();
		lastEmotionPath = "";

		ctrlToolTip.Create(EmoticonsDlg::m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
		ctrlToolTip.SetDelayTime(TTDT_AUTOMATIC, 1000);

		pos.left = 0;
		pos.right = bW + EMOTICONS_ICONMARGIN;
		pos.top = 0;
		pos.bottom = bW + EMOTICONS_ICONMARGIN;

		for (unsigned int iY = 0; iY < nYfor; iY++)
		for (unsigned int iX = 0; iX < nXfor; iX++)
		{
			if ((iY*nXfor)+iX+1 > Emoticons.size()) break;

			// dve stejne emotikony za sebou nechceme
			if ((*pEmotion)->getEmoticonBmpPath() != lastEmotionPath) {
				try {
					CButton emoButton;
					emoButton.Create(EmoticonsDlg::m_hWnd, pos, (*pEmotion)->getEmoticonText().c_str(), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER);
					HBITMAP b = (*pEmotion)->getEmoticonBmp(GetSysColor(COLOR_BTNFACE));
					emoButton.SetBitmap(b);
					bitmapList.push_back(b);

					tstring smajl = (*pEmotion)->getEmoticonText();
					CToolInfo ti(TTF_SUBCLASS, emoButton.m_hWnd, 0, 0, const_cast<LPTSTR>(smajl.c_str()));
					ctrlToolTip.AddTool(&ti);

					DeleteObject((HGDIOBJ)emoButton);
				} catch (...) { }

				pos.left = pos.left + bW + EMOTICONS_ICONMARGIN;
				pos.right = pos.left + bW + EMOTICONS_ICONMARGIN;
				if (pos.left >= (LONG)(nX*(bW+EMOTICONS_ICONMARGIN))) {
					pos.left = 0;
					pos.right = bW + EMOTICONS_ICONMARGIN;
					pos.top = pos.top + bW + EMOTICONS_ICONMARGIN;
					pos.bottom = pos.top + bW + EMOTICONS_ICONMARGIN;
				}
			}

			lastEmotionPath = (*pEmotion)->getEmoticonBmpPath();
			try { pEmotion++; }
			catch (...) {}
		}

		try {
			pos.left = -128;
			pos.right = pos.left;
			pos.top = -128;
			pos.bottom = pos.top;
			CButton emoButton;
			emoButton.Create(EmoticonsDlg::m_hWnd, pos, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT);
			emoButton.SetFocus();
			DeleteObject((HGDIOBJ)emoButton);
		} catch (...) { }

	} else PostMessage(WM_CLOSE);

	return 0;
}

LRESULT CALLBACK EmoticonsDlg::NewWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message==WM_ACTIVATE && wParam==0) {
		m_pDialog->PostMessage(WM_CLOSE);
		return FALSE;
    }
	if (message==WM_CLOSE) {
		for(vector<HBITMAP>::const_iterator i = bitmapList.begin(); i != bitmapList.end(); i++) {
			DeleteObject(*i);
		}
		bitmapList.clear();
	}
	return ::CallWindowProc(m_MFCWndProc, hWnd, message, wParam, lParam);
}
}