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
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_RENAME_PROFILE, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

SharePage::SharePage(SettingsManager *s) : defaultProfile(SETTING(DEFAULT_SP)), curProfile(SETTING(DEFAULT_SP)), PropPage(s), dirPage(unique_ptr<ShareDirectories>(new ShareDirectories(this, s))) {
	SetTitle(CTSTRING(SETTINGS_SHARINGPAGE));
	m_psp.dwFlags |= PSP_RTLREADING;
}

SharePage::~SharePage() { }

void SharePage::setProfileList() {
	while (ctrlProfile.GetCount() > 0)
		ctrlProfile.DeleteString(0);

	int n = 0;
	for (const auto& sp : profiles) {
		ctrlProfile.InsertString(n, Text::toT(sp->getDisplayName()).c_str());
		n++;
	}
}

LRESULT SharePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	GetDlgItem(IDC_EDIT_TEMPSHARES).EnableWindow((ShareManager::getInstance()->hasTempShares()) ? 1 : 0);
	ctrlAddProfile.Attach(GetDlgItem(IDC_ADD_PROFILE));

	ctrlProfile.Attach(GetDlgItem(IDC_PROFILE_SEL));
	setProfileList();


	ctrlProfile.SetCurSel(0);

	dirPage->Create(this->m_hWnd);

	auto factor = WinUtil::getFontFactor();
	dirPage->SetWindowPos(HWND_TOP, 22*factor, 155*factor, 0, 0, SWP_NOSIZE);

	/*CRect rc;
	::GetWindowRect(GetDlgItem(IDC_SETTINGS_SHARED_DIRECTORIES), rc);
	POINT p;
	p.x = rc.left;
	p.y = rc.top;
	::ScreenToClient(m_hWnd, &p);
	//::AdjustWindowRect(rc, GetWindowLongPtr(GWL_STYLE), false);
	dirPage->SetWindowPos(HWND_TOP, p.x+15, p.y+25, 0, 0, SWP_NOSIZE);
	//dirPage->MoveWindow(rc.left+10, rc.top+10, 0, 0);
	//dirPage->SetWindowPos(HWND_TOP, 17, 150, 0, 0, SWP_NOSIZE);*/
	dirPage->ShowWindow(SW_SHOW);
	//ClientToScreen(
	return TRUE;
}

LRESULT SharePage::onProfileChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	curProfile = profiles[ctrlProfile.GetCurSel()]->token;
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
	auto old = find(profiles, defaultProfile);
	(*old)->isDefault = false;

	defaultProfile = curProfile;
	dirPage->rebuildDiffs();

	//rebuild the list
	auto p = find(profiles, curProfile);
	(*p)->isDefault = true;
	rotate(profiles.begin(), p, profiles.end());
	setProfileList();
	ctrlProfile.SetCurSel(0);

	dirPage->showProfile();

	fixControls();
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
	ShareProfileInfoPtr newProfile = new ShareProfileInfo(name);
	newProfile->state = ShareProfileInfo::STATE_ADDED;
	if (copy) {
		dirPage->onCopyProfile(newProfile->token);
	}

	//add in lists
	profiles.push_back(newProfile);

	//set the selection
	ctrlProfile.AddString(Text::toT(name).c_str());
	ctrlProfile.SetCurSel(ctrlProfile.GetCount()-1);
	curProfile = newProfile->token;
	dirPage->showProfile();
	fixControls();
}

LRESULT SharePage::onClickedRemoveProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//auto p = getSelectedProfile();
	//can't remove the default profile
	if (curProfile == defaultProfile) {
		return 0;
	}

	auto p = find(profiles.begin(), profiles.end(), curProfile);
	if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_PROFILE_REMOVAL, Text::toT((*p)->name)), MB_ICONQUESTION))
		return 0;

	auto profile = *p;
	/* Remove the profile */
	if ((*p)->state == ShareProfileInfo::STATE_ADDED) {
		profiles.erase(p);
	} else {
		(*p)->state = ShareProfileInfo::STATE_REMOVED;

		//move to the back to keep the combo order up to date
		profiles.erase(p);
		profiles.push_back(profile);
	}

	dirPage->onRemoveProfile();

	/* Switch to default profile */
	ctrlProfile.DeleteString(ctrlProfile.GetCurSel());
	curProfile = defaultProfile;
	dirPage->showProfile();
	ctrlProfile.SetCurSel(0);
	fixControls();
	return 0;
}

LRESULT SharePage::onClickedRenameProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto profile = profiles[ctrlProfile.GetCurSel()];
	auto name = Text::toT(profile->name);

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

	profile->name = Text::fromT(virt.line);
	if (profile->state != ShareProfileInfo::STATE_ADDED) {
		profile->state = ShareProfileInfo::STATE_RENAMED;
	}

	fixControls();
	return 0;
}

LRESULT SharePage::onApplyChanges(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//applyChanges(false);
	return 0;
}

void SharePage::write() { }

Dispatcher::F SharePage::getThreadedTask() {
	if (hasChanged()) {
		bool resetAdcHubs = false;
		if (defaultProfile != SETTING(DEFAULT_SP) && (ClientManager::getInstance()->hasAdcHubs() || FavoriteManager::getInstance()->hasAdcHubs())) {
			tstring oldName = Text::toT(getProfile(SETTING(DEFAULT_SP))->name);
			tstring newName = Text::toT(getProfile(defaultProfile)->name);
			resetAdcHubs = WinUtil::showQuestionBox(TSTRING_F(CHANGE_SP_ASK_HUBS, oldName.c_str() % newName.c_str()), MB_ICONQUESTION);
		}

		return Dispatcher::F([=] { 
			applyChanges(true, resetAdcHubs);
		});
	}

	return nullptr;
}

void SharePage::fixControls() {
	//::EnableWindow(GetDlgItem(IDC_APPLY_CHANGES), hasChanged());
	::EnableWindow(GetDlgItem(IDC_REMOVE_PROFILE), curProfile != defaultProfile);
	::EnableWindow(GetDlgItem(IDC_SET_DEFAULT), curProfile != defaultProfile);
}

bool SharePage::hasChanged() {
	return defaultProfile != SETTING(DEFAULT_SP) || any_of(profiles.begin(), profiles.end(), [](const ShareProfileInfoPtr& aProfile) { return aProfile->state != ShareProfileInfo::STATE_NORMAL; }) || dirPage->hasChanged();
}

void SharePage::applyChanges(bool isQuit, bool resetAdcHubs) {
	ShareProfileInfo::List handledProfiles;
	auto getHandled = [&](ShareProfileInfo::State aState) {
		handledProfiles.clear();
		copy_if(profiles.begin(), profiles.end(), back_inserter(handledProfiles), [aState](const ShareProfileInfoPtr& sp) { return sp->state == aState; });
	};

	getHandled(ShareProfileInfo::STATE_ADDED);
	if (!handledProfiles.empty()) {
		ShareManager::getInstance()->addProfiles(handledProfiles);
	}

	if (defaultProfile != SETTING(DEFAULT_SP)) {
		auto oldDefault = SETTING(DEFAULT_SP);
		ClientManager::getInstance()->resetProfile(oldDefault, defaultProfile, !resetAdcHubs);

		//ensure that favorite hubs get the correct display name in the tab
		SettingsManager::getInstance()->set(SettingsManager::DEFAULT_SP, defaultProfile);
		FavoriteManager::getInstance()->resetProfile(oldDefault, defaultProfile, !resetAdcHubs);
	}

	getHandled(ShareProfileInfo::STATE_REMOVED);
	if (!handledProfiles.empty()) {
		int favReset = 0;
		ClientManager::getInstance()->resetProfiles(handledProfiles, defaultProfile); //reset for currently connected hubs
		favReset += FavoriteManager::getInstance()->resetProfiles(handledProfiles, defaultProfile); //reset for favorite hubs

		ShareManager::getInstance()->removeProfiles(handledProfiles); //remove from profiles

		if (favReset > 0)
			MainFrame::getMainFrame()->callAsync([=] { WinUtil::showMessageBox(TSTRING_F(X_FAV_PROFILES_RESET, favReset), MB_ICONINFORMATION); });

	}

	getHandled(ShareProfileInfo::STATE_RENAMED);
	if (!handledProfiles.empty()) {
		ShareManager::getInstance()->renameProfiles(handledProfiles);
		FavoriteManager::getInstance()->onProfilesRenamed();
	}

	dirPage->applyChanges(isQuit);
}