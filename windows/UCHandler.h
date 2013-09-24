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

#if !defined(UC_HANDLER_H)
#define UC_HANDLER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OMenu.h"
#include "resource.h"

#include "../client/FavoriteManager.h"

template<class T>
class UCHandler {
public:
	UCHandler() { subMenu.CreatePopupMenu(); }

	typedef UCHandler<T> thisClass;
	BEGIN_MSG_MAP(thisClass)
		COMMAND_RANGE_HANDLER(IDC_USER_COMMAND, IDC_USER_COMMAND + userCommands.size(), onUserCommand)
	END_MSG_MAP()

	LRESULT onUserCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		dcassert(wID >= IDC_USER_COMMAND);
		size_t n = (size_t)wID - IDC_USER_COMMAND;
		dcassert(n < userCommands.size());

		UserCommand& uc = userCommands[n];

		T* t = static_cast<T*>(this);

		t->runUserCommand(uc);
		return 0;
	}

	void prepareMenu(OMenu& menu, int ctx, const string& hubUrl) {
		prepareMenu(menu, ctx, StringList(1, hubUrl));
	}

	void prepareMenu(OMenu& menu, int ctx, const StringList& hubs) {
		bool isOp = false;
		userCommands = FavoriteManager::getInstance()->getUserCommands(ctx, hubs, isOp);
		isOp = isOp && (ctx != UserCommand::CONTEXT_HUB);
		int n = 0;
		int m = 0;
		
		if(!userCommands.empty() || isOp) {
			subMenu.DestroyMenu();
			subMenu.m_hMenu = NULL;

			menu.AppendMenu(MF_SEPARATOR);
			
			if(SETTING(UC_SUBMENU)) {
				subMenu.CreatePopupMenu();				
				subMenu.InsertSeparatorLast(TSTRING(SETTINGS_USER_COMMANDS));
				
				menu.AppendMenu(MF_POPUP, (UINT)(HMENU)subMenu, CTSTRING(SETTINGS_USER_COMMANDS));
			}
			
			CMenuHandle cur = SETTING(UC_SUBMENU) ? subMenu.m_hMenu : menu.m_hMenu;	

			if(isOp) {
				extraItems = 5;
			} else {
				extraItems = 1;
			}

			for(const auto& uc: userCommands) {
				if(uc.getType() == UserCommand::TYPE_SEPARATOR) {
					// Avoid double separators...
					if( (cur.GetMenuItemCount() >= 1) &&
						!(cur.GetMenuState(cur.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR))
					{
						cur.AppendMenu(MF_SEPARATOR);
						m++;
					}
				} else if(uc.isRaw() || uc.isChat()) {
					cur = SETTING(UC_SUBMENU) ? subMenu.m_hMenu : menu.m_hMenu;
					for(auto i = uc.getDisplayName().begin(), iend = uc.getDisplayName().end(); i != iend; ++i) {
						tstring name = Text::toT(*i);
						if(i + 1 == iend) {
							cur.AppendMenu(MF_STRING, IDC_USER_COMMAND + n, name.c_str());
							m++;
						} else {
							bool found = false;
							TCHAR buf[1024];
							// Let's see if we find an existing item...
							for(int k = 0; k < cur.GetMenuItemCount(); k++) {
								if(cur.GetMenuState(k, MF_BYPOSITION) & MF_POPUP) {
									cur.GetMenuString(k, buf, 1024, MF_BYPOSITION);
									if(stricmp(buf, name.c_str()) == 0) {
										found = true;
										cur = (HMENU)cur.GetSubMenu(k);
									}
								}
							}
							if(!found) {
								HMENU m = CreatePopupMenu();
								cur.AppendMenu(MF_POPUP, (UINT_PTR)m, name.c_str());
								cur = m;
							}
						}
					}
				}
				n++;
			}
		}
	}

private:
	OMenu subMenu;
	UserCommand::List userCommands;
	int extraItems;
};

#endif // !defined(UC_HANDLER_H)