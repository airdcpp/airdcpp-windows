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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <airdcpp/forward.h>

#include <airdcpp/transfer/download/DownloadManagerListener.h>
#include <airdcpp/transfer/upload/upload_bundles/UploadBundleInfoReceiverListener.h>
#include <airdcpp/connection/ConnectionManagerListener.h>
#include <airdcpp/queue/QueueManagerListener.h>
#include <airdcpp/core/queue/TaskQueue.h>
#include <airdcpp/transfer/TransferInfoManagerListener.h>
#include <airdcpp/util/Util.h>

#include <windows/components/OMenu.h>
#include <windows/UCHandler.h>
#include <windows/components/TypedListViewCtrl.h>
#include <windows/resource.h>
#include <windows/UserInfoBaseHandler.h>
#include <windows/util/FormatUtil.h>

namespace wingui {
class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadBundleInfoReceiverListener, private ConnectionManagerListener, private QueueManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener, private TransferInfoManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView() { }
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETINFOTIP, onInfoTip)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		CHAIN_COMMANDS(uibBase)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void runUserCommand(UserCommand& uc);
	void prepareClose();

	LRESULT onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlTransfers.deleteAllItems();
		return 0;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

private:
	class ItemInfo;	
public:
	typedef TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS, string, noCaseStringHash, noCaseStringEq, NULL> ItemInfoList;
	ItemInfoList& getUserList() { return ctrlTransfers; }
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_BUNDLE
	};

	enum {
		COLUMN_FIRST,
		COLUMN_USER = COLUMN_FIRST,
		COLUMN_FILE,
		COLUMN_HUB_CONNECTIONS,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_IP,
		//COLUMN_ENCRYPTION,
		COLUMN_LAST
	};

	enum {
		IMAGE_DOWNLOAD = 0,
		IMAGE_UPLOAD,
		IMAGE_D_USER,
		IMAGE_U_USER
	};

	struct UpdateInfo;
	class ItemInfo : public UserInfoBase {
	public:
		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING
			
		};
		
		ItemInfo(const HintedUser& u, const string aToken, bool aDownload, bool aBundle = false) : user(u), download(aDownload),
			token(aToken), isBundle(aBundle) { }

		bool isBundle = false;
		bool download = true;
		bool transferFailed = false;
		bool collapsed = true;
		
		uint8_t flagIndex = 0;
		int16_t connections = 0;
		int16_t hits = -1;
		int16_t onlineUsers = 0;

		ItemInfo* parent = nullptr;
		HintedUser user;
		string token;
		string bundle;
		Status status = STATUS_WAITING;
		Transfer::Type type;
		
		int64_t pos = 0;
		int64_t size = 0;
		int64_t actual = 0;
		int64_t speed = 0;
		int64_t timeLeft = 0;
		int64_t totalSpeed = 0;
		
		tstring ip;
		tstring statusString;
		tstring target;
		tstring encryption;

		QueueToken getLocalBundleToken() const noexcept { return Util::toUInt32(bundle); }
		bool hasBundle() const noexcept { return !bundle.empty(); }
		void update(const UpdateInfo& ui);

		const UserPtr& getUser() const { return user.user; }
		const string& getHubUrl() const { return user.hint; }

		double getRatio() const { return (pos > 0) ? (double)actual / (double)pos : 1.0; }

		const tstring getText(uint8_t col) const;
		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		uint8_t getImageIndex() const { return static_cast<uint8_t>(!download ? (!parent ? IMAGE_UPLOAD : IMAGE_U_USER) : (!parent ? IMAGE_DOWNLOAD : IMAGE_D_USER)); }

		ItemInfo* createParent();
		void updateUser(const vector<ItemInfo*>& aChildren);

		const string& getGroupCond() const;
		//const tstring& getIpText() const { return ip; }
	};

	struct UpdateInfo : public Task {
		enum {
			MASK_POS			= 0x01,
			MASK_SIZE			= 0x02,
			MASK_ACTUAL			= 0x04,
			MASK_SPEED			= 0x08,
			MASK_FILE			= 0x10,
			MASK_STATUS			= 0x20,
			MASK_TIMELEFT		= 0x40,
			MASK_IP				= 0x80,
			MASK_STATUS_STRING	= 0x100,
			MASK_CONNECTIONS	= 0x200,
			MASK_ENCRYPTION		= 0x400,
			MASK_TOTALSPEED		= 0x800,
			MASK_BUNDLE         = 0x1000,
			MASK_USERS          = 0x2000,
			MASK_USER           = 0x4000
		};

		bool operator==(const ItemInfo& ii) const {
			return download == ii.download && token == ii.token; 
		}

		UpdateInfo(string aToken, bool isDownload, bool aIsTransferFailed = false) : 
			user(HintedUser()), download(isDownload), token(std::move(aToken)), transferFailed(aIsTransferFailed)
		{ }

		void setUpdateFlags(const TransferInfoPtr& aInfo, int aUpdateFlags) noexcept;

		uint32_t updateMask = 0;

		string token;

		const bool download;
		const bool transferFailed;
		uint8_t flagIndex = 0;	

		void setConnections(int16_t aRunning) { connections = aRunning; updateMask |= MASK_CONNECTIONS; }
		int16_t connections = 0;
		void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos = 0;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size = 0;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual = 0;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed = 0;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft = 0;
		void setTotalSpeed(int64_t aTotalSpeed) { totalSpeed = aTotalSpeed; updateMask |= MASK_TOTALSPEED; }
		int64_t totalSpeed = 0;
		void setStatusString(const tstring& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		tstring statusString;
		void setTarget(const tstring& aTarget) { target = aTarget; updateMask |= MASK_FILE; }
		tstring target;
		void setIP(const FormatUtil::CountryFlagInfo& aInfo) { IP = aInfo.text; flagIndex = aInfo.flagIndex, updateMask |= MASK_IP; }
		tstring IP;
		void setEncryptionInfo(const tstring& aInfo) { Encryption = aInfo; updateMask |= MASK_ENCRYPTION; }
		tstring Encryption;
		void setType(const Transfer::Type& aType) { type = aType; }
		Transfer::Type type = Transfer::TYPE_LAST;
		void setBundle(const string& aBundle) { bundle = aBundle; updateMask |= MASK_BUNDLE; }
		void setBundle(QueueToken aBundle) { if (aBundle == 0) return; bundle = Util::toString(aBundle); updateMask |= MASK_BUNDLE; }
		string bundle;
		void setOnlineUsers(const int16_t aUsers) { onlineUsers = aUsers; updateMask |= MASK_USERS; }
		int16_t onlineUsers = 0;
		void setUser(const HintedUser& aUser) { user = aUser; updateMask |= MASK_USER; }
		HintedUser user;
	};

	void speak(uint8_t type, UpdateInfo* ui) { tasks.add(type, unique_ptr<Task>(ui)); PostMessage(WM_SPEAKER); }

	ItemInfoList ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	CImageListManaged arrows;

	TaskQueue tasks;

	ParamMap ucLineParams;


	/* Menu handlers */

	void performActionFiles(std::function<void (const ItemInfo* aInfo)> f, bool oncePerParent=false);
	void performActionBundles(std::function<void (const ItemInfo* aInfo)> f);

	void handleCollapseAll();
	void handleExpandAll();
	void handleSlowDisconnect();
	void handleRemoveBundle();
	void handleRemoveBundleSource();
	void handleOpenFolder();
	void handleForced();
	void handleSearchDir();
	void handleSearchAlternates();
	void handleRemoveFile();
	void handleDisconnect();

	/* Listeners */
	void on(DownloadManagerListener::BundleTick, const BundleList& bundles, uint64_t aTick) noexcept override;

	void on(UploadBundleInfoReceiverListener::BundleTick, const TickUploadBundleList& bundles) noexcept;
	void on(UploadBundleInfoReceiverListener::BundleComplete, const string& bundleToken, const string& bundleName) noexcept override;
	void on(UploadBundleInfoReceiverListener::BundleSizeName, const string& bundleToken, const string& newTarget, int64_t aSize) noexcept override;

	void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleDownloadStatus, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept override;

	void on(TransferInfoManagerListener::Added, const TransferInfoPtr& aInfo) noexcept override;
	void on(TransferInfoManagerListener::Updated, const TransferInfoPtr& aInfo, int aUpdatedProperties, bool aTick) noexcept override;
	void on(TransferInfoManagerListener::Removed, const TransferInfoPtr& aInfo) noexcept override;
	void on(TransferInfoManagerListener::Failed, const TransferInfoPtr& aInfo) noexcept override;
	void on(TransferInfoManagerListener::Tick, const TransferInfo::List& aTransfers, int aUpdatedProperties) noexcept override;
	void on(TransferInfoManagerListener::Completed, const TransferInfoPtr& aInfo) noexcept override;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	static const string& getBundle(const TransferInfoPtr& aInfo) noexcept;

	void onBundleName(const BundlePtr& aBundle);
	void onBundleComplete(const string& bundleToken, const string& bundleName, bool isUpload);
	void onBundleWaiting(const BundlePtr& aBundle, bool removed);

	ItemInfo* findItem(const UpdateInfo& ui, int& pos) const;
	void updateItem(const ItemInfo* aII, uint32_t updateMask);
	void setDefaultItem(OMenu& aMenu);

	void regroupItem(ItemInfo* ii, const UpdateInfo& ui, int pos) noexcept;

	static tstring getRunningStatus(const OrderedStringSet& aFlags) noexcept;
};
}

#endif // !defined(TRANSFER_VIEW_H)