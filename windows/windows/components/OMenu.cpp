/* 
 * Copyright (C) 2003-2004 "Opera", <opera at home dot se>
 *
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

#include <windows/stdafx.h>

#include <windows/util/ActionUtil.h>
#include <windows/util/WinUtil.h>
#include <windows/components/BarShader.h>
#include <windows/MainFrm.h>

#include <web-server/ContextMenuManager.h>

#define EXTENSION_GROUP_LIMIT 2


namespace wingui {
OMenu::~OMenu() {
	if (::IsMenu(m_hMenu)) {
		while(GetMenuItemCount() != 0)
			RemoveMenu(0, MF_BYPOSITION);
	}
}

OMenu::OMenu(OMenu* aParent /*nullptr*/) : CMenu(), start(IDC_MENURANGE), parent(aParent) { 
	if (!parent) {
		MENUINFO mi = { sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS };
		::SetMenuInfo(m_hMenu, &mi);
	}
}

BOOL OMenu::DeleteMenu(UINT nPosition, UINT nFlags) {
	//CheckOwnerDrawn(nPosition, nFlags & MF_BYPOSITION);
	return CMenu::DeleteMenu(nPosition, nFlags);
}
BOOL OMenu::RemoveMenu(UINT nPosition, UINT nFlags) {
	//CheckOwnerDrawn(nPosition, nFlags & MF_BYPOSITION);
	return CMenu::RemoveMenu(nPosition, nFlags);
}

BOOL OMenu::CreatePopupMenu() {
	BOOL b = CMenu::CreatePopupMenu();
	return b;
}

OMenu* OMenu::createSubMenu(const tstring& aTitle, bool appendSeparator /*false*/, bool aIsShellMenu /*false*/) {
	auto menu = getMenu();
	menu->appendThis(aTitle, appendSeparator);
	menu->isShellmenu = aIsShellMenu;
	return menu;
}

OMenu* OMenu::getMenu() {
	OMenu* menu = new OMenu(this);
	subMenuList.push_back(unique_ptr<OMenu>(menu));
	menu->CreatePopupMenu();
	return menu;
}

bool OMenu::hasItems() {
	return GetMenuItemCount() > 0;
}

bool OMenu::isPopup(unsigned index) {
	return (GetMenuState(index, MF_BYPOSITION) & MF_POPUP) == MF_POPUP;
}

bool OMenu::isSeparator(unsigned index) {
	//Items that open a sub menu also seem to have MF_SEPARATOR flag...
	return !isPopup(index) && (GetMenuState(index, MF_BYPOSITION) & MF_SEPARATOR) == MF_SEPARATOR;
}

tstring OMenu::getText(unsigned index) const {
	TCHAR buf[1024];
	GetMenuString(index, buf, 1024, MF_BYPOSITION);
	return buf;
}

OMenu* OMenu::getChild(unsigned position) {
	HMENU h = GetSubMenu(position);
	for (size_t i = 0, n = subMenuList.size(); i < n; ++i) {
		auto menu = subMenuList[i].get();
		if (menu->m_hMenu == h) {
			return menu;
		}
	}
	return nullptr;
}


void OMenu::appendThis(const tstring& aTitle, bool appendSeparator /*false*/) {
	dcassert(parent);
	parent->AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)*this, aTitle.c_str());
	if (appendSeparator)
		InsertSeparatorFirst(aTitle);
}

void OMenu::appendSeparator() {
	// Avoid double separators...
	if (GetMenuItemCount() > 0 && !isSeparator(GetMenuItemCount() - 1))
		AppendMenu(MF_SEPARATOR);
}

void OMenu::appendExtensionMenuItemGroup(OMenu& menu_, const webserver::ContextMenuItemList& aItems, const ExtensionMenuItemClickHandler& aClickHandler) noexcept {
	if (aItems.empty()) {
		return;
	}

	for (const auto& extItem: aItems) {
		if (!extItem->getChildren().empty()) {
			auto subMenu = menu_.createSubMenu(Text::toT(extItem->getTitle()));
			appendExtensionMenuItemGroup(*subMenu, extItem->getChildren(), aClickHandler);
		} else {
			menu_.appendItem(
				Text::toT(extItem->getTitle()),
				[=]() {
					if (!extItem->getUrls().empty()) {
						for (const auto& url : extItem->getUrls()) {
							ActionUtil::openLink(Text::toT(url));
						}

						return;
					}

					auto clickData = webserver::ContextMenuItemClickData(
						extItem->getHook().getId(),
						extItem->getId(),
						{ webserver::ContextMenuManager::URLS_SUPPORT },
						{ webserver::Access::ADMIN },
						webserver::SettingValueMap()
					);

					aClickHandler(clickData);
				},
				OMenu::FLAG_THREADED
			);
		};
	}
}

void OMenu::appendExtensionMenuItems(OMenu& menu_, const webserver::GroupedContextMenuItemList& aItems, const ExtensionMenuItemClickHandler& aClickHandler) noexcept {
	if (aItems.empty()) {
		return;
	}

	for (const auto& group: aItems) {
		auto& groupMenu = group->getItems().size() > EXTENSION_GROUP_LIMIT ? *menu_.createSubMenu(Text::toT(group->getTitle())) : menu_;
		appendExtensionMenuItemGroup(groupMenu, group->getItems(), aClickHandler);
	}

	menu_.appendSeparator();
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
	addItem(mi);
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

	/*if (mii.dwItemData) {
		auto mi = (OMenuItem*)mii.dwItemData;
		if(mii.fType &= MFT_OWNERDRAW) {
			auto i = find_if(items.begin(), items.end(), [mi](unique_ptr<OMenuItem> i) { return i.get() == mi; });
			items.erase(i);
		}
	}*/
}

BOOL OMenu::InsertMenuItem(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii) {
	if (lpmii->fMask & MIIM_TYPE && !(lpmii->fType & MFT_OWNERDRAW)) {
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
	}

	return CMenu::InsertMenuItem(uItem, bByPosition, lpmii);
}

void OMenu::open(HWND aHWND, unsigned flags /*= TPM_LEFTALIGN | TPM_RIGHTBUTTON*/, CPoint pt /*= GetMessagePos()*/) {
	// Check empty submenus
	disableEmptyMenus();

	// No end separators
	while (GetMenuItemCount() > 0 && isSeparator(GetMenuItemCount() - 1)) {
		RemoveMenu(GetMenuItemCount() - 1, MF_BYPOSITION);
	}

	flags |= TPM_RETURNCMD;
	unsigned ret = ::TrackPopupMenu(m_hMenu, flags, pt.x, pt.y, 0, aHWND, 0);
	checkCommand(aHWND, ret);
}

void OMenu::disableEmptyMenus() {
	for (auto& m: subMenuList) {
		if (m->isShellmenu)
			continue;

		if (m->hasItems()) {
			if (m->GetMenuItemCount() > 1 || !m->isSeparator(0))
				continue;
		}
		
		EnableMenuItem((UINT_PTR)(HMENU)*m.get(), MFS_DISABLED);
	}
}

void OMenu::checkCommand(HWND aHWND, unsigned cmd){
	if(cmd >= start) {
		if(cmd - start < items.size()) {
			items[cmd - start].get()->f();
			return;
		}
	}

	//not found
	::PostMessage(aHWND, WM_COMMAND, cmd, NULL);
}

unsigned OMenu::getNextID() { 
	return parent ? parent->getNextID() : start + items.size(); 
}

void OMenu::addItem(OMenuItem* mi) {
	parent ? parent->addItem(mi) : items.push_back(unique_ptr<OMenuItem>(mi));
}

unsigned OMenu::appendItem(const tstring& text, const Dispatcher::F& f /*nullptr*/, int flags /*0*/) {
	// init structure for new item
	CMenuItemInfo info;
	info.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	info.fType = MFT_STRING;

	if(flags & FLAG_DISABLED) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DISABLED;
	}
	if (flags & FLAG_DEFAULT && !defaultSet) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DEFAULT;
		defaultSet = true;
	}

	if(flags & FLAG_CHECKED) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_CHECKED;
	}

	info.dwTypeData = const_cast< LPTSTR >( text.c_str() );
	const auto index = GetMenuItemCount();

	info.wID = getNextID();

	if(f) {
		OMenuItem* mi = new OMenuItem();
		mi->ownerdrawn = false;
		if (flags & FLAG_THREADED) {
			mi->f = [=] { MainFrame::getMainFrame()->addThreadedTask(f); };
		} else {
			mi->f = f;
		}

		//mi->f = (flags & FLAG_THREADED) ? [=] { MainFrame::getMainFrame()->addThreadedTask(f); } : f;
		mi->text = text;
		mi->parent = this;

		addItem(mi);

		info.dwItemData = reinterpret_cast<ULONG_PTR>(mi);
	}

	CMenu::InsertMenuItem(index, TRUE, &info);
	return index;
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
			if (mi && mi->ownerdrawn) {
				bHandled = TRUE;
				CRect rc(dis.rcItem);
				//rc.top += 2;
				rc.bottom -= 2;

				CDC dc;
				dc.Attach(dis.hDC);

				if (SETTING(MENUBAR_TWO_COLORS))
					OperaColors::FloodFill(dc, rc.left, rc.top, rc.right, rc.bottom, SETTING(MENUBAR_LEFT_COLOR), SETTING(MENUBAR_RIGHT_COLOR), SETTING(MENUBAR_BUMPED));
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
}