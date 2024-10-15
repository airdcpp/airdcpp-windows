/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/util/Util.h>
#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/favorites/FavoriteManager.h>
#include <airdcpp/share/profiles/ShareProfileManager.h>
#include <airdcpp/share/temp_share/TempShareManager.h>

#include <windows/Resource.h>
#include <windows/settings/SharePage.h>
#include <windows/util/WinUtil.h>
#include <windows/dialog/LineDlg.h>
#include <windows/settings/base/PropertiesDlg.h>
#include <windows/settings/TempShare_dlg.h>
#include <windows/settings/SharePageDlg.h>
#include <windows/MainFrm.h>

namespace wingui {
PropPage::TextItem SharePage::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_ADD_PROFILE, ResourceManager::ADD_PROFILE_DOTS },
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_RENAME_PROFILE, ResourceManager::RENAME },
	{ 0, ResourceManager::LAST }
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
	fixControls();
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
	fixControls();
	return 0;
}

LRESULT SharePage::onClickedDefault(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto old = ranges::find(profiles, defaultProfile, &ShareProfileInfo::token);
	(*old)->isDefault = false;

	defaultProfile = curProfile;

	//rebuild the list
	auto p = ranges::find(profiles, curProfile, &ShareProfileInfo::token);
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
	auto newProfile = make_shared<ShareProfileInfo>(name);
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

	dirPage->onRemoveProfile(curProfile);

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
	return Dispatcher::F([=] { 
		applyChanges(true);
	});
}

void SharePage::fixControls() {
	auto hasTempShares = !TempShareManager::getInstance()->getTempShares().empty();

	::EnableWindow(GetDlgItem(IDC_REMOVE_PROFILE), curProfile != defaultProfile);
	::EnableWindow(GetDlgItem(IDC_SET_DEFAULT), curProfile != defaultProfile);
	::EnableWindow(GetDlgItem(IDC_EDIT_TEMPSHARES), hasTempShares ? TRUE : FALSE);
}

void SharePage::applyChanges(bool isQuit) {
	ShareProfileInfo::List handledProfiles;
	auto getHandled = [&](ShareProfileInfo::State aState) {
		handledProfiles.clear();
		copy_if(profiles.begin(), profiles.end(), back_inserter(handledProfiles), [aState](const ShareProfileInfoPtr& sp) { return sp->state == aState; });
	};

	auto& profileMgr = ShareManager::getInstance()->getProfileMgr();

	getHandled(ShareProfileInfo::STATE_ADDED);
	if (!handledProfiles.empty()) {
		profileMgr.addProfiles(handledProfiles);
	}

	if (defaultProfile != SETTING(DEFAULT_SP)) {
		profileMgr.setDefaultProfile(defaultProfile);
	}

	getHandled(ShareProfileInfo::STATE_REMOVED);
	if (!handledProfiles.empty()) {
		profileMgr.removeProfiles(handledProfiles); //remove from profiles
	}

	getHandled(ShareProfileInfo::STATE_RENAMED);
	if (!handledProfiles.empty()) {
		profileMgr.renameProfiles(handledProfiles);
	}

	dirPage->applyChanges(isQuit);
}
}