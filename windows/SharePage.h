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

#if !defined(SHARE_PAGE_H)
#define SHARE_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>

#include "ShareDirectories.h"
#include "PropPage.h"

#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"

class SharePage : public CPropertyPage<IDD_SHAREPAGE>, public PropPage, public SharePageBase
{
public:
	SharePage(SettingsManager *s);
	~SharePage();

	BEGIN_MSG_MAP_EX(SharePage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)
		COMMAND_HANDLER(IDC_PROFILE_SEL, CBN_SELCHANGE , onProfileChanged)
		COMMAND_HANDLER(IDC_ADD_PROFILE, BN_CLICKED, onClickedAddProfile)
		COMMAND_ID_HANDLER(IDC_SET_DEFAULT, onClickedDefault)
		COMMAND_ID_HANDLER(IDC_REMOVE_PROFILE, onClickedRemoveProfile)
		COMMAND_ID_HANDLER(IDC_RENAME_PROFILE, onClickedRenameProfile)
		COMMAND_ID_HANDLER(IDC_APPLY_CHANGES, onApplyChanges)
		COMMAND_ID_HANDLER(IDC_EDIT_TEMPSHARES, onEditTempShares)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedAddProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRemoveProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRenameProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onProfileChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditTempShares(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onApplyChanges(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	Dispatcher::F getThreadedTask();
protected:
	void setProfileList();
	void handleAddProfile(bool copy);

	ProfileToken getCurProfile() { return curProfile; }
	ProfileToken getDefaultProfile() { return defaultProfile; }

	unique_ptr<ShareDirectories> dirPage;
	ProfileToken curProfile;
	ProfileToken defaultProfile;

	friend class FolderTree;
	static TextItem texts[];

	void fixControls();

	CComboBox ctrlProfile;
	CButton ctrlAddProfile;

	void applyChanges(bool isQuit, bool resetAdcHubs);

	bool hasChanged();
};

#endif // !defined(SHARE_PAGE_H)
