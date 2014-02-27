/*
* Copyright (C) 2012-2014 AirDC++ Project
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

#ifndef DCPLUSPLUS_BROWSE_DLG
#define DCPLUSPLUS_BROWSE_DLG

#include "stdafx.h"
#include "WinUtil.h"

#include "../client/UpdateManagerListener.h"
#include "../client/UpdateManager.h"
#include "../client/TimerManager.h"

class BrowseDlg {
public:
	typedef COMDLG_FILTERSPEC ExtensionList;
	enum RecentType {
		TYPE_NOSAVE,
		TYPE_GENERIC,
		TYPE_SETTINGS_RESOURCES,
		TYPE_FILELIST,
		TYPE_APP,
		TYPE_LAST
	};

	enum DialogType {
		DIALOG_OPEN_FILE,
		DIALOG_SAVE_FILE,
		DIALOG_SELECT_FILE,
		DIALOG_SELECT_FOLDER
	};

	static const GUID browseGuids[TYPE_LAST];


	BrowseDlg(HWND hwnd, RecentType aRecentType, DialogType aDlgType);
	~BrowseDlg();

	bool show(tstring& target);

	bool setOkLabel(const tstring& aText);
	bool setFileName(const tstring& aName);
	bool setTitle(const tstring& aTitle);
	bool setPath(const tstring& aPath, bool forced = false);
	bool addFlags(int aFlags);
	bool setTypes(int typeCount, const COMDLG_FILTERSPEC* types);
	bool setTypeIndex(int aIndex);
private:
	DialogType type;
	bool initialized = false;

	HWND m_hwnd = nullptr;
	IFileDialog *pfd = nullptr;
};

#endif
