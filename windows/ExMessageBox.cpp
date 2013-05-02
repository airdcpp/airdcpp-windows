/*
 * Copyright (C) 2010 Crise, crise<at>mail.berlios.de
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

// @todo Smarter width calculation for Vista+ in CheckMessageBoxProc

#include "stdafx.h"

#include "ExMessageBox.h"
#include "WinUtil.h"

ExMessageBox::MessageBoxValues ExMessageBox::mbv = {0};

// Helper function for CheckMessageBoxProc
BOOL WINAPI ScreenToClient(HWND hWnd, LPRECT lpRect)
{
	if (!::ScreenToClient(hWnd, (LPPOINT)lpRect))
		return FALSE;
	return ::ScreenToClient(hWnd, ((LPPOINT)lpRect) + 1);
}

/**
 * Below CheckMessageBoxProc adds everyones favorite "don't show again" checkbox to the dialog
 * much of the layout code (especially for XP and older windows versions) is copied with changes
 * from a GPL'ed project emabox at SourceForge.
 **/
LRESULT CALLBACK CheckMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == 2025)
			{
				const LRESULT res = SendMessage((HWND)lParam, BM_GETSTATE, 0, 0);
				bool bCheckedAfter = ((res & BST_CHECKED) == 0);
				
				// Update usedata
				ExMessageBox::SetUserData((void*)(bCheckedAfter ? BST_CHECKED : BST_UNCHECKED));
				
				SendMessage((HWND)lParam, BM_SETCHECK, bCheckedAfter ? BST_CHECKED : BST_UNCHECKED, 0);
			}
		}
		break;
		case WM_ERASEBKGND:
		{
			// Vista+ has grey strip
			if (Util::getOsMajor() >= 6)
			{
				RECT rc = {0};
				HDC dc = (HDC)wParam;
				
				// Fill the entire dialog
				GetClientRect(hWnd, &rc);
				FillRect(dc, &rc, GetSysColorBrush(COLOR_WINDOW));
				
				// Calculate strip height
				RECT rcButton = {0};
				GetWindowRect(FindWindowEx(hWnd, NULL, L"BUTTON", NULL), &rcButton);
				int stripHeight = (rcButton.bottom - rcButton.top) + 24;
				
				// Fill the strip
				rc.top += (rc.bottom - rc.top) - stripHeight;
				FillRect(dc, &rc, GetSysColorBrush(COLOR_3DFACE));
				
				// Make a line
				HGDIOBJ oldPen = SelectObject(dc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT)));
				MoveToEx(dc, rc.left - 1, rc.top, (LPPOINT)NULL);
				LineTo(dc, rc.right, rc.top);
				DeleteObject(SelectObject(dc, oldPen));
				return S_OK;
			}
		}
		break;
		case WM_CTLCOLORSTATIC:
		{
			// Vista+ has grey strip
			if ((Util::getOsMajor() >= 6) && ((HWND)lParam == GetDlgItem(hWnd, 2025)))
			{
				HDC hdc = (HDC)wParam;
				SetBkMode(hdc, TRANSPARENT);
				return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
			}
		}
		break;
		case WM_INITDIALOG:
		{
			RECT rc = {0};
			HWND current = NULL;
			int iWindowWidthBefore;
			int iWindowHeightBefore;
			int iClientHeightBefore;
			int iClientWidthBefore;
			
			pair<LPCTSTR, UINT> checkdata = (*(pair<LPCTSTR, UINT>*)ExMessageBox::GetUserData());
			
			GetClientRect(hWnd, &rc);
			iClientHeightBefore = rc.bottom - rc.top;
			iClientWidthBefore = rc.right - rc.left;
			
			GetWindowRect(hWnd, &rc);
			iWindowWidthBefore = rc.right - rc.left;
			iWindowHeightBefore = rc.bottom - rc.top;
			
			// Create checkbox (resized and moved later)
			HWND check = CreateWindow(L"BUTTON", checkdata.first, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_VCENTER | BS_CHECKBOX,
			                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			                          hWnd, (HMENU)2025, GetModuleHandle(NULL), NULL
			                         );
			                         
			// Assume checked by default
			SendMessage(check, BM_SETCHECK, checkdata.second, 0);
			ExMessageBox::SetUserData((void*)checkdata.second); 
			
			// Apply default font
			const int cyMenuSize = GetSystemMetrics(SM_CYMENUSIZE);
			const int cxMenuSize = GetSystemMetrics(SM_CXMENUSIZE);
			const HFONT hNewFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			HFONT hOldFont;
			SIZE size;
			
			SendMessage(check, WM_SETFONT, (WPARAM)hNewFont, (LPARAM)TRUE);
			
			// Get the size of the checkbox
			HDC hdc = GetDC(check);
			hOldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
			GetTextExtentPoint32(hdc, checkdata.first, wcslen(checkdata.first), &size);
			SelectObject(hdc, hOldFont);
			ReleaseDC(check, hdc);
			
			// Checkbox dimensions
			int iCheckboxWidth = cxMenuSize + size.cx + 1;
			int iCheckboxHeight = (cyMenuSize > size.cy) ? cyMenuSize : size.cy;
			
			// Vista+ has a different kind of layout altogether
			if (Util::getOsMajor() >= 6)
			{
				// Align checkbox with buttons (aproximately)
				int iCheckboxTop = int(iClientHeightBefore - (iCheckboxHeight * 1.70));
				MoveWindow(check, 5, iCheckboxTop, iCheckboxWidth, iCheckboxHeight, FALSE);
				
				// Resize and re-center dialog
				int iWindowWidthAfter = iWindowWidthBefore + iCheckboxWidth;
				int iWindowLeftAfter = rc.left + (iWindowWidthBefore - iWindowWidthAfter) / 2;
				MoveWindow(hWnd, iWindowLeftAfter, rc.top, iWindowWidthAfter, iWindowHeightBefore, TRUE);
				
				// Go through the buttons and move them
				while ((current = FindWindowEx(hWnd, current, L"BUTTON", NULL)) != NULL)
				{
					if (current == check) continue;
					
					RECT rc;
					GetWindowRect(current, &rc);
					ScreenToClient(hWnd, &rc);
					MoveWindow(current, rc.left + iCheckboxWidth, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
				}
			}
			else
			{
				RECT rt = {0}, rb = {0};
				
				// Let's find us the label
				while ((current = FindWindowEx(hWnd, current, L"STATIC", NULL)) != NULL)
				{
					if (GetWindowTextLength(current) > 0)
					{
						GetWindowRect(current, &rt);
						ScreenToClient(hWnd, &rt);
						current = NULL;
						break;
					}
				}
				
				// For correcting width, here just to make lines shorter
				int iWidthAdjustment = (rt.left + iCheckboxWidth) - iWindowWidthBefore;
				
				// Go through the buttons and move them
				current = NULL;
				while ((current = FindWindowEx(hWnd, current, L"BUTTON", NULL)) != NULL)
				{
					if (current == check) continue;
					
					GetWindowRect(current, &rb);
					ScreenToClient(hWnd, &rb);
					MoveWindow(current, rb.left + (iWidthAdjustment > 0 ? (iWidthAdjustment + 15) / 2 : 0), rb.top + iCheckboxHeight, rb.right - rb.left, rb.bottom - rb.top, FALSE);
				}
				
				// Move the checkbox
				int iCheckboxTop = rt.top + (rt.bottom - rt.top) + ((rb.top - rt.bottom) / 2);
				MoveWindow(check, rt.left, iCheckboxTop, iCheckboxWidth, iCheckboxHeight, FALSE);
				
				// Resize and re-center dialog
				int iWindowHeightAfter = iWindowHeightBefore + iCheckboxHeight;
				int iWindowTopAfter = rc.top + (iWindowHeightBefore - iWindowHeightAfter) / 2;
				int iWindowWidthAfter = (iWidthAdjustment > 0) ? iWindowWidthBefore + iWidthAdjustment + 15 : iWindowWidthBefore;
				int iWindowLeftAfter = rc.left + (iWindowWidthBefore - iWindowWidthAfter) / 2;
				MoveWindow(hWnd, iWindowLeftAfter, iWindowTopAfter, iWindowWidthAfter, iWindowHeightAfter, TRUE);
			}
		}
		break;
	}
	
	return CallWindowProc(ExMessageBox::GetMessageBoxProc(), hWnd, uMsg, wParam, lParam);
}

// Overload the standard MessageBox for convenience
int WINAPI MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, WNDPROC wndProc)
{
	if (!wndProc)
		return MessageBox(hWnd, lpText, lpCaption, uType);
	return ExMessageBox::Show(hWnd, lpText, lpCaption, uType, wndProc);
}

int WINAPI MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, LPCTSTR lpQuestion, UINT uType, UINT& uCheck)
{
	pair<LPCTSTR, UINT> data = make_pair(lpQuestion, uCheck);
	ExMessageBox::SetUserData(&data);
	int nRet = ExMessageBox::Show(hWnd, lpText, lpCaption, uType, CheckMessageBoxProc);
	uCheck = (UINT)ExMessageBox::GetUserData();
	ExMessageBox::SetUserData(NULL);
	return nRet;
}