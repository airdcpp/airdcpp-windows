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
#include <windows/Resource.h>

#include <windows/ActionUtil.h>

#include <windows/DirectoryListingFrm.h>
#include <windows/PrivateFrame.h>
#include <windows/TextFrame.h>
#include <windows/SearchFrm.h>
#include <windows/LineDlg.h>
#include <windows/MainFrm.h>
#include <windows/FormatUtil.h>

#include <windows/HubFrame.h>
#include <windows/BrowseDlg.h>
#include <windows/ExMessageBox.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/util/DupeUtil.h>
#include <airdcpp/favorites/FavoriteUserManager.h>
#include <airdcpp/core/geo/GeoManager.h>
#include <airdcpp/util/LinkUtil.h>
#include <airdcpp/core/localization/Localization.h>
#include <airdcpp/events/LogManager.h>
#include <airdcpp/core/classes/Magnet.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/util/RegexUtil.h>
#include <airdcpp/core/localization/ResourceManager.h>
#include <airdcpp/core/classes/ScopedFunctor.h>
#include <airdcpp/search/SearchInstance.h>
#include <airdcpp/util/text/StringTokenizer.h>
#include <airdcpp/favorites/ReservedSlotManager.h>
#include <airdcpp/hub/user_command/UserCommand.h>
#include <airdcpp/util/Util.h>
#include <airdcpp/util/ValueGenerator.h>
#include <airdcpp/viewed_files/ViewFileManager.h>

#include <airdcpp/modules/PreviewAppManager.h>

namespace dcpp {

void UserInfoBase::pm() {
	wingui::ActionUtil::PM()(getUser(), getHubUrl());
}

void UserInfoBase::matchQueue() {
	wingui::ActionUtil::MatchQueue()(getUser(), getHubUrl());
}

void UserInfoBase::getList() {
	wingui::ActionUtil::GetList()(getUser(), getHubUrl());
}
void UserInfoBase::browseList() {
	wingui::ActionUtil::BrowseList()(getUser(), getHubUrl());
}
void UserInfoBase::getBrowseList() {
	wingui::ActionUtil::GetBrowseList()(getUser(), getHubUrl());
}

void UserInfoBase::connectFav() {
	wingui::ActionUtil::ConnectFav()(getUser(), getHubUrl());
}

void UserInfoBase::handleFav() {
	if (getUser() && !getUser()->isFavorite()) {
		FavoriteUserManager::getInstance()->addFavoriteUser(HintedUser(getUser(), getHubUrl()));
	} else if (getUser()) {
		FavoriteUserManager::getInstance()->removeFavoriteUser(getUser());
	}
}

void UserInfoBase::grant() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(getUser(), getHubUrl()), 600);
	}
}

void UserInfoBase::grantTimeless() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(getUser(), getHubUrl()), 0);
	}
}

void UserInfoBase::removeAll() {
	wingui::MainFrame::getMainFrame()->addThreadedTask([=] {
		if (getUser()) {
			QueueManager::getInstance()->removeSource(getUser(), QueueItem::Source::FLAG_REMOVED);
		}
		});
}

void UserInfoBase::grantHour() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(getUser(), getHubUrl()), 3600);
	}
}

void UserInfoBase::grantDay() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(getUser(), getHubUrl()), 24 * 3600);
	}
}

void UserInfoBase::grantWeek() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().reserveSlot(HintedUser(getUser(), getHubUrl()), 7 * 24 * 3600);
	}
}

void UserInfoBase::ungrant() {
	if (getUser()) {
		FavoriteUserManager::getInstance()->getReservedSlots().unreserveSlot(getUser());
	}
}

bool UserInfoBase::hasReservedSlot() {
	if (getUser()) {
		return FavoriteUserManager::getInstance()->getReservedSlots().hasReservedSlot(getUser());
	}
	return false;
}
}

namespace wingui {
void ActionUtil::PM::operator()(UserPtr aUser, const string& aUrl) const {
	if (aUser)
		PrivateFrame::openWindow(HintedUser(aUser, aUrl));
}

void ActionUtil::MatchQueue::operator()(UserPtr aUser, const string& aUrl) const {
	if (!aUser)
		return;

	MainFrame::getMainFrame()->addThreadedTask([=] {
		try {
			auto listData = FilelistAddData(HintedUser(aUser, aUrl), nullptr, ADC_ROOT_STR);
			QueueManager::getInstance()->addListHooked(listData, QueueItem::FLAG_MATCH_QUEUE);
		} catch (const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogMessage::SEV_ERROR, STRING(SETTINGS_QUEUE));
		}
	});
}

void ActionUtil::GetList::operator()(UserPtr aUser, const string& aUrl) const {
	if (!aUser)
		return;

	MainFrame::getMainFrame()->addThreadedTask([=] {
		DirectoryListingFrame::openWindow(HintedUser(aUser, aUrl), QueueItem::FLAG_CLIENT_VIEW);
	});
}

void ActionUtil::BrowseList::operator()(UserPtr aUser, const string& aUrl) const {
	if (!aUser)
		return;

	MainFrame::getMainFrame()->addThreadedTask([=] {
		DirectoryListingFrame::openWindow(HintedUser(aUser, aUrl), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
		});
}

void ActionUtil::GetBrowseList::operator()(UserPtr aUser, const string& aUrl) const {
	if (!aUser)
		return;

	if (allowGetFullList(HintedUser(aUser, aUrl))) {
		GetList()(aUser, aUrl);
	}
	else {
		BrowseList()(aUser, aUrl);
	}
}

void ActionUtil::ConnectFav::operator()(UserPtr aUser, const string& aUrl) const {
	if (aUser) {
		if (!aUrl.empty()) {
			connectHub(aUrl);
		}
	}
}

bool ActionUtil::browseList(tstring& target_, HWND aOwner) {
	const BrowseDlg::ExtensionList types[] = {
		{ CTSTRING(FILE_LISTS), _T("*.xml;*.xml.bz2") },
		{ CTSTRING(ALL_FILES), _T("*.*") }
	};

	BrowseDlg dlg(aOwner, BrowseDlg::TYPE_FILELIST, BrowseDlg::DIALOG_OPEN_FILE);
	dlg.setTitle(TSTRING(OPEN_FILE_LIST));
	dlg.setPath(Text::toT(AppUtil::getListPath()));
	dlg.setTypes(2, types);
	return dlg.show(target_);
}

bool ActionUtil::browseApplication(tstring& target_, HWND aOwner) {
	const BrowseDlg::ExtensionList types[] = {
		{ CTSTRING(APPLICATION), _T("*.exe") }
	};

	BrowseDlg dlg(aOwner, BrowseDlg::TYPE_APP, BrowseDlg::DIALOG_SELECT_FILE);
	dlg.setPath(target_, true);
	dlg.setTypes(1, types);
	return dlg.show(target_);
}

bool ActionUtil::getUCParams(HWND aParent, const UserCommand& uc, ParamMap& params_) noexcept {
	string::size_type i = 0;
	StringMap done;

	while ((i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if (j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j - i);
		if (done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);
			dlg.line = Text::toT(boost::get<string>(params_["line:" + name]));

			if (uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if (dlg.DoModal(aParent) == IDOK) {
				params_["line:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while ((i = uc.getCommand().find("%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = uc.getCommand().find(']', i);
		if (j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j - i);
		if (done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);

			if (uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if (dlg.DoModal(aParent) == IDOK) {
				params_["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

void ActionUtil::copyMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	if (!aFile.empty()) {
		WinUtil::setClipboard(Text::toT(makeMagnet(aHash, aFile, aSize)));
	}
}

string ActionUtil::makeMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	return Magnet::makeMagnet(aHash, aFile, aSize);
}

void ActionUtil::searchHash(const TTHValue& aHash, const string&, int64_t) {
	SearchFrame::openWindow(Text::toT(aHash.toBase32()), 0, Search::SIZE_DONTCARE, SEARCH_TYPE_TTH);
}

void ActionUtil::openLink(const tstring& aUrl) {
	parseDBLClick(aUrl);
}

bool ActionUtil::parseDBLClick(const tstring& aStr) {
	if (aStr.empty())
		return false;

	auto url = Text::fromT(aStr);

	if (LinkUtil::isHubLink(url)) {
		connectHub(url);
		return true;
	}

	if (DupeUtil::isRelease(url)) {
		search(Text::toT(url));
		return true;
	} else if (RegexUtil::stringRegexMatch(LinkUtil::getUrlReg(), url)) {
		if (aStr.find(_T("magnet:?")) != tstring::npos) {
			parseMagnetUri(aStr, HintedUser());
			return true;
		}
		::ShellExecute(NULL, NULL, Text::toT(url).c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	} else if (aStr.size() > 3 && (aStr.substr(1, 2) == _T(":\\")) || (aStr.substr(0, 2) == _T("\\\\"))) {
		::ShellExecute(NULL, NULL, aStr.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}

	return false;
}

void ActionUtil::parseMagnetUri(const tstring& aUrl, const HintedUser& aOptionalUser, RichTextBox* aCtrlEdit /*nullptr*/) {
	if (strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		Magnet m = Magnet(Text::fromT(aUrl));
		if (!m.hash.empty() && Encoder::isBase32(m.hash.c_str())) {
			auto sel = !aOptionalUser ? SettingsManager::MAGNET_SEARCH : SETTING(MAGNET_ACTION);
			BOOL remember = false;
			if (SETTING(MAGNET_ASK) && aOptionalUser) {
				CTaskDialog taskdlg;

				tstring msg = CTSTRING_F(MAGNET_INFOTEXT, Text::toT(m.fname) % Util::formatBytesW(m.fsize));
				taskdlg.SetContentText(msg.c_str());
				TASKDIALOG_BUTTON buttons[] =
				{
					{ IDC_MAGNET_OPEN, CTSTRING(DOWNLOAD_OPEN), },
					{ IDC_MAGNET_QUEUE, CTSTRING(SAVE_DEFAULT), },
					{ IDC_MAGNET_SEARCH, CTSTRING(MAGNET_DLG_SEARCH), },
				};
				taskdlg.ModifyFlags(0, TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS);
				taskdlg.SetWindowTitle(CTSTRING(MAGNET_DLG_TITLE));
				taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);


				taskdlg.SetButtons(buttons, _countof(buttons));
				//taskdlg.SetExpandedInformationText(_T("More information"));
				taskdlg.SetMainIcon(IDI_MAGNET);
				taskdlg.SetVerificationText(CTSTRING(MAGNET_DLG_REMEMBER));

				//if (!aUser.user)
				//	taskdlg.EnableButton(0, FALSE);

				taskdlg.DoModal(WinUtil::mainWnd, &sel, 0, &remember);
				if (sel == IDCANCEL) {
					return;
				}

				sel = sel - IDC_MAGNET_SEARCH;
				if (remember) {
					SettingsManager::getInstance()->set(SettingsManager::MAGNET_ASK, false);
					SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, sel);
				}
			}

			if (sel == SettingsManager::MAGNET_SEARCH) {
				searchHash(m.getTTH(), m.fname, m.fsize);
			} else if (sel == SettingsManager::MAGNET_DOWNLOAD) {
				try {
					if (aCtrlEdit) {
						aCtrlEdit->handleDownload(SETTING(DOWNLOAD_DIRECTORY), Priority::DEFAULT, false);
					} else {
						addFileDownload(SETTING(DOWNLOAD_DIRECTORY) + m.fname, m.fsize, m.getTTH(), aOptionalUser, 0);
					}
				} catch (const Exception& e) {
					LogManager::getInstance()->message(e.getError(), LogMessage::SEV_ERROR, STRING(SETTINGS_QUEUE));
				}
			} else if (sel == SettingsManager::MAGNET_OPEN) {
				openTextFile(m.fname, m.fsize, m.getTTH(), aOptionalUser, false);
			}
		} else {
			MessageBox(WinUtil::mainWnd, CTSTRING(MAGNET_DLG_TEXT_BAD), CTSTRING(MAGNET_DLG_TITLE), MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

void ActionUtil::openTextFile(const string& aFileName, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, bool aIsClientView) noexcept {
	if (!aUser) {
		dcassert(0);
		return;
	}

	MainFrame::getMainFrame()->addThreadedTask([=] {
		if (aIsClientView && (!SETTING(NFO_EXTERNAL) || PathUtil::getFileExt(aFileName) != ".nfo")) {
			auto fileData = ViewedFileAddData(aFileName, aTTH, aSize, nullptr, aUser, true);
			ViewFileManager::getInstance()->addUserFileHookedNotify(fileData);
			return;
		}

		try {
			auto fileData = ViewedFileAddData(aFileName, aTTH, aSize, nullptr, aUser, false);
			QueueManager::getInstance()->addOpenedItemHooked(fileData, false);
		} catch (const Exception& e) {
			auto nicks = ClientManager::getInstance()->getFormattedNicks(aUser);
			LogManager::getInstance()->message(STRING_F(ADD_FILE_ERROR, aFileName % nicks % e.getError()), LogMessage::SEV_NOTIFY, STRING(SETTINGS_QUEUE));
		}
	});
}

void ActionUtil::openFile(const tstring& aFileName) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		if (PathUtil::fileExists(Text::fromT(aFileName))) {
			::ShellExecute(NULL, NULL, PathUtil::formatPathW(aFileName).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	});
}

void ActionUtil::openFolder(const tstring& aFileName) {
	if (aFileName.empty())
		return;

	MainFrame::getMainFrame()->addThreadedTask([=] {
		if (PathUtil::fileExists(Text::fromT(aFileName))) {
			::ShellExecute(NULL, Text::toT("open").c_str(), PathUtil::formatPathW(PathUtil::getFilePath(aFileName)).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	});
}


void ActionUtil::appendPreviewMenu(OMenu& parent_, const string& aTarget) {
	auto previewMenu = parent_.createSubMenu(TSTRING(PREVIEW), true);

	auto lst = PreviewAppManager::getInstance()->getPreviewApps();
	auto ext = PathUtil::getFileExt(aTarget);
	if (ext.empty()) return;

	ext = ext.substr(1); //remove the dot

	for (auto& i : lst) {
		auto tok = StringTokenizer<string>(i->getExtension(), ';').getTokens();

		if (tok.size() == 0 || any_of(tok.begin(), tok.end(), [&ext](const string& si) { return _stricmp(ext.c_str(), si.c_str()) == 0; })) {

			string application = i->getApplication();
			string arguments = i->getArguments();

			previewMenu->appendItem(Text::toT((i->getName())), [=] {
				string tempTarget = QueueManager::getInstance()->getTempTarget(aTarget);
				ParamMap ucParams;

				ucParams["file"] = "\"" + tempTarget + "\"";
				ucParams["dir"] = "\"" + PathUtil::getFilePath(tempTarget) + "\"";

				::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams)).c_str(), Text::toT(PathUtil::getFilePath(tempTarget)).c_str(), SW_SHOWNORMAL);
			});
		}
	}
}

template<typename T>
static void appendPrioMenu(OMenu& aParent, const vector<T>& aBase, bool aIsBundle, function<void(Priority aPrio)> aPrioF, Callback aAutoPrioF) {
	if (aBase.empty())
		return;

	tstring text;

	if (aIsBundle) {
		text = aBase.size() == 1 ? TSTRING(SET_BUNDLE_PRIORITY) : TSTRING(SET_BUNDLE_PRIORITIES);
	} else {
		text = aBase.size() == 1 ? TSTRING(SET_FILE_PRIORITY) : TSTRING(SET_FILE_PRIORITIES);
	}

	Priority p = Priority::DEFAULT;
	for (auto& aItem : aBase) {
		if (aItem->getPriority() != p && p != Priority::DEFAULT) {
			p = Priority::DEFAULT;
			break;
		}

		p = aItem->getPriority();
	}

	auto priorityMenu = aParent.createSubMenu(text, true);

	int curItem = 0;
	auto appendItem = [=, &curItem](const tstring& aString, Priority aPrio) {
		curItem++;
		priorityMenu->appendItem(aString, [=] {
			aPrioF(aPrio);
		}, OMenu::FLAG_THREADED | (p == aPrio ? OMenu::FLAG_CHECKED : 0));
	};

	if (aIsBundle) {
		appendItem(TSTRING(PAUSED_FORCED), Priority::PAUSED_FORCE);
	}

	appendItem(TSTRING(PAUSED), Priority::PAUSED);
	appendItem(TSTRING(LOWEST), Priority::LOWEST);
	appendItem(TSTRING(LOW), Priority::LOW);
	appendItem(TSTRING(NORMAL), Priority::NORMAL);
	appendItem(TSTRING(HIGH), Priority::HIGH);
	appendItem(TSTRING(HIGHEST), Priority::HIGHEST);

	curItem++;

	auto usingAutoPrio = all_of(aBase.begin(), aBase.end(), [](const T& item) { return item->getAutoPriority(); });
	priorityMenu->appendItem(TSTRING(AUTO), aAutoPrioF, OMenu::FLAG_THREADED | (usingAutoPrio ? OMenu::FLAG_CHECKED : 0));
}

void ActionUtil::appendBundlePrioMenu(OMenu& aParent, const BundleList& aBundles) {
	auto prioF = [=](Priority aPrio) {
		for (auto& b : aBundles)
			QueueManager::getInstance()->setBundlePriority(b->getToken(), aPrio);
	};

	auto autoPrioF = [=] {
		for (auto& b : aBundles)
			QueueManager::getInstance()->toggleBundleAutoPriority(b->getToken());
	};

	appendPrioMenu<BundlePtr>(aParent, aBundles, true, prioF, autoPrioF);
}

void ActionUtil::appendBundlePauseMenu(OMenu& aParent, const BundleList& aBundles) {

	//Maybe move this to priority menu??
	auto pauseMenu = aParent.createSubMenu(TSTRING(PAUSE_BUNDLE_FOR), true);
	auto pauseTimes = { 5, 10, 30, 60, 90, 120, 180 };
	for (auto t : pauseTimes) {
		pauseMenu->appendItem(Util::toStringW(t) + _T(" ") + TSTRING(MINUTES_LOWER), [=] {
			for (auto b : aBundles)
				QueueManager::getInstance()->setBundlePriority(b, Priority::PAUSED_FORCE, false, GET_TIME() + (t * 60));
			}, OMenu::FLAG_THREADED);
	}
	pauseMenu->appendSeparator();
	pauseMenu->appendItem(TSTRING(CUSTOM), [=] {
		LineDlg dlg;
		dlg.title = TSTRING(PAUSE_BUNDLE_FOR);
		dlg.description = CTSTRING(PAUSE_TIME);
		if (dlg.DoModal() == IDOK) {
			for (auto b : aBundles)
				QueueManager::getInstance()->setBundlePriority(b, Priority::PAUSED_FORCE, false, GET_TIME() + (Util::toUInt(Text::fromT(dlg.line)) * 60));
		}
		}, OMenu::FLAG_THREADED);

}


void ActionUtil::appendFilePrioMenu(OMenu& aParent, const QueueItemList& aFiles) {
	auto prioF = [=](Priority aPrio) {
		for (auto& qi : aFiles)
			QueueManager::getInstance()->setQIPriority(qi->getTarget(), aPrio);
	};

	auto autoPrioF = [=] {
		for (auto& qi : aFiles)
			QueueManager::getInstance()->setQIAutoPriority(qi->getTarget());
	};

	appendPrioMenu<QueueItemPtr>(aParent, aFiles, false, prioF, autoPrioF);
}

void ActionUtil::search(const tstring& aSearch, bool aSearchDirectory) {
	tstring searchTerm = aSearch;
	searchTerm.erase(std::remove(searchTerm.begin(), searchTerm.end(), '\r'), searchTerm.end());
	searchTerm.erase(std::remove(searchTerm.begin(), searchTerm.end(), '\n'), searchTerm.end());
	if (!searchTerm.empty()) {
		SearchFrame::openWindow(searchTerm, 0, Search::SIZE_DONTCARE, aSearchDirectory ? SEARCH_TYPE_DIRECTORY : SEARCH_TYPE_ANY);
	}
}

void ActionUtil::viewLog(const string& aPath, bool aHistory /*false*/) {
	if (aHistory) {
		auto aText = LogManager::readFromEnd(aPath, SETTING(LOG_LINES), Util::convertSize(64, Util::KB));
		if (!aText.empty()) {
			TextFrame::viewText(PathUtil::getFileName(aPath), aText, TextFrame::FileType::LOG, nullptr);
		}
	} else if (SETTING(OPEN_LOGS_INTERNAL)) {
		TextFrame::openFile(aPath);
	} else {
		ShellExecute(NULL, NULL, Text::toT(aPath).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
}

void ActionUtil::removeBundle(QueueToken aBundleToken) {
	BundlePtr aBundle = QueueManager::getInstance()->findBundle(aBundleToken);
	if (aBundle) {
		if (::MessageBox(0, CTSTRING_F(CONFIRM_REMOVE_DIR_BUNDLE, Text::toT(aBundle->getName())), Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
			return;
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			auto b = aBundle;
			QueueManager::getInstance()->removeBundle(b, false);
		});
	}
}

void ActionUtil::appendLanguageMenu(CComboBoxEx& ctrlLanguage_) noexcept {
	ctrlLanguage_.SetImageList(ResourceLoader::flagImages);
	int count = 0;

	const auto languages = Localization::getLanguages();
	for (const auto& l : languages) {
		COMBOBOXEXITEM cbli = { CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE };
		CString str = Text::toT(l.getLanguageName()).c_str();
		cbli.iItem = count++;
		cbli.pszText = (LPTSTR)(LPCTSTR)str;
		cbli.cchTextMax = str.GetLength();

		auto flagIndex = Localization::getFlagIndexByCode(l.getCountryFlagCode());
		cbli.iImage = flagIndex;
		cbli.iSelectedImage = flagIndex;
		ctrlLanguage_.InsertItem(&cbli);
	}

	auto cur = Localization::getLanguageIndex(languages);
	if (cur) {
		ctrlLanguage_.SetCurSel(*cur);
	}
}

void ActionUtil::setLanguage(int aLanguageIndex) noexcept {
	auto languages = Localization::getLanguages();
	if (static_cast<size_t>(aLanguageIndex) < languages.size()) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, languages[aLanguageIndex].getLanguageSettingValue());
	}
}

void ActionUtil::appendHistory(CComboBox& ctrlCombo_, SettingsManager::HistoryType aType) {
	// Add new items to the history dropdown list
	while (ctrlCombo_.GetCount())
		ctrlCombo_.DeleteString(0);

	const auto& lastSearches = SettingsManager::getInstance()->getHistory(aType);
	for (const auto& s : lastSearches) {
		ctrlCombo_.InsertString(0, Text::toT(s).c_str());
	}
}

string ActionUtil::addHistory(CComboBox& ctrlCombo, SettingsManager::HistoryType aType) {
	string ret = Text::fromT(WinUtil::getComboText(ctrlCombo, (WORD)-1));
	if (!ret.empty() && SettingsManager::getInstance()->addToHistory(ret, aType))
		appendHistory(ctrlCombo, aType);

	return ret;
}

void ActionUtil::showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, hIcon, force);
}

void ActionUtil::showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, dwInfoFlags, force);
}

bool ActionUtil::onConnSpeedChanged(WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl) {
	tstring speed;
	speed.resize(1024);
	speed.resize(::GetWindowText(hWndCtl, &speed[0], 1024));
	if (!speed.empty() && wNotifyCode != CBN_SELENDOK) {
		boost::wregex reg;
		if (speed[speed.size() - 1] == '.')
			reg.assign(_T("(\\d+\\.)"));
		else
			reg.assign(_T("(\\d+(\\.\\d+)?)"));
		if (!regex_match(speed, reg)) {
			CComboBox tmp;
			tmp.Attach(hWndCtl);
			DWORD dwSel;
			if ((dwSel = tmp.GetEditSel()) != CB_ERR && dwSel > 0) {
				auto it = speed.begin() + HIWORD(dwSel) - 1;
				speed.erase(it);
				tmp.SetEditSel(0, -1);
				tmp.SetWindowText(speed.c_str());
				tmp.SetEditSel(HIWORD(dwSel) - 1, HIWORD(dwSel) - 1);
				tmp.Detach();
				return false;
			}
		}
	}

	return true;
}

void ActionUtil::appendSpeedCombo(CComboBox& aCombo, SettingsManager::StrSetting aSetting) {
	auto curSpeed = SettingsManager::getInstance()->get(aSetting);
	bool found = false;

	//add the speed strings	
	for (const auto& speed : SettingsManager::connectionSpeeds) {
		if (Util::toDouble(curSpeed) < Util::toDouble(speed) && !found) {
			aCombo.AddString(Text::toT(curSpeed).c_str());
			found = true;
		}
		else if (curSpeed == speed) {
			found = true;
		}
		aCombo.AddString(Text::toT(speed).c_str());
	}

	//set current upload speed setting
	aCombo.SetCurSel(aCombo.FindString(0, Text::toT(curSpeed).c_str()));
}

void ActionUtil::appendDateUnitCombo(CComboBox& ctrlDateUnit, int aSel /*= 1*/) {
	ctrlDateUnit.AddString(CTSTRING(HOURS));
	ctrlDateUnit.AddString(CTSTRING(DAYS));
	ctrlDateUnit.AddString(CTSTRING(WEEKS));
	ctrlDateUnit.AddString(CTSTRING(MONTHS));
	ctrlDateUnit.AddString(CTSTRING(YEARS));
	ctrlDateUnit.SetCurSel(aSel);
}

void ActionUtil::appendSizeCombos(CComboBox& aUnitCombo, CComboBox& aModeCombo, int aUnitSel /*= 2*/, int aModeSel /*= 1*/) {
	aUnitCombo.AddString(CTSTRING(B));
	aUnitCombo.AddString(CTSTRING(KiB));
	aUnitCombo.AddString(CTSTRING(MiB));
	aUnitCombo.AddString(CTSTRING(GiB));
	aUnitCombo.SetCurSel(aUnitSel);


	aModeCombo.AddString(CTSTRING(NORMAL));
	aModeCombo.AddString(CTSTRING(AT_LEAST));
	aModeCombo.AddString(CTSTRING(AT_MOST));
	aModeCombo.AddString(CTSTRING(EXACT_SIZE));
	aModeCombo.SetCurSel(aModeSel);
}

int64_t ActionUtil::parseSize(CEdit& ctrlSize, CComboBox& ctrlSizeMode) {
	tstring size(ctrlSize.GetWindowTextLength() + 1, _T('\0'));
	ctrlSize.GetWindowText(&size[0], size.size());
	size.resize(size.size() - 1);

	double lsize = Util::toDouble(Text::fromT(size));
	switch (ctrlSizeMode.GetCurSel()) {
	case 1:
		lsize = Util::convertSize(lsize, Util::KB); break;
	case 2:
		lsize = Util::convertSize(lsize, Util::MB); break;
	case 3:
		lsize = Util::convertSize(lsize, Util::GB); break;
	}

	return static_cast<int64_t>(lsize);
}

LRESULT ActionUtil::onAddressFieldChar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	return onUserFieldChar(wNotifyCode, wID, hWndCtl, bHandled);
}

LRESULT ActionUtil::onUserFieldChar(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	TCHAR buf[1024];

	::GetWindowText(hWndCtl, buf, 1024);
	tstring old = buf;


	// Strip '$', '|', '<', '>' and ' ' from text
	TCHAR* b = buf, * f = buf, c;
	while ((c = *b++) != 0)
	{
		if (c != '$' && c != '|' && (wID == IDC_USERDESC || c != ' ') && ((wID != IDC_NICK && wID != IDC_USERDESC && wID != IDC_EMAIL) || (c != '<' && c != '>')))
			*f++ = c;
	}

	*f = '\0';

	if (old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if (start > 0) start--;
		if (end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	bHandled = FALSE;
	return FALSE;
}

void ActionUtil::getProfileConflicts(HWND aParent, int aProfile, ProfileSettingItem::List& conflicts) {
	conflicts.clear();

	// a custom set value that differs from the one used by the profile? don't replace those without confirmation
	for (const auto& newSetting : SettingsManager::profileSettings[aProfile]) {
		if (newSetting.isSet() && !newSetting.isProfileCurrent()) {
			conflicts.push_back(newSetting);
		}
	}

	if (!conflicts.empty()) {
		string msg;
		for (const auto& setting : conflicts) {
			msg += STRING_F(SETTING_NAME_X, setting.getDescription()) + "\r\n";
			msg += STRING_F(CURRENT_VALUE_X, setting.currentToString()) + "\r\n";
			msg += STRING_F(PROFILE_VALUE_X, setting.profileToString()) + "\r\n\r\n";
		}

		CTaskDialog taskdlg;

		tstring tmp1 = TSTRING_F(MANUALLY_CONFIGURED_MSG, conflicts.size() % Text::toT(SettingsManager::getInstance()->getProfileName(aProfile)).c_str());
		taskdlg.SetContentText(tmp1.c_str());

		auto tmp2 = Text::toT(msg);
		taskdlg.SetExpandedInformationText(tmp2.c_str());
		taskdlg.SetExpandedControlText(CTSTRING(SHOW_CONFLICTING));
		TASKDIALOG_BUTTON buttons[] =
		{
			{ IDC_USE_PROFILE, CTSTRING(USE_PROFILE_SETTINGS), },
			{ IDC_USE_OWN, CTSTRING(USE_CURRENT_SETTINGS), },
		};
		taskdlg.ModifyFlags(0, TDF_USE_COMMAND_LINKS | TDF_EXPAND_FOOTER_AREA);
		taskdlg.SetWindowTitle(CTSTRING(MANUALLY_CONFIGURED_DETECTED));
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetMainIcon(TD_INFORMATION_ICON);

		taskdlg.SetButtons(buttons, _countof(buttons));

		int sel = 0;
		taskdlg.DoModal(aParent, &sel, 0, 0);
		if (sel == IDC_USE_OWN) {
			conflicts.clear();
		}
	}
}

void ActionUtil::addFileDownload(const string& aTarget, int64_t aSize, const TTHValue& aTTH, const HintedUser& aOptionalUser, time_t aDate, Flags::MaskType aFlags /*0*/, Priority aPriority /*DEFAULT*/) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		try {
			auto fileInfo = BundleFileAddData(PathUtil::getFileName(aTarget), aTTH, aSize, aPriority, aDate);
			auto options = BundleAddOptions(PathUtil::getFilePath(aTarget), aOptionalUser, nullptr);
			QueueManager::getInstance()->createFileBundleHooked(options, fileInfo, aFlags);
		} catch (const Exception& e) {
			auto nick = aOptionalUser ? Text::fromT(FormatUtil::getNicks(aOptionalUser)) : STRING(UNKNOWN);
			LogManager::getInstance()->message(STRING_F(ADD_FILE_ERROR, aTarget % nick % e.getError()), LogMessage::SEV_ERROR, STRING(SETTINGS_QUEUE));
		}
	});
}

void ActionUtil::connectHub(const string& aUrl) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		ClientManager::getInstance()->createClient(aUrl);
	});
}

/*bool ActionUtil::checkClientPassword() {
	if (hasPassDlg)
		return false;

	auto passDlg = PassDlg();
	ScopedFunctor([] { hasPassDlg = false; });

	passDlg.description = TSTRING(PASSWORD_DESC);
	passDlg.title = TSTRING(PASSWORD_TITLE);
	passDlg.ok = TSTRING(UNLOCK);
	if (passDlg.DoModal(NULL) == IDOK) {
		if (passDlg.line != Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
			MessageBox(mainWnd, CTSTRING(INVALID_PASSWORD), Text::toT(shortVersionString).c_str(), MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}*/

void ActionUtil::findNfo(const string& aAdcPath, const HintedUser& aUser) noexcept {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		SearchInstance searchInstance("windows_find_nfo");

		auto search = make_shared<Search>(Priority::HIGH, Util::toString(ValueGenerator::rand()));
		search->maxResults = 1;
		search->path = aAdcPath;
		search->exts = { ".nfo" };
		search->size = 256 * 1024;
		search->sizeType = Search::SIZE_ATMOST;
		search->requireReply = true;

		string error;
		if (!searchInstance.userSearchHooked(aUser, search, error)) {
			MainFrame::getMainFrame()->ShowPopup(Text::toT(error), Text::toT(PathUtil::getAdcLastDir(aAdcPath)), NIIF_ERROR, true);
			return;
		}

		for (auto i = 0; i < 5; i++) {
			Thread::sleep(500);
			if (searchInstance.getResultCount() > 0) {
				break;
			}
		}

		if (searchInstance.getResultCount() > 0) {
			auto result = searchInstance.getResultList().front();

			auto fileData = ViewedFileAddData(result->getFileName(), result->getTTH(), result->getSize(), nullptr, aUser, true);
			ViewFileManager::getInstance()->addUserFileHookedNotify(fileData);
		} else {
			MainFrame::getMainFrame()->ShowPopup(TSTRING(NO_NFO_FOUND), Text::toT(PathUtil::getAdcLastDir(aAdcPath)), NIIF_INFO, true);
		}
	});
}

bool ActionUtil::allowGetFullList(const HintedUser& aUser) noexcept {
	if (aUser.user->isNMDC()) {
		return true;
	}

	auto shareInfo = ClientManager::getInstance()->getShareInfo(aUser);
	return shareInfo && (*shareInfo).fileCount > SETTING(FULL_LIST_DL_LIMIT);
}

/*tstring ActionUtil::getNicks(const CID& cid) {
	return Text::toT(Util::listToString(ClientManager::getInstance()->getNicks(cid)));
}

tstring ActionUtil::getNicks(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getFormattedNicks(user));
}


tstring ActionUtil::formatFolderContent(const DirectoryContentInfo& aContentInfo) {
	return Text::toT(Util::formatDirectoryContent(aContentInfo));
}

tstring ActionUtil::formatFileType(const string& aFileName) {
	auto type = PathUtil::getFileExt(aFileName);
	if (type.size() > 0 && type[0] == '.')
		type.erase(0, 1);
	return Text::toT(type);
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if (hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	}
	else {
		return make_pair(Text::toT(Util::listToString(hubs)), true);
	}
}

pair<tstring, bool> ActionUtil::getHubNames(const CID& cid) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid));
}

tstring ActionUtil::getHubNames(const HintedUser& aUser) {
	return Text::toT(ClientManager::getInstance()->getFormattedHubNames(aUser));
}

ActionUtil::CountryFlagInfo ActionUtil::toCountryInfo(const string& aIP) noexcept {
	auto ip = aIP;
	uint8_t flagIndex = 0;
	if (!ip.empty()) {
		// Only attempt to grab a country mapping if we actually have an IP address
		string tmpCountry = GeoManager::getInstance()->getCountry(aIP);
		if (!tmpCountry.empty()) {
			ip = tmpCountry + " (" + ip + ")";
			flagIndex = Localization::getFlagIndexByCode(tmpCountry.c_str());
		}
	}

	return {
		Text::toT(ip),
		flagIndex
	};
}*/
}
