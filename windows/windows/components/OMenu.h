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

#ifndef __OMENU_H
#define __OMENU_H

#include <airdcpp/core/header/typedefs.h>
#include <forward.h>

#include <windows/Dispatchers.h>

namespace wingui {

#define EXT_CONTEXT_MENU(menu, menuId, tokens) \
if (!tokens.empty()) { \
	auto cmm = &webserver::WebServerManager::getInstance()->getContextMenuManager(); \
	auto listData = webserver::ContextMenuItemListData({ webserver::ContextMenuManager::URLS_SUPPORT }, { webserver::Access::ADMIN }, this); \
	auto extMenuItems = cmm->get##menuId##Menu(tokens, listData); \
	OMenu::appendExtensionMenuItems(menu, extMenuItems, [=](const auto& aClickData) {  \
		cmm->onClick##menuId##Item(tokens, aClickData); \
	}); \
}

#define EXT_CONTEXT_MENU_ENTITY(menu, menuId, tokens, entity) \
if (!tokens.empty()) { \
	auto cmm = &webserver::WebServerManager::getInstance()->getContextMenuManager(); \
	auto listData = webserver::ContextMenuItemListData({ webserver::ContextMenuManager::URLS_SUPPORT }, { webserver::Access::ADMIN }, this); \
	auto extMenuItems = cmm->get##menuId##Menu(tokens, listData, entity); \
	OMenu::appendExtensionMenuItems(menu, extMenuItems, [=](const auto& aClickData) {  \
		cmm->onClick##menuId##Item(tokens, aClickData, entity); \
	}); \
}

/*
 * Wouldn't it be Wonderful if WTL made their functions virtual? Yes it would...
 */
class OMenu : public CMenu {
public:
	struct OMenuItem {
		using List = vector<unique_ptr<OMenuItem>>;

		OMenuItem() = default;

		tstring text;
		OMenu* parent = nullptr;
		void* data = nullptr;
		bool ownerdrawn = true;
		Dispatcher::F f;
	};

	enum {
		FLAG_DEFAULT	= 0x01,
		FLAG_THREADED	= 0x02,
		FLAG_CHECKED	= 0x04,
		FLAG_DISABLED	= 0x08,
	};

	explicit OMenu(OMenu* aParent = nullptr);
	~OMenu();

	BOOL CreatePopupMenu();

	void InsertSeparator(UINT uItem, BOOL byPosition, const tstring& caption, bool accels = false);
	inline void InsertSeparatorFirst(const tstring& caption/*, bool accels = false*/) {
		InsertSeparator(0, TRUE, caption);
	}
	inline void InsertSeparatorLast(const tstring& caption/*, bool accels = false*/) {
		InsertSeparator(GetMenuItemCount(), TRUE, caption);
	}

	using ExtensionMenuItemClickHandler = std::function<void (const webserver::ContextMenuItemClickData &)>;
	static void appendExtensionMenuItems(OMenu& menu_, const webserver::GroupedContextMenuItemList& aItems, const ExtensionMenuItemClickHandler& aClickHandler) noexcept;

	void appendSeparator();

	/* These should be used when submenus are created within a separate function/scope */
	OMenu* createSubMenu(const tstring& aTitle, bool appendSeparator = false, bool isShellMenu = false);
	OMenu* getMenu();

	OMenu* getChild(unsigned position);

	void appendThis(const tstring& aTitle, bool appendSeparator = false);

	void CheckOwnerDrawn(UINT uItem, BOOL byPosition);

	inline void RemoveFirstItem() {
		RemoveMenu(0, MF_BYPOSITION);
	}
	inline void RemoveFirstItem(int amount) {
		for (int i = 0; i < amount; ++i) {
			RemoveMenu(0, MF_BYPOSITION);
		}
	}
	BOOL DeleteMenu(UINT nPosition, UINT nFlags);
	BOOL RemoveMenu(UINT nPosition, UINT nFlags);

	inline void ClearMenu() {
		RemoveFirstItem(GetMenuItemCount());
	}

	unsigned appendItem(const tstring& text, const Dispatcher::F& f = Dispatcher::F(), int aFlags = 0);
	BOOL InsertMenuItem(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii);

	virtual void open(HWND aHWND, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON, CPoint pt = GetMessagePos());

	unsigned getNextID();
	void addItem(OMenuItem* mi);
	bool hasItems();
	bool isPopup(unsigned index);
	bool isSeparator(unsigned index);
	tstring getText(unsigned index) const;

	static LRESULT onMeasureItem(HWND hWnd, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT onDrawItem(HWND hWnd, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	void disableEmptyMenus();
	void checkCommand(HWND aHWND, unsigned cmd);
private:
	//friend class CShellContextMenu;
	unsigned start;
	OMenuItem::List items;
	vector<unique_ptr<OMenu>> subMenuList;

	static void CalcTextSize(const tstring& text, HFONT font, LPSIZE size) {
		HDC dc = CreateCompatibleDC(NULL);
		HGDIOBJ old = SelectObject(dc, font);
		::GetTextExtentPoint32(dc, text.c_str(), min((int)_tcslen(text.c_str()), (int)8192), size);
		SelectObject(dc, old);
		DeleteDC(dc);
	}

	OMenu* parent;
	bool defaultSet = false;
	bool isShellmenu = false;

	static void appendExtensionMenuItemGroup(OMenu& menu_, const webserver::ContextMenuItemList& aItems, const ExtensionMenuItemClickHandler& aClickHandler) noexcept;
};

#define MESSAGE_HANDLER_HWND(msg, func) \
	if(uMsg == msg) \
	{ \
		bHandled = TRUE; \
		lResult = func(hWnd, uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

}
#endif // __OMENU_H
