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

#ifndef MDI_COMMAND_BAR_H
#define MDI_COMMAND_BAR_H

#include <windows/stdafx.h>
#include <windows/util/DarkMode.h>

namespace wingui {

// MDI command bar that draws the menus and the buttons of a maximized child window with the colors of the current window theme
// (the visual styles have no dark variant of the buttons)
class MDICommandBar : public CMDICommandBarCtrlImpl<MDICommandBar> {
public:
	DECLARE_WND_SUPERCLASS(_T("AirDC_MDICommandBar"), GetWndClassName())

	typedef CMDICommandBarCtrlImpl<MDICommandBar> baseClass;

	BEGIN_MSG_MAP(MDICommandBar)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(1) // Parent window messages
		NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, onParentCustomDraw)
		CHAIN_MSG_MAP_ALT(baseClass, 1)
	ALT_MSG_MAP(2) // MDI client window messages
		CHAIN_MSG_MAP_ALT(baseClass, 2)
	ALT_MSG_MAP(3) // Message hook messages
		CHAIN_MSG_MAP_ALT(baseClass, 3)
	END_MSG_MAP()

	// The menu bar items are drawn with the system colors
	LRESULT onParentCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		if (!DarkMode::isEnabled() || pnmh->hwndFrom != m_hWnd) {
			bHandled = FALSE;
			return 0;
		}

		auto tbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(pnmh);
		if (tbcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
			return CDRF_NOTIFYITEMDRAW;
		}

		if (tbcd->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) {
			bHandled = FALSE;
			return 0;
		}

		const auto state = tbcd->nmcd.uItemState;
		const bool disabled = (state & CDIS_DISABLED) != 0;
		if (!disabled && (state & (CDIS_HOT | CDIS_SELECTED))) {
			CDCHandle dc = tbcd->nmcd.hdc;
			auto oldBrush = dc.SelectBrush(DarkMode::getSysColorBrush(COLOR_MENUHILIGHT));
			auto oldPen = dc.SelectStockPen(NULL_PEN);
			const auto& rc = tbcd->nmcd.rc;
			dc.RoundRect(rc.left, rc.top, rc.right + 1, rc.bottom + 1, 6, 6);
			dc.SelectPen(oldPen);
			dc.SelectBrush(oldBrush);
		}

		tbcd->clrText = disabled ? DarkMode::getSysColor(COLOR_GRAYTEXT) : RGB(255, 255, 255);
		tbcd->nStringBkMode = TRANSPARENT;
		_ParentCustomDrawHelper(tbcd);
		return CDRF_SKIPDEFAULT;
	}

	// The menus are owner drawn with the system colors
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
		if (!DarkMode::isEnabled()) {
			CMDICommandBarCtrlImpl<MDICommandBar>::DrawItem(lpDrawItemStruct);
			return;
		}

		auto pmd = reinterpret_cast<_MenuItemData*>(lpDrawItemStruct->itemData);
		CDCHandle dc = lpDrawItemStruct->hDC;
		const RECT& rcItem = lpDrawItemStruct->rcItem;

		const bool disabled = (lpDrawItemStruct->itemState & ODS_GRAYED) != 0;
		const bool checked = (lpDrawItemStruct->itemState & ODS_CHECKED) != 0;
		const bool selected = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0 && LOWORD(lpDrawItemStruct->itemID) != static_cast<WORD>(-1);

		auto bgBrush = DarkMode::getSysColorBrush(COLOR_MENU);
		dc.FillRect(&rcItem, bgBrush);
		if (selected && !disabled) {
			// Rounded selection like in the native menus
			const int inset = m_szBitmap.cx / 4;
			auto oldBrush = dc.SelectBrush(DarkMode::getSysColorBrush(COLOR_MENUHILIGHT));
			auto oldPen = dc.SelectStockPen(NULL_PEN);
			dc.RoundRect(rcItem.left + inset, rcItem.top + 1, rcItem.right - inset + 1, rcItem.bottom, inset * 2, inset * 2);
			dc.SelectPen(oldPen);
			dc.SelectBrush(oldBrush);
		}

		if (pmd->fType & MFT_SEPARATOR) {
			RECT rc = rcItem;
			rc.left += m_szButton.cx;
			rc.top += (rc.bottom - rc.top) / 2;
			rc.bottom = rc.top + 1;
			dc.FillRect(&rc, DarkMode::getSysColorBrush(COLOR_3DSHADOW));
			return;
		}

		// Image or check mark
		RECT rcButn = { rcItem.left, rcItem.top, rcItem.left + m_szButton.cx, rcItem.top + m_szButton.cy };
		::OffsetRect(&rcButn, 0, ((rcItem.bottom - rcItem.top) - (rcButn.bottom - rcButn.top)) / 2);

		if (pmd->iButton >= 0) {
			if (checked) {
				RECT rcCheck = rcButn;
				::InflateRect(&rcCheck, -1, -1);
				dc.FrameRect(&rcCheck, ::GetSysColorBrush(COLOR_HIGHLIGHT));
			}

			POINT point = { rcButn.left + (rcButn.right - rcButn.left - m_szBitmap.cx) / 2, rcButn.top + (rcButn.bottom - rcButn.top - m_szBitmap.cy) / 2 };
			if (!disabled) {
				::ImageList_Draw(m_hImageList, pmd->iButton, dc, point.x, point.y, ILD_TRANSPARENT);
			} else {
				DrawBitmapDisabled(dc, pmd->iButton, point, bgBrush, bgBrush, DarkMode::getSysColorBrush(COLOR_3DSHADOW));
			}
		} else if (checked) {
			CMenuItemInfo info;
			info.fMask = MIIM_TYPE;
			::GetMenuItemInfo(reinterpret_cast<HMENU>(lpDrawItemStruct->hwndItem), lpDrawItemStruct->itemID, MF_BYCOMMAND, &info);

			// Check mark and bullet glyphs of the Marlett font
			CFont marlett;
			marlett.CreateFont(m_szBitmap.cy, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SYMBOL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Marlett"));
			auto oldFont = dc.SelectFont(marlett);
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(DarkMode::getSysColor(disabled ? COLOR_GRAYTEXT : COLOR_MENUTEXT));
			dc.DrawText((info.fType & MFT_RADIOCHECK) ? _T("h") : _T("a"), 1, &rcButn, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			dc.SelectFont(oldFont);
		}

		// Text
		RECT rcText = rcItem;
		rcText.left += m_szButton.cx + s_kcxGap + s_kcxTextMargin;
		rcText.right -= m_szButton.cx;
		dc.SetBkMode(TRANSPARENT);
		DrawMenuText(dc, rcText, pmd->lpstrText, disabled ? DarkMode::getSysColor(COLOR_GRAYTEXT) : selected ? RGB(255, 255, 255) : DarkMode::getSysColor(COLOR_MENUTEXT));
	}

	void _DrawMDIButton(CWindowDC& dc, LPRECT pRects, int nBtn) {
		if (!DarkMode::isEnabled()) {
			CMDICommandBarCtrlImpl<MDICommandBar>::_DrawMDIButton(dc, pRects, nBtn);
			return;
		}

		for (int i = 0; i < 3; ++i) {
			if (nBtn != -1 && nBtn != i) {
				continue;
			}

			const auto& rc = pRects[i];
			dc.FillRect(&rc, DarkMode::getSysColorBrush(m_nBtnPressed == i ? COLOR_3DSHADOW : COLOR_3DFACE));

			CPen pen;
			pen.CreatePen(PS_SOLID, 1, DarkMode::getSysColor(m_bParentActive ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
			auto oldPen = dc.SelectPen(pen);

			const int size = (std::min(rc.right - rc.left, rc.bottom - rc.top) / 3) & ~1;
			const int x = (rc.left + rc.right) / 2 - size / 2;
			const int y = (rc.top + rc.bottom) / 2 - size / 2;

			switch (i) {
				case 0: // close
					dc.MoveTo(x, y);
					dc.LineTo(x + size + 1, y + size + 1);
					dc.MoveTo(x + size, y);
					dc.LineTo(x - 1, y + size + 1);
					break;
				case 1: // restore
				{
					const int offset = std::max(2, size / 4);
					auto oldBrush = dc.SelectStockBrush(NULL_BRUSH);
					dc.Rectangle(x, y + offset, x + size - offset + 1, y + size + 1);
					dc.SelectBrush(oldBrush);
					dc.MoveTo(x + offset, y + offset);
					dc.LineTo(x + offset, y);
					dc.LineTo(x + size, y);
					dc.LineTo(x + size, y + size - offset);
					dc.LineTo(x + size - offset, y + size - offset);
					break;
				}
				case 2: // minimize
					dc.MoveTo(x, y + size / 2);
					dc.LineTo(x + size + 1, y + size / 2);
					break;
			}

			dc.SelectPen(oldPen);
		}
	}
};

}

#endif // !defined(MDI_COMMAND_BAR_H)
