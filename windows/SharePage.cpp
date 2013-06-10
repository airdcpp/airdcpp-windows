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

#include "stdafx.h"

#include "../client/Util.h"
#include "../client/version.h"
#include "../client/ClientManager.h"
#include "../client/FavoriteManager.h"

#include "Resource.h"
#include "SharePage.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PropertiesDlg.h"
#include "TempShare_dlg.h"
#include "SharePageDlg.h"
#include "MainFrm.h"

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/range/algorithm/find_if.hpp>

PropPage::TextItem SharePage::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_ADD_PROFILE, ResourceManager::ADD_PROFILE_DOTS },
	//{ IDC_ADD_PROFILE_COPY, ResourceManager::ADD_PROFILE_COPY },
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_RENAME_PROFILE, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

SharePage::SharePage(SettingsManager *s) : PropPage(s), dirPage(unique_ptr<ShareDirectories>(new ShareDirectories(this, s))) {
	SetTitle(CTSTRING(SETTINGS_SHARINGPAGE));
	m_psp.dwFlags |= PSP_RTLREADING;
}

SharePage::~SharePage() { }

LRESULT SharePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	GetDlgItem(IDC_EDIT_TEMPSHARES).EnableWindow((ShareManager::getInstance()->hasTempShares()) ? 1 : 0);
	ctrlAddProfile.Attach(GetDlgItem(IDC_ADD_PROFILE));

	curProfile = SP_DEFAULT;

	ctrlProfile.Attach(GetDlgItem(IDC_PROFILE_SEL));
	auto profileList = ShareManager::getInstance()->getProfiles();
	int n = 0;
	for(const auto& sp: profileList) {
		if (sp->getToken() != SP_HIDDEN) {
			ctrlProfile.InsertString(n, Text::toT(sp->getDisplayName()).c_str());
			n++;
			profiles.push_back(sp);
		}
	}

	ctrlProfile.SetCurSel(0);

	dirPage->Create(this->m_hWnd);
	//CRect rc;
	//::GetWindowRect(GetDlgItem(IDC_SETTINGS_SHARED_DIRECTORIES), rc);
	//::AdjustWindowRect(rc, GetWindowLongPtr(GWL_STYLE), false);
	//dirPage->SetWindowPos(m_hWnd, rc.left+10, rc.top+10, 0, 0, SWP_NOSIZE);
	dirPage->SetWindowPos(HWND_TOP, 17, 150, 0, 0, SWP_NOSIZE);
	dirPage->ShowWindow(SW_SHOW);
	return TRUE;
}

ShareProfilePtr SharePage::getSelectedProfile() {
	return profiles[ctrlProfile.GetCurSel()];
}

LRESULT SharePage::onProfileChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto p = getSelectedProfile();
	curProfile = p->getToken();
	dirPage->showProfile();
	fixControls();
	return 0;
}

LRESULT SharePage::onEditTempShares(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TempShareDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT SharePage::onClickedDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT SharePage::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlAddProfile.SetState(false);
	return 0;
}

LRESULT SharePage::onClickedAddProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CRect rect;
	ctrlAddProfile.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x-rect.Width();
	ctrlAddProfile.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	targetMenu.appendItem(CTSTRING(NEW_PROFILE_DOTS), [this] { handleAddProfile(false); });
	targetMenu.appendItem(CTSTRING(COPY_FROM_THIS_DOTS), [this] { handleAddProfile(true); });
	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

void SharePage::handleAddProfile(bool copy) {
	LineDlg virt;
	virt.title = copy ? TSTRING(COPY_PROFILE) : TSTRING(NEW_PROFILE);
	virt.description = TSTRING(PROFILE_NAME_DESC);
	if(virt.DoModal(m_hWnd) != IDOK) {
		return;
	}

	//create the profile
	string name = Text::fromT(virt.line);
	auto newProfile = ShareProfilePtr(new ShareProfile(name));
	if (copy) {
		dirPage->onCopyProfile(newProfile->getToken());
	}

	//add in lists
	profiles.push_back(newProfile);
	addProfiles.insert(newProfile);

	//set the selection
	ctrlProfile.AddString(Text::toT(name).c_str());
	ctrlProfile.SetCurSel(ctrlProfile.GetCount()-1);
	curProfile = newProfile->getToken();
	dirPage->showProfile();
	fixControls();
}

LRESULT SharePage::onClickedRemoveProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto p = getSelectedProfile();
	//can't remove the default profile
	if (p->getToken() == SP_DEFAULT) {
		return 0;
	}

	if (MessageBox(CTSTRING_F(CONFIRM_PROFILE_REMOVAL, Text::toT(p->getDisplayName())), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
		return 0;

	/* Remove the profile */
	auto n = find(addProfiles.begin(), addProfiles.end(), p);
	if (n != addProfiles.end()) {
		addProfiles.erase(n);
	} else {
		removeProfiles.push_back(p->getToken());
	}
	renameProfiles.erase(remove_if(renameProfiles.begin(), renameProfiles.end(), CompareFirst<ShareProfilePtr, string>(p)), renameProfiles.end());

	dirPage->onRemoveProfile();

	/* Switch to default profile */
	ctrlProfile.DeleteString(ctrlProfile.GetCurSel());
	curProfile = SP_DEFAULT;
	dirPage->showProfile();
	ctrlProfile.SetCurSel(0);
	fixControls();
	return 0;
}

LRESULT SharePage::onClickedRenameProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto profile = profiles[ctrlProfile.GetCurSel()];
	auto name = Text::toT(profile->getPlainName());
	auto p = boost::find_if(renameProfiles, CompareFirst<ShareProfilePtr, string>(profile));
	if (p != renameProfiles.end()) {
		name = Text::toT(p->second);
	}

	LineDlg virt;
	virt.title = TSTRING(PROFILE_NAME);
	virt.description = TSTRING(PROFILE_NAME_DESC);
	virt.line = name;
	if(virt.DoModal(m_hWnd) != IDOK || virt.line.empty())
		return 0;
	if (name == virt.line)
		return 0;

	// Update the list
	auto pos = ctrlProfile.GetCurSel();
	ctrlProfile.DeleteString(pos);
	tstring displayName = virt.line + (pos == 0 ? (_T(" (") + TSTRING(DEFAULT) + _T(")")) : Util::emptyStringT);
	ctrlProfile.InsertString(pos, displayName.c_str());
	ctrlProfile.SetCurSel(pos);

	if (p != renameProfiles.end()) {
		p->second = Text::fromT(virt.line);
		return 0;
	}

	renameProfiles.emplace_back(profile, Text::fromT(virt.line));

	fixControls();
	return 0;
}

ShareProfilePtr SharePage::getProfile(ProfileToken aProfile) {
	auto n = find(profiles.begin(), profiles.end(), aProfile);
	if (n != profiles.end()) {
		return *n;
	}

	auto p = find(addProfiles.begin(), addProfiles.end(), aProfile);
	if (p != addProfiles.end()) {
		return *n;
	}
	return nullptr;
}

LRESULT SharePage::onApplyChanges(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//applyChanges(false);
	return 0;
}

void SharePage::write() { }

Dispatcher::F SharePage::getThreadedTask() {
	if (hasChanged()) {
		return Dispatcher::F([=] { 
			applyChanges(true); 
		});
	}

	return nullptr;
}

void SharePage::fixControls() {
	//::EnableWindow(GetDlgItem(IDC_APPLY_CHANGES), hasChanged());
	::EnableWindow(GetDlgItem(IDC_REMOVE_PROFILE), curProfile != SP_DEFAULT);
}

bool SharePage::hasChanged() {
	return (!addProfiles.empty() || !removeProfiles.empty() || !renameProfiles.empty() || dirPage->hasChanged());
}

void SharePage::applyChanges(bool isQuit) {
	if (!addProfiles.empty()) {
		ShareManager::getInstance()->addProfiles(addProfiles);
		//aAddProfiles.clear();
	}

	if (!removeProfiles.empty()) {
		int favReset = 0;
		ClientManager::getInstance()->resetProfiles(removeProfiles, ShareManager::getInstance()->getShareProfile(SP_DEFAULT)); //reset for currently connected hubs
		favReset += FavoriteManager::getInstance()->resetProfiles(removeProfiles, ShareManager::getInstance()->getShareProfile(SP_DEFAULT)); //reset for favorite hubs
		ShareManager::getInstance()->removeProfiles(removeProfiles); //remove from profiles
		if (favReset > 0)
			MainFrame::getMainFrame()->callAsync([=] { ::MessageBox(WinUtil::mainWnd, CWSTRING_F(X_FAV_PROFILES_RESET, favReset), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK); });

		//aRemoveProfiles.clear();
	}

	if (!renameProfiles.empty()) {
		for(const auto& j: renameProfiles) {
			j.first->setPlainName(j.second);
		}
		FavoriteManager::getInstance()->onProfilesRenamed();
		//aRenameProfiles.clear();
	}

	dirPage->applyChanges(isQuit);
}