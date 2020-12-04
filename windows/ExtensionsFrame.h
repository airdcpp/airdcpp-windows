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

#ifndef EXTENSIONS_FRAME
#define EXTENSIONS_FRAME

#include "Async.h"
#include "StaticFrame.h"
#include "StatusBarTextHandler.h"
#include "TypedListViewCtrl.h"

#include <airdcpp/HttpDownload.h>

#include <web-server/WebServerManager.h>
#include <web-server/ExtensionManager.h>
#include <web-server/Extension.h>

using namespace webserver;

#define EXT_STATUS_MSG_MAP 17

class ExtensionsFrame : public MDITabChildWindowImpl<ExtensionsFrame>, public StaticFrame<ExtensionsFrame, ResourceManager::EXTENSIONS, IDC_EXTENSIONS>,
	private SettingsManagerListener, private webserver::ExtensionManagerListener, private Async<ExtensionsFrame>
{
public:
	typedef MDITabChildWindowImpl<ExtensionsFrame> baseClass;

	ExtensionsFrame();
	~ExtensionsFrame() { };

	DECLARE_FRAME_WND_CLASS_EX(_T("ExtensionsFrame"), IDR_EXTENSIONS, 0, COLOR_3DFACE);


	BEGIN_MSG_MAP(ExtensionsFrame)
		NOTIFY_HANDLER(IDC_EXTENSIONS_LIST, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_EXTENSIONS_LIST, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(IDC_EXTENSIONS_LIST, LVN_GETINFOTIP, ctrlList.onInfoTip)
		NOTIFY_HANDLER(IDC_EXTENSIONS_LIST, LVN_ITEMCHANGED, onItemChanged)
		// NOTIFY_HANDLER(IDC_AUTOSEARCH, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_EXTENSIONS_LIST, NM_CUSTOMDRAW, onCustomDraw)

		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)

		COMMAND_ID_HANDLER(IDC_UPDATE, onClickedInstall)
		COMMAND_ID_HANDLER(IDC_CONFIGURE, onClickedConfigure)
		COMMAND_ID_HANDLER(IDC_ACTIONS, onClickedExtensionActions)
		COMMAND_ID_HANDLER(IDC_RELOAD, onClickedReload)
		COMMAND_ID_HANDLER(IDC_OPEN_LINK, onClickedReadMore)
		COMMAND_ID_HANDLER(IDC_SETTINGS_OPTIONS, onClickedSettings)

		CHAIN_MSG_MAP(baseClass)
		ALT_MSG_MAP(EXT_STATUS_MSG_MAP)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, statusTextHandler.onGetToolTip)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onClickedExtensionActions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedInstall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedConfigure(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedReadMore(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

	void createColumns();

	static string id;
private:
	StatusBarTextHandler statusTextHandler;
	CContainedWindow ctrlStatusContainer;

	class ItemInfo;
	TypedListViewCtrl<ItemInfo, IDC_EXTENSIONS_LIST, LVITEM_GROUPING> ctrlList;

	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	enum class ExtensionGroupEnum {
		INSTALLED = 0,
		REMOTE = 1,
		NOT_INSTALLED = 2,
	};

	enum class ExtensionIconEnum {
		RUNNING = 0,
		NOT_RUNNING = 1,
		NOT_INSTALLED = 2,
		REMOTE = 3,
	};

	struct ExtensionCatalogItem {
		const string name;
		const string description;
		const string version;
		const string homepage;
	};

	class ItemInfo {
	public:
		ItemInfo(const webserver::ExtensionPtr& aExtension) : ext(aExtension) { }
		ItemInfo(const string& aName, const string& aDescription, const string& aVersion, const string& aHomepage) { 
			setCatalogItem(aName, aDescription, aVersion, aHomepage);
		}
		~ItemInfo() { }

		const string& getName() const noexcept;
		const string& getDescription() const noexcept;
		tstring getHomepage() const noexcept;

		ExtensionGroupEnum getGroupId() const noexcept;

		const tstring getText(int col) const;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col) noexcept;
		int getImageIndex() const noexcept;

		bool hasUpdate() const noexcept;

		void setCatalogItem(const string& aName, const string& aDescription, const string& aVersion, const string& aHomepage) noexcept;

		webserver::ExtensionPtr ext = nullptr;
		unique_ptr<ExtensionCatalogItem> catalogItem = nullptr;
	private:
	};

	typedef unordered_map<string, unique_ptr<ItemInfo>> ItemInfoMap;
	ItemInfoMap itemInfos;
	typedef vector<const ItemInfo*> ItemInfoList;

	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_INSTALLED_VERSION,
		COLUMN_LATEST_VERSION,
		COLUMN_LAST
	};

	bool closed = false;

	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	void appendLocalExtensionActions(const ItemInfoList& aItems, OMenu& menu_) noexcept;
	void appendRemoteMenuItems(const ItemInfoList& aItems, OMenu& menu_) noexcept;

	ItemInfoList getSelectedLocalExtensions() noexcept;
	void fixControls() noexcept;

	void initLocalExtensions() noexcept;
	void updateList() noexcept;
	void reload() noexcept;
	void addEntry(const ItemInfo* ii) noexcept;
	void updateEntry(const ItemInfo* ii) noexcept;

	void onStopExtension(const ItemInfo* ii) noexcept;
	void onStartExtension(const ItemInfo* ii) noexcept;
	void onRemoveExtension(const ItemInfo* ii) noexcept;
	void onConfigExtension(const ItemInfo* ii) noexcept;
	void onUpdateExtension(const ItemInfo* ii) noexcept;
	void onReadMore(const ItemInfo* ii) noexcept;

	void downloadExtensionList() noexcept;
	void onExtensionListDownloaded() noexcept;

	void installExtension(const ItemInfo* ii) noexcept;
	void onExtensionInfoDownloaded() noexcept;
	void parseExtensionCatalogItems(const json& aJson) noexcept;

	webserver::ExtensionManager& getExtensionManager() noexcept;

	CImageList listImages;
	CStatusBarCtrl ctrlStatus;
	int statusSizes[2];

	CButton ctrlInstall, ctrlActions, ctrlReadMore, ctrlReload, ctrlOptions, ctrlConfigure;

	string getData(const string& aData, const string& aEntry, size_t& pos) noexcept;
	void updateStatusAsync(const tstring& aMessage, uint8_t aSeverity) noexcept;

	unique_ptr<HttpDownload> httpDownload;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	void on(webserver::ExtensionManagerListener::Started) noexcept override;
	void on(webserver::ExtensionManagerListener::Stopped) noexcept override;

	void on(webserver::ExtensionManagerListener::ExtensionAdded, const webserver::ExtensionPtr& e) noexcept override;
	void on(webserver::ExtensionManagerListener::ExtensionStateUpdated, const Extension*) noexcept override;
	void on(webserver::ExtensionManagerListener::ExtensionRemoved, const webserver::ExtensionPtr& e) noexcept override;

	void on(webserver::ExtensionManagerListener::InstallationStarted, const string& aInstallId) noexcept override;
	void on(webserver::ExtensionManagerListener::InstallationFailed, const string& aInstallId, const string& aError) noexcept override;
	void on(webserver::ExtensionManagerListener::InstallationSucceeded, const string& aInstallId, const ExtensionPtr& aExtension, bool aUpdated) noexcept override;


};
#endif