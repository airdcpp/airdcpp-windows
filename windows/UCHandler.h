/*
 * Copyright (C) 2001-2023 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/FavoriteManager.h>

template<class T>
class UCHandler {
public:
	UCHandler() { }
	virtual ~UCHandler() { }

	void prepareMenu(OMenu& menu, int ctx, const string& hubUrl) {
		prepareMenu(menu, ctx, StringList(1, hubUrl));
	}

	void prepareMenu(OMenu& menu, int ctx, const StringList& hubs) {
		bool isOp = false;
		userCommands = FavoriteManager::getInstance()->getUserCommands(ctx, hubs, isOp);
		isOp = isOp && (ctx != UserCommand::CONTEXT_HUB);
		
		if(!userCommands.empty()) {
			OMenu* subMenu = &menu;
			menu.appendSeparator();

			if(SETTING(UC_SUBMENU)) {
				subMenu = menu.createSubMenu(TSTRING(SETTINGS_USER_COMMANDS), true);
			}
			
			OMenu* cur = subMenu;

			for (size_t n = 0; n < userCommands.size(); ++n) {
				UserCommand* uc = &userCommands[n];
				if(uc->getType() == UserCommand::TYPE_SEPARATOR) {
					if(cur->hasItems()) {
						cur->appendSeparator();
					}
				} else if(uc->isRaw() || uc->isChat()) {
					cur = subMenu;
					for(auto i = uc->getDisplayName().begin(), iend = uc->getDisplayName().end(); i != iend; ++i) {
						tstring name = Text::toT(*i);
						if(i + 1 == iend) {
							cur->appendItem(name, [=] { static_cast<T*>(this)->runUserCommand(*uc); });
						} else {
							bool found = false;
							// Let's see if we find an existing item...
							for(int k = 0; k < cur->GetMenuItemCount(); k++) {
								if(cur->isPopup(k) && stricmp(cur->getText(k), name) == 0) {
									found = true;
									cur = cur->getChild(k);
								}
							}
							if(!found) {
								cur = cur->createSubMenu(name);
							}
						}
					}
				}
			}
		}
	}

private:
	UserCommand::List userCommands;
};

#endif // !defined(UC_HANDLER_H)