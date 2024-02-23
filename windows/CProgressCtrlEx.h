/*
 * Copyright (C) 2011-2024 AirDC++ Project
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

/*
* A Custom control that wraps CStatic on CProgressBarCtrl,
* using invisible background for the CStatic to be able to show text on top of the progressbar.
* The text is centered by default, alignment can be changed via SetTextAlign().
*/
#ifndef PROGRESSCTRLEX_H
#define PROGRESSCTRLEX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "WinUtil.h"

class CProgressCtrlEx :  public CWindowImpl< CProgressCtrlEx, CProgressBarCtrl, CControlWinTraits> {

public:
	CProgressCtrlEx() {
		dTextFormat = DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
	}
	 virtual ~CProgressCtrlEx() {}
	 
	 BEGIN_MSG_MAP(CProgressCtrlEx)
		 MESSAGE_HANDLER(WM_PAINT, onPaint)
		 MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBg)
	END_MSG_MAP()
	
	
	LRESULT OnEraseBg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return TRUE;
	}

	LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		DefWindowProc(uMsg, wParam, lParam);
		CRect rc;
		GetClientRect(rc);
		CDC dc = GetDC();
		dc.SetBkMode(TRANSPARENT);
		dc.SelectFont(WinUtil::progressFont);
		dc.SetTextColor(WinUtil::TBprogressTextColor);
		dc.DrawText(ctrlText.c_str(), _tcslen(ctrlText.c_str()), &rc, dTextFormat);
		ReleaseDC(dc);
		return 0;
	}
	
	void SetPosWithText(int aPos, const tstring& aText){
		SetText(aText);
		SetPos(aPos);
	}

	void SetPosWithPercentage(int aPos){
		showPercentage(aPos);
		SetPos(aPos);;
	}
	
	void SetText(const tstring& text){
		ctrlText = text;
	}
	
	tstring GetText() {
		return ctrlText;
	}

	LONG GetTextLen() {
		return ctrlText.size();
	}

	void SetTextFormat(DWORD newFormat) {
		dTextFormat = newFormat;
	}

private:

	void showPercentage(int pos){
		tstring tmp;
		if(pos > 0 && GetRangeLimit(0) > 0){
			tmp = Text::toT(Util::toString(((double)pos / (int)GetRangeLimit(0) *100))) + _T("%");
		} else 
			tmp = _T("0%");
		ctrlText = tmp;
	}

	DWORD dTextFormat;
	tstring ctrlText;

};

#endif