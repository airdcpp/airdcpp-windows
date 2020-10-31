/*
*
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
#include "Resource.h"

#include "ExtensionsFrame.h"
#include "ResourceLoader.h"
#include "DynamicDialogBase.h"

#include "semver/semver.hpp"

#include <airdcpp/ScopedFunctor.h>

#include <api/common/SettingUtils.h>

#define MAX_STATUS_LINES 10

string ExtensionsFrame::id = "Extensions";

int ExtensionsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_INSTALLED_VERSION, COLUMN_LATEST_VERSION };
int ExtensionsFrame::columnSizes[] = { 300, 900};
static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::DESCRIPTION, ResourceManager::CURRENT_VERSION, ResourceManager::LATEST_VERSION };

ExtensionsFrame::ExtensionsFrame() : ctrlStatusContainer((LPTSTR)STATUSCLASSNAME, this, EXT_STATUS_MSG_MAP), statusTextHandler(ctrlStatus, 1, MAX_STATUS_LINES) { };

LRESULT ExtensionsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_EXTENSIONS_LIST);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	createColumns();

	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	ctrlList.setFlickerFree(WinUtil::bgBrush);
	ctrlList.SetFont(WinUtil::listViewFont);

	//TODO: add new icons
	listImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_ONLINE, 16))); //extension running
	listImages.AddIcon(CIcon(ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_ONLINE, 16)))); //extension stopped
	listImages.AddIcon(CIcon(ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_QCONNECT, 16))));  // not installed
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_GET_TTH, 16)));  // remote extension
	ctrlList.SetImageList(listImages, LVSIL_SMALL);


	ctrlInstall.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_UPDATE);
	ctrlInstall.SetWindowText(CTSTRING(INSTALL));
	ctrlInstall.SetFont(WinUtil::systemFont);

	ctrlReload.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_RELOAD);
	ctrlReload.SetWindowText(CTSTRING(RELOAD));
	ctrlReload.SetFont(WinUtil::systemFont);

	ctrlActions.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_ACTIONS);
	ctrlActions.SetWindowText(CTSTRING(ACTIONS_DOTS));
	ctrlActions.SetFont(WinUtil::systemFont);

	ctrlReadMore.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_OPEN_LINK);
	ctrlReadMore.SetWindowText(CTSTRING(OPEN_HOMEPAGE));
	ctrlReadMore.SetFont(WinUtil::systemFont);

	SettingsManager::getInstance()->addListener(this);

	getExtensionManager().addListener(this);
	initLocalExtensions();

	callAsync([=] {
		updateList(); 
		downloadExtensionList(); 
	});

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(1, statusSizes);

	statusTextHandler.init(*this);

	WinUtil::SetIcon(m_hWnd, IDI_EXTENSIONS);
	bHandled = FALSE;
	return TRUE;
}

void ExtensionsFrame::initLocalExtensions() noexcept {

	ctrlList.insertGroup(static_cast<int>(ExtensionGroupEnum::INSTALLED), TSTRING(INSTALLED), LVGA_HEADER_LEFT);
	ctrlList.insertGroup(static_cast<int>(ExtensionGroupEnum::REMOTE), TSTRING(REMOTE), LVGA_HEADER_LEFT);
	ctrlList.insertGroup(static_cast<int>(ExtensionGroupEnum::NOT_INSTALLED), TSTRING(NOT_INSTALLED), LVGA_HEADER_LEFT);

	auto list = getExtensionManager().getExtensions();
	for (const auto& i : list) {
		itemInfos.emplace(i->getName(), make_unique<ItemInfo>(i));
	}
}

int ExtensionsFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, int col) noexcept {
	return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
}

int ExtensionsFrame::ItemInfo::getImageIndex() const noexcept {
	if (!ext) {
		return static_cast<int>(ExtensionIconEnum::NOT_INSTALLED);
	}

	if (!ext->isManaged()) {
		return static_cast<int>(ExtensionIconEnum::REMOTE);
	}

	return static_cast<int>(ext->isRunning() ? ExtensionIconEnum::RUNNING : ExtensionIconEnum::NOT_RUNNING);
}

ExtensionsFrame::ExtensionGroupEnum ExtensionsFrame::ItemInfo::getGroupId() const noexcept {
	if (!ext) {
		return ExtensionGroupEnum::NOT_INSTALLED;
	}

	return ext->isManaged() ? ExtensionGroupEnum::INSTALLED : ExtensionGroupEnum::REMOTE;
}

bool ExtensionsFrame::ItemInfo::hasUpdate() const noexcept {
	if (!ext || latestVersion.empty()) {
		return false;
	}

	try {
		semver::version installedSemver{ ext->getVersion() };
		semver::version latestSemver{ latestVersion };

		auto comp = installedSemver.compare(latestSemver);
		return comp < 0;
	} catch (const std::exception& e) {
		dcdebug("Failed to parse extension version (%s)\n", e.what());
	}

	return false;
}

LRESULT ExtensionsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/) {
	if (reinterpret_cast<HWND>(wParam) == ctrlList) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList, pt);
		}
		
		if (ctrlList.GetSelectedCount() == 1) {
			int sel = ctrlList.GetNextItem(-1, LVNI_SELECTED);
			auto ii = ctrlList.getItemData(sel);
			string title = ii->ext ? ii->ext->getName() : ii->getName();
			OMenu menu;
			menu.CreatePopupMenu();
			menu.InsertSeparatorFirst(Text::toT(title));

			bool hasDownload = !!httpDownload;
			if (!ii->ext) {
				menu.appendItem(TSTRING(INSTALL), [=] { installExtension(ii); }, hasDownload ? OMenu::FLAG_DISABLED : 0);
			} else {
				if (ii->hasUpdate()) {
					menu.appendItem(TSTRING(UPDATE), [=] { onUpdateExtension(ii); }, hasDownload ? OMenu::FLAG_DISABLED : 0);
				}

				appendLocalExtensionActions({ ii }, menu);
			}

			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
	}

	return FALSE;
}


void ExtensionsFrame::appendLocalExtensionActions(const ItemInfoList& aItems, OMenu& menu_) noexcept {
	{
		StringList tokens;
		for (const auto& ii: aItems) {
			tokens.push_back(ii->getName());
		}

		EXT_CONTEXT_MENU(menu_, Extension, tokens);
	}

	menu_.appendSeparator();


	bool hasManagedExtensions = any_of(aItems.begin(), aItems.end(), [](const ItemInfo* ii) { return ii->ext->isManaged(); });
	bool hasRunningExtensions = any_of(aItems.begin(), aItems.end(), [](const ItemInfo* ii) { return ii->ext->isManaged() && ii->ext->isRunning(); });
	bool hasStoppedExtensions = any_of(aItems.begin(), aItems.end(), [](const ItemInfo* ii) { return ii->ext->isManaged() && !ii->ext->isRunning(); });

	if (hasStoppedExtensions) {
		menu_.appendItem(TSTRING(START), [=] {
			for (const auto& ii : aItems) {
				if (!ii->ext->isRunning()) {
					onStartExtension(ii);
				}
			}
		});
	}
	
	if (hasRunningExtensions) {
		menu_.appendItem(TSTRING(STOP), [=] {
			for (const auto& ii: aItems) {
				if (ii->ext->isRunning()) {
					onStopExtension(ii);
				}
			}
		});
	}

	if (aItems.size() == 1 && aItems.front()->ext->hasSettings()) {
		menu_.appendItem(TSTRING(CONFIGURE) + _T("..."), [=] { onConfigExtension(aItems.front()); });
	}

	if (hasManagedExtensions) {
		menu_.appendSeparator();
		menu_.appendItem(TSTRING(UNINSTALL), [=] {
			for (const auto& ii : aItems) {
				onRemoveExtension(ii);
			}
		});
	}
}


LRESULT ExtensionsFrame::onClickedInstall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto ii = ctrlList.getSelectedItem();
	installExtension(ii);
	return TRUE;
}

LRESULT ExtensionsFrame::onClickedReadMore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto ii = ctrlList.getSelectedItem();
	onReadMore(ii);
	return TRUE;
}

void ExtensionsFrame::reload() noexcept {
	ctrlList.SetRedraw(FALSE);

	ctrlList.DeleteAllItems();
	itemInfos.clear();
	initLocalExtensions();

	ctrlList.SetRedraw(TRUE);

	downloadExtensionList();
}

LRESULT ExtensionsFrame::onClickedReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	reload();
	return TRUE;
}


ExtensionsFrame::ItemInfoList ExtensionsFrame::getSelectedLocalExtensions() noexcept {
	ItemInfoList ret;
	ctrlList.forEachSelectedT([&](const ItemInfo* ii) {
		if (ii->ext) {
			ret.push_back(ii);
		}
	});

	return ret;
}

LRESULT ExtensionsFrame::onClickedExtensionActions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const auto localExtensions = getSelectedLocalExtensions();

	CRect rect;
	ctrlActions.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x - rect.Width();
	ctrlActions.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	appendLocalExtensionActions(localExtensions, targetMenu);
	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

LRESULT ExtensionsFrame::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlActions.SetState(false);
	return 0;
}

LRESULT ExtensionsFrame::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	ctrlList.SetFocus();
	return 0;
}

void ExtensionsFrame::updateList() noexcept {
	ctrlList.SetRedraw(FALSE);
	ctrlList.DeleteAllItems();
	for (auto& i : itemInfos) {
		addEntry(i.second.get());
	}
	ctrlList.SetRedraw(TRUE);
}

void ExtensionsFrame::addEntry(const ItemInfo* ii) noexcept {
	ctrlList.insertItem(ctrlList.getSortPos(ii), ii, ii->getImageIndex(), static_cast<int>(ii->getGroupId()));
}

void ExtensionsFrame::updateEntry(const ItemInfo* ii) noexcept {
	ctrlList.updateItemImage(ii);
	ctrlList.updateItem(ii);
}

void ExtensionsFrame::onStopExtension(const ItemInfo* ii) noexcept {
	if (!ii->ext->stop()) {
		updateStatusAsync(Text::toT("Failed to stop the extension"), LogMessage::SEV_ERROR);
	}
}

void ExtensionsFrame::onStartExtension(const ItemInfo* ii) noexcept {
	auto wsm = WebServerManager::getInstance();

	try {
		ii->ext->start(getExtensionManager().getStartCommand(ii->ext->getEngines()), wsm);
	} catch (const Exception& e) {
		updateStatusAsync(Text::toT(e.what()), LogMessage::SEV_ERROR);
	}
}

void ExtensionsFrame::onRemoveExtension(const ItemInfo* ii) noexcept {
	try {
		getExtensionManager().removeExtension(ii->ext);
	} catch (const Exception& e) {
		updateStatusAsync(Text::toT(e.what()), LogMessage::SEV_ERROR);
	}
}

void ExtensionsFrame::onUpdateExtension(const ItemInfo* ii) noexcept {
	installExtension(ii);
}

void ExtensionsFrame::onReadMore(const ItemInfo* ii) noexcept {
	ActionUtil::openLink(Text::toT(ii->ext->getHomepage()));
}

void ExtensionsFrame::onConfigExtension(const ItemInfo* ii) noexcept {
	DynamicDialogBase dlg(TSTRING(CONFIGURE) + _T(" ") + Text::toT(ii->getName()));

	auto settings = ii->ext->getSettings();

	//use reference to setting item...
	for (auto& s : settings) {
		dlg.getPage()->addConfigItem(s);
	}
	webserver::SettingValueMap values;
	if (dlg.DoModal() == IDOK) {
		for (auto& s : settings) {
			//if (s.getDefaultValue(). != s.getValue())
				values.emplace(s.name, s.getValue());
		}

		UserList userReferences;
		ii->ext->setSettingValues(values, userReferences);
	}
}


void ExtensionsFrame::downloadExtensionList() noexcept {
	updateStatusAsync(TSTRING(EXTENSION_CATALOG_DOWNLOADING), LogMessage::SEV_INFO);
	httpDownload.reset(new HttpDownload(
		extensionUrl,
		[this] { onExtensionListDownloaded(); }, 
		false
	));
}


webserver::ExtensionManager& ExtensionsFrame::getExtensionManager() noexcept {
	return webserver::WebServerManager::getInstance()->getExtensionManager();
}

void ExtensionsFrame::updateStatusAsync(const tstring& aMessage, uint8_t aSeverity) noexcept {
	callAsync([=] {
		statusTextHandler.addStatusText(aMessage, aSeverity);
	});
}

void ExtensionsFrame::onExtensionListDownloaded() noexcept {
	ScopedFunctor([&] {
		httpDownload.reset();

		callAsync([this] {
			fixControls();
		});
	});

	if (httpDownload->buf.empty()) {
		updateStatusAsync(TSTRING_F(X_DOWNLOAD_FAILED_X, TSTRING(EXTENSION_CATALOG) % Text::toT(httpDownload->status)), LogMessage::SEV_ERROR);
		return;
	}
	
	updateStatusAsync(TSTRING(EXTENSION_CATALOG_DOWNLOADED), LogMessage::SEV_INFO);
	try {

		json listJson = json::parse(httpDownload->buf);
		auto pJson = listJson.at("objects");

		callAsync([this, list = std::move(pJson)] {
			for (const auto i: list) {
				json package = i.at("package");
				string name = package.at("name");
				string desc = package.at("description");
				string version = package.at("version");

				auto existing = itemInfos.find(name);
				if (existing != itemInfos.end()) {
					existing->second->setLatestVersion(version);
				} else {
					itemInfos.emplace(name, make_unique<ItemInfo>(name, desc, version));
				}
			}

			updateList();
		});
	} catch (const std::exception& e) {
		updateStatusAsync(TSTRING_F(X_PARSING_FAILED_X, TSTRING(EXTENSION_CATALOG) % Text::toT(e.what())), LogMessage::SEV_ERROR);
	}

}

void ExtensionsFrame::installExtension(const ItemInfo* ii) noexcept {
	auto wsm = WebServerManager::getInstance();
	if (!wsm->isRunning()) {
		updateStatusAsync(TSTRING(WEB_EXTENSION_SERVER_NOT_RUNNING), LogMessage::SEV_ERROR);
		return;
	}

	const auto name = Text::toT(ii->getName());
	updateStatusAsync(ii->ext ? TSTRING_F(EXTENSION_UPDATING_X, name) : TSTRING_F(EXTENSION_INSTALLING_X, name), LogMessage::SEV_INFO);
	httpDownload.reset(new HttpDownload(
		packageUrl + ii->getName() + "/latest",
		[this] {
			onExtensionInfoDownloaded(); 
		}, 
		false
	));

	fixControls();
}

void ExtensionsFrame::onExtensionInfoDownloaded() noexcept {
	ScopedFunctor([&] {
		callAsync([this] {
			fixControls();
		});

		httpDownload.reset(); 
	});

	if (httpDownload->buf.empty()) {
		updateStatusAsync(TSTRING_F(X_DOWNLOAD_FAILED_X, TSTRING(EXTENSION_DATA) % Text::toT(httpDownload->status)), LogMessage::SEV_ERROR);
		return;
	}

	try {
		const json packageJson = json::parse(httpDownload->buf);

		json dist = packageJson.at("dist");
		
		const string aInstallId = packageJson.at("_id"); //"install_id" ???
		const string aSha = dist.at("shasum");
		const string aUrl = dist.at("tarball");


		getExtensionManager().downloadExtension(aInstallId, aUrl, aSha);
	} catch (const std::exception& e) {
		updateStatusAsync(TSTRING_F(X_PARSING_FAILED_X, TSTRING(EXTENSION_DATA) % Text::toT(e.what())), LogMessage::SEV_ERROR);
	}

}
string ExtensionsFrame::getData(const string& aData, const string& aEntry, size_t& pos) noexcept {
	pos = aData.find(aEntry, pos);
	if (pos == string::npos)
		return Util::emptyString;

	pos += aEntry.length();
	size_t iend = aData.find(",", pos);
	return aData.substr(pos, iend-1 - pos);

}
LRESULT ExtensionsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		SettingsManager::getInstance()->removeListener(this);
		getExtensionManager().removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_EXTENSIONS, false);
		PostMessage(WM_CLOSE);
		return 0;
	}

	ctrlList.SetRedraw(FALSE);
	ctrlList.DeleteAllItems();
	ctrlList.SetRedraw(TRUE);

	itemInfos.clear();
	bHandled = FALSE;
	return 0;
}

void ExtensionsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect sr;
	// CRect rc = rect;
	if (ctrlStatus.IsWindow()) {
		int w[2];
		ctrlStatus.GetClientRect(sr);

		w[1] = sr.right - 16;
		w[0] = 16;

		ctrlStatus.SetParts(2, w);

		statusTextHandler.onUpdateLayout(w[1]);
	}

	CRect rc = rect;
	int tmp = sr.top + 32;
	rc.bottom -= tmp;
	ctrlList.MoveWindow(rc);

	rc = rect;

	const int button_width = 100;
	// const int textbox_width = 30;

	const long bottom = rc.bottom - 2;
	const long top = rc.bottom - 28;

	//buttons
	rc.bottom = bottom;
	rc.top = bottom - 22;
	rc.left = 20;
	rc.right = rc.left + button_width;
	ctrlInstall.MoveWindow(rc);

	// rc.OffsetRect(button_width + 2, 0);
	//ctrlInstall.MoveWindow(rc);

	rc.OffsetRect(button_width + 2, 0);
	ctrlActions.MoveWindow(rc);

	rc.OffsetRect(button_width + 2, 0);
	ctrlReadMore.MoveWindow(rc);

	rc.OffsetRect(button_width + 4, 0);
	rc.right = rc.left + button_width + 40;
	ctrlReload.MoveWindow(rc);
	// CRect rc = rect;
	// ctrlList.MoveWindow(rc);
}

void ExtensionsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if (ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if (ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (ctrlList.GetFont() != WinUtil::listViewFont) {
		ctrlList.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if (refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}


void ExtensionsFrame::on(webserver::ExtensionManagerListener::Started) noexcept {
	callAsync([this] {
		reload();
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::Stopped) noexcept {
	callAsync([this] {
		reload();
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionAdded, const webserver::ExtensionPtr& e) noexcept {
	callAsync([=] {
		if (e->isManaged()) {
			// Local extension
			updateList();
		} else {
			// Remote extension
			auto i = itemInfos.find(e->getName());
			if (i == itemInfos.end()) {
				auto ret = itemInfos.emplace(e->getName(), make_unique<ItemInfo>(e));
				addEntry(ret.first->second.get());
			} else {
				auto& ii = i->second;
				ii->ext = e;
				updateEntry(ii.get());
			}
		}
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionStateUpdated, const Extension* aExtension) noexcept {
	callAsync([this, extensionName = aExtension->getName()] {
		auto i = itemInfos.find(extensionName);
		if (i != itemInfos.end()) {
			auto& ii = i->second;
			updateEntry(ii.get());
		}
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionRemoved, const webserver::ExtensionPtr& e) noexcept {
	callAsync([=] {
		ctrlList.SetRedraw(FALSE);
		auto i = itemInfos.find(e->getName());
		if (i != itemInfos.end()) {
			ctrlList.deleteItem(i->second.get());
			itemInfos.erase(i);
			downloadExtensionList();
		}
		ctrlList.SetRedraw(TRUE);
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::InstallationSucceeded, const string& /*aInstallId*/, const ExtensionPtr& aExtension, bool aUpdated) noexcept {
	const auto name = Text::toT(aExtension->getName());
	updateStatusAsync(aUpdated ? TSTRING_F(WEB_EXTENSION_UPDATED, name) : TSTRING_F(WEB_EXTENSION_INSTALLED, name), LogMessage::SEV_INFO);
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::InstallationStarted, const string& /*aInstallId*/) noexcept {
	/*callAsync([=] {
		updateStatus(TSTRING_F(WEB_EXTENSION_INSTALLED, aInstallId));
	});*/
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::InstallationFailed, const string& /*aInstallId*/, const string& aError) noexcept {
	updateStatusAsync(TSTRING_F(WEB_EXTENSION_INSTALLATION_FAILED, Text::toT(aError)), LogMessage::SEV_ERROR);
}

const tstring ExtensionsFrame::ItemInfo::getText(int col) const {

	switch (col) {

	case COLUMN_NAME: return Text::toT(getName());
	case COLUMN_DESCRIPTION: return Text::toT(getDescription());
	case COLUMN_INSTALLED_VERSION: return ext == nullptr ? Util::emptyStringT : Text::toT(ext->getVersion());
	case COLUMN_LATEST_VERSION: return Text::toT(getLatestVersion());

	default: return Util::emptyStringT;
	}
}

void ExtensionsFrame::fixControls() noexcept {
	const auto installedExtensions = getSelectedLocalExtensions();

	const auto isLocalExtension = ctrlList.GetSelectedCount() == 1 && installedExtensions.size() == 1;
	ctrlInstall.SetWindowTextW(isLocalExtension ? CTSTRING(UPDATE) : CTSTRING(INSTALL));

	::EnableWindow(GetDlgItem(IDC_UPDATE), !httpDownload && ctrlList.GetSelectedCount() == 1 && ((isLocalExtension && installedExtensions.front()->hasUpdate()) || installedExtensions.empty()));
	::EnableWindow(GetDlgItem(IDC_OPEN_LINK), ctrlList.GetSelectedCount() == 1);
	::EnableWindow(GetDlgItem(IDC_ACTIONS), !installedExtensions.empty());
	::EnableWindow(GetDlgItem(IDC_RELOAD), !httpDownload);
}

LRESULT ExtensionsFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT ExtensionsFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
	{
		auto ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		int colIndex = ctrlList.findColumn(cd->iSubItem);
		dcassert(ii);
		if (colIndex == COLUMN_INSTALLED_VERSION && ii->ext && ii->ext->isManaged()) {
			if (ii->hasUpdate()) {
				cd->clrText = SETTING(ERROR_COLOR);
			} else {
				cd->clrText = SETTING(COLOR_STATUS_FINISHED);
			}
		} else {
			cd->clrText = SETTING(NORMAL_COLOUR);
		}

		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	default:
		return CDRF_DODEFAULT;
	}
}

void ExtensionsFrame::createColumns() {
	// Create list view columns
	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlList.setSortColumn(COLUMN_NAME);

}
