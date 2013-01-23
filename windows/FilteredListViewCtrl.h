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

#if !defined(FILTERED_LISTVIEW_CTRL_H)
#define FILTERED_LISTVIEW_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "TypedListViewCtrl.h"

template<class T, int ctrlId>
class FilteredListViewCtrl : public TypedListViewCtrl<T, ctrlId>
{

public:
	FilteredListViewCtrl() { }
	~FilteredListViewCtrl() {  }

	typedef TypedListViewCtrl<T, ctrlId> baseClass;

	BEGIN_MSG_MAP(thisClass)
		//NOTIFY_HANDLER(LVN_GETDISPINFO, ctrlId, OnGetItemDispInfo)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP();
};

#endif // !defined(FILTERED_LISTVIEW_CTRL_H)