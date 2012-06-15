/* 
 * Copyright (C) 2003-2004 "Opera", <opera at home dot se>
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

#include "OMenu.h"
#include "WinUtil.h"
#include "BarShader.h"

namespace dcpp {

OMenu::~OMenu() {
	if (::IsMenu(m_hMenu)) {
		while(GetMenuItemCount() != 0)
			RemoveMenu(0, MF_BYPOSITION);
	} else {
		// How can we end up here??? it sure happens sometimes..
		for (auto i = items.begin(); i != items.end(); ++i) {
			delete *i;
		}
	}

	for (auto i = subMenuList.begin(); i != subMenuList.end(); ++i) {
		delete *i;
	}
	//pUnMap();
}
/*
void OMenu::pMap() {
	if (m_hMenu != NULL) {
		menus[m_hMenu] = this;
	}
}
void OMenu::pUnMap() {
	if (m_hMenu != NULL) {
		Iter i = menus.find(m_hMenu);
		dcassert(i != menus.end());
		menus.erase(i);
	}
}
*/
BOOL OMenu::CreatePopupMenu() {
	//pUnMap();
	BOOL b = CMenu::CreatePopupMenu();
	//pMap();
	return b;
}

OMenu* OMenu::createSubMenu(const tstring& aTitle) {
	OMenu* menu = new OMenu();
	subMenuList.push_back(menu);
	menu->CreatePopupMenu();
	AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)*menu, aTitle.c_str());
	return menu;
}

void OMenu::InsertSeparator(UINT uItem, BOOL byPosition, const tstring& caption, bool accels /*= false*/) {
	OMenuItem* mi = new OMenuItem();
	mi->text = caption;
	tstring::size_type i = 0;
	if (!accels)
		while ((i = mi->text.find('&')) != tstring::npos)
			mi->text.erase(i, i+1);

	if(mi->text.length() > 25) {
		mi->text = mi->text.substr(0, 25) + _T("...");
	}
	mi->parent = this;
	items.push_back(mi);
	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_FTYPE | MIIM_DATA;
	mii.fType = MFT_OWNERDRAW | MFT_SEPARATOR;
	mii.dwItemData = (ULONG_PTR)mi;
	InsertMenuItem(uItem, byPosition, &mii);
}

void OMenu::CheckOwnerDrawn(UINT uItem, BOOL byPosition) {
	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_TYPE | MIIM_DATA | MIIM_SUBMENU;
	GetMenuItemInfo(uItem, byPosition, &mii);

	if (mii.dwItemData != NULL) {
		OMenuItem* mi = (OMenuItem*)mii.dwItemData;
		if(mii.fType &= MFT_OWNERDRAW) {
			OMenuItem::Iter i = find(items.begin(), items.end(), mi);
			items.erase(i);
		}
		delete mi;
	}
}

BOOL OMenu::InsertMenuItem(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii) {
	if (lpmii->fMask & MIIM_TYPE && !(lpmii->fType & MFT_OWNERDRAW))
		if (lpmii->fMask & MIIM_DATA) {
			// If we add this MENUITEMINFO to several items we might destroy the original data, so we copy it to be sure
			MENUITEMINFO mii;
			CopyMemory(&mii, lpmii, sizeof(MENUITEMINFO));
			lpmii = &mii;
			OMenuItem* omi = new OMenuItem();
			omi->ownerdrawn = false;
			omi->data = (void*)lpmii->dwItemData;
			omi->parent = this;
			lpmii->dwItemData = (ULONG_PTR)omi;
			// Do this later on? Then we're out of scope -> mii dead -> lpmii dead pointer
			return CMenu::InsertMenuItem(uItem, bByPosition, lpmii);
		}
	return CMenu::InsertMenuItem(uItem, bByPosition, lpmii);
}

LRESULT OMenu::onInitMenuPopup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HMENU mnu = (HMENU)wParam;
	bHandled = TRUE; // Always true, since we do the following manually:
	::DefWindowProc(hWnd, uMsg, wParam, lParam);
	for (int i = 0; i < ::GetMenuItemCount(mnu); ++i) {
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_TYPE | MIIM_DATA;
		::GetMenuItemInfo(mnu, i, TRUE, &mii);
		if ((mii.fType &= MFT_OWNERDRAW) != MFT_OWNERDRAW && mii.dwItemData != NULL) {
			OMenuItem* omi = (OMenuItem*)mii.dwItemData;
			if (omi->ownerdrawn) {
				mii.fType |= MFT_OWNERDRAW;
				::SetMenuItemInfo(mnu, i, TRUE, &mii);
			}
		}
	}

	return S_OK;
}

LRESULT OMenu::onMeasureItem(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;
	if (wParam == NULL) {
		MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
		if (mis->CtlType == ODT_MENU) {
			OMenuItem* mi = (OMenuItem*)mis->itemData;
			if (mi) {
				bHandled = TRUE;
				tstring& text = mi->text;

				SIZE size;
				CalcTextSize(text, WinUtil::boldFont, &size);
				mis->itemWidth = size.cx + 4;
				mis->itemHeight = size.cy + 4;

				return TRUE;
			}
		}
	}
	return S_OK;
}

LRESULT OMenu::onDrawItem(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;
	if (wParam == NULL) {
		DRAWITEMSTRUCT dis = *(DRAWITEMSTRUCT*)lParam;
		if (dis.CtlType == ODT_MENU) {
			OMenuItem* mi = (OMenuItem*)dis.itemData;
			if (mi) {
				bHandled = TRUE;
				CRect rc(dis.rcItem);
				//rc.top += 2;
				rc.bottom -= 2;

				CDC dc;
				dc.Attach(dis.hDC);

				if (BOOLSETTING(MENUBAR_TWO_COLORS))
					OperaColors::FloodFill(dc, rc.left, rc.top, rc.right, rc.bottom, SETTING(MENUBAR_LEFT_COLOR), SETTING(MENUBAR_RIGHT_COLOR), BOOLSETTING(MENUBAR_BUMPED));
				else
					dc.FillSolidRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SETTING(MENUBAR_LEFT_COLOR));

				dc.SetBkMode(TRANSPARENT);
				dc.SetTextColor(OperaColors::TextFromBackground(SETTING(MENUBAR_LEFT_COLOR)));
				HFONT oldFont = dc.SelectFont(WinUtil::boldFont);
				tstring buf = mi->text;
				dc.DrawText(buf.c_str(), _tcslen(buf.c_str()), rc, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
				dc.SelectFont(oldFont);

				dc.Detach();
				return S_OK;
			}
		}
	}

	return S_OK;
}

} // namespace dcpp
