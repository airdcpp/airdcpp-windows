/*
 * Copyright (C) 2011-2012 AirDC++ Project
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
		dTextAlign = SS_CENTER;
	}
	 virtual ~CProgressCtrlEx() {}
	 
	 BEGIN_MSG_MAP(CProgressCtrlEx)
		 MESSAGE_HANDLER(WM_CREATE, OnCreate)
		 MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		 MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBg)
	END_MSG_MAP()
	
	LRESULT OnCreate(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		DefWindowProc(Msg, wParam, lParam);
		CRect rc;
		GetClientRect(&rc);
		ctrlText.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS | SS_CENTERIMAGE | dTextAlign, WS_EX_TRANSPARENT);
		return 0;
	}
	
	LRESULT OnEraseBg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return TRUE;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlText.m_hWnd) {
			::SetBkMode(hDC, TRANSPARENT);
			::SelectObject(hDC, WinUtil::progressFont);
			::SetTextColor(hDC, WinUtil::TBprogressTextColor);
			return (LRESULT)(HBRUSH)::GetStockObject(NULL_BRUSH);
		}
		return FALSE;
	}

	void SetPosWithText(int aPos, const tstring& aText){
		SetRedraw(FALSE);
		SetPos(aPos);
		SetText(aText);
		SetRedraw(TRUE);
		Invalidate();
	}
	void SetPosWithPercentage(int aPos){
		SetRedraw(FALSE);
		SetPos(aPos);
		showPercentage();
		SetRedraw(TRUE);
		Invalidate();
	}
	
	void SetText(const tstring& text){
		ctrlText.SetWindowText(text.c_str());
	}

	//SS_LEFT, SS_RIGHT or SS_CENTER
	void SetTextAlign(DWORD newAlign) {
		ctrlText.ModifyStyle(dTextAlign, newAlign);
	}	
	
	tstring GetText() {
		tstring tmp;
		tmp.resize(ctrlText.GetWindowTextLength());
		tmp.resize(ctrlText.GetWindowText(&tmp[0], tmp.size()+1));
		return tmp;
	}

	LONG GetTextLen() {
		return ctrlText.GetWindowTextLength();
	}

private:

	void showPercentage(){
		tstring tmp;
		if(GetPos() > 0 && GetRangeLimit(0) > 0){
			tmp = Text::toT(Util::toString(((double)(int)GetPos() / (int)GetRangeLimit(0) *100))) + _T("%");
		} else 
			tmp = _T("0%");
		ctrlText.SetWindowText(tmp.c_str());
	}

	CStatic ctrlText;
	DWORD dTextAlign;

};

#endif