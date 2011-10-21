/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(TRANSFER_VIEW_H)
#define TRANSFER_VIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\client\DownloadManagerListener.h"
#include "..\client\UploadManagerListener.h"
#include "..\client\ConnectionManagerListener.h"
#include "..\client\QueueManagerListener.h"
#include "..\client\TaskQueue.h"
#include "..\client\forward.h"
#include "..\client\Util.h"
#include "..\client\Download.h"
#include "..\client\Upload.h"

#include "OMenu.h"
#include "UCHandler.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "resource.h"
#include "SearchFrm.h"

class TransferView : public CWindowImpl<TransferView>, private DownloadManagerListener, 
	private UploadManagerListener, private ConnectionManagerListener, private QueueManagerListener,
	public UserInfoBaseHandler<TransferView>, public UCHandler<TransferView>,
	private SettingsManagerListener
{
public:
	DECLARE_WND_CLASS(_T("TransferView"))

	TransferView() : PreviewAppsSize(0) { }
	~TransferView(void);

	typedef UserInfoBaseHandler<TransferView> uibBase;
	typedef UCHandler<TransferView> ucBase;

	BEGIN_MSG_MAP(TransferView)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETDISPINFO, ctrlTransfers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_COLUMNCLICK, ctrlTransfers.onColumnClick)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_GETINFOTIP, ctrlTransfers.onInfoTip)
		NOTIFY_HANDLER(IDC_TRANSFERS, LVN_KEYDOWN, onKeyDownTransfers)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_TRANSFERS, NM_DBLCLK, onDoubleClickTransfers)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		COMMAND_ID_HANDLER(IDC_FORCE, onForce)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVEALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_OPEN_BUNDLE_FOLDER, onOpenBundleFolder)
		COMMAND_ID_HANDLER(IDC_REMOVE_BUNDLE, onRemoveBundle)
		COMMAND_ID_HANDLER(IDC_REMOVE_BUNDLE_FINISHED, onRemoveBundleFinished)
		COMMAND_ID_HANDLER(IDC_REMOVE_BUNDLE_SOURCE, onRemoveBundleSource)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_DISCONNECT_ALL, onDisconnectAll)
		COMMAND_ID_HANDLER(IDC_COLLAPSE_ALL, onCollapseAll)
		COMMAND_ID_HANDLER(IDC_EXPAND_ALL, onExpandAll)
		COMMAND_ID_HANDLER(IDC_MENU_SLOWDISCONNECT, onSlowDisconnect)
		COMMAND_ID_HANDLER(IDC_REMOVE_FILE, onRemoveFile)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_IP, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_HUB, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SPEED, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_STATUS, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_ALL, onCopy);
		COMMAND_ID_HANDLER(IDC_AUTOPRIORITY, onAutoPriority)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onPriority)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);			
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClickTransfers(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onDisconnectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSlowDisconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenBundleFolder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAutoPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void runUserCommand(UserCommand& uc);
	void prepareClose();

	LRESULT onCollapseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CollapseAll();
		return 0;
	}

	LRESULT onExpandAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ExpandAll();
		return 0;
	}

	LRESULT onKeyDownTransfers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::disconnect);
		return 0;
	}

	LRESULT onRemoveBundleSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeBundleSource);
		return 0;
	}

	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeAll);
		return 0;
	}

	LRESULT onRemoveBundle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeBundle);
		return 0;
	}

	LRESULT onRemoveBundleFinished(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTransfers.forEachSelected(&ItemInfo::removeBundleFinished);
		return 0;
	}

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
	typedef TypedTreeListViewCtrl<ItemInfo, IDC_TRANSFERS, tstring, noCaseStringHash, noCaseStringEq> ItemInfoList;
	ItemInfoList& getUserList() { return ctrlTransfers; }
private:
	enum {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_PARENT
	};

	enum {
		COLUMN_FIRST,
		COLUMN_USER = COLUMN_FIRST,
		COLUMN_FILE,
		COLUMN_HUB,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_CIPHER,
		COLUMN_IP,
		COLUMN_RATIO,
		COLUMN_LAST
	};

	enum {
		IMAGE_DOWNLOAD = 0,
		IMAGE_UPLOAD,
		IMAGE_SEGMENT
	};

	struct UpdateInfo;
	class ItemInfo : public UserInfoBase {
	public:
		enum Status {
			STATUS_RUNNING,
			STATUS_WAITING
			
		};
		
		ItemInfo(const HintedUser& u, const string aToken, bool aDownload, bool aBundle = false) : user(u), download(aDownload), transferFailed(false),
			status(STATUS_WAITING), pos(0), size(0), actual(0), speed(0), timeLeft(0), totalSpeed(0)/*ttlf*/, ip(Util::emptyStringT), target(Util::emptyStringT),
			flagIndex(0), collapsed(true), parent(NULL), hits(-1), statusString(Util::emptyStringT), running(0), token(aToken), isBundle(aBundle), bundle(Util::emptyStringT),
			users(0) { }

		bool isBundle;
		bool download;
		bool transferFailed;
		bool collapsed;
		
		uint8_t flagIndex;
		int16_t running;
		int16_t hits;
		int16_t users;

		ItemInfo* parent;
		HintedUser user;
		string token;
		tstring bundle;
		Status status;
		Transfer::Type type;
		
		int64_t pos;
		int64_t size;
		int64_t actual;
		int64_t speed;
		int64_t timeLeft;
		int64_t totalSpeed;
		
		tstring ip;
		tstring statusString;
		tstring target;
		tstring cipher;

		void update(const UpdateInfo& ui);

		const UserPtr& getUser() const { return user.user; }

		void disconnect();
		void removeAll();
		void removeBundle();
		void removeBundleFinished();
		void removeBundleSource();

		double getRatio() const { return (pos > 0) ? (double)actual / (double)pos : 1.0; }

		const tstring getText(uint8_t col) const;
		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		uint8_t getImageIndex() const { return static_cast<uint8_t>(!download ? IMAGE_UPLOAD : (!parent ? IMAGE_DOWNLOAD : IMAGE_SEGMENT)); }

		ItemInfo* createParent();

		inline const tstring& getGroupCond() const;
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
			MASK_SEGMENT		= 0x200,
			MASK_CIPHER			= 0x400,
			MASK_TOTALSPEED		= 0x800, /* ttlf */
			MASK_BUNDLE         = 0x1000,
			MASK_USERS          = 0x2000,
			MASK_USER           = 0x4000
		};

		bool operator==(const ItemInfo& ii) const {
			return download == ii.download && token == ii.token; 
		}

		UpdateInfo(string aToken, bool isDownload, bool isTransferFailed = false) : 
			updateMask(0), user(HintedUser(NULL, Util::emptyString)), download(isDownload), token(aToken), transferFailed(isTransferFailed), flagIndex(0), type(Transfer::TYPE_LAST)
		{ }

		uint32_t updateMask;

		string token;

		bool download;
		bool transferFailed;
		uint8_t flagIndex;		
		void setRunning(int16_t aRunning) { running = aRunning; updateMask |= MASK_SEGMENT; }
		int16_t running;
		void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft;
		void setTotalSpeed(int64_t aTotalSpeed) { totalSpeed = aTotalSpeed; updateMask |= MASK_TOTALSPEED; }
		int64_t totalSpeed;
		void setStatusString(const tstring& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		tstring statusString;
		void setTarget(const tstring& aTarget) { target = aTarget; updateMask |= MASK_FILE; }
		tstring target;
		void setIP(const tstring& aIP, uint8_t aFlagIndex) { IP = aIP; flagIndex = aFlagIndex, updateMask |= MASK_IP; }
		tstring IP;
		void setCipher(const tstring& aCipher) { cipher = aCipher; updateMask |= MASK_CIPHER; }
		tstring cipher;
		void setType(const Transfer::Type& aType) { type = aType; }
		Transfer::Type type;
		void setBundle(const string& aBundle) { bundle = Text::toT(aBundle); updateMask |= MASK_BUNDLE; }
		tstring bundle;
		void setUsers(const int16_t aUsers) { users = aUsers; updateMask |= MASK_USERS; }
		int16_t users;
		void setUser(const HintedUser aUser) { user = aUser; updateMask |= MASK_USER; }
		HintedUser user;
	};

	void speak(uint8_t type, UpdateInfo* ui) { tasks.add(type, unique_ptr<Task>(ui)); PostMessage(WM_SPEAKER); }

	ItemInfoList ctrlTransfers;
	static int columnIndexes[];
	static int columnSizes[];

	CImageList arrows;

	TaskQueue tasks;

	StringMap ucLineParams;
	int PreviewAppsSize;

	void on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi) noexcept;
	void on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const string& aReason) noexcept;
	void on(ConnectionManagerListener::Removed, const ConnectionQueueItem* aCqi) noexcept;
	void on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem* aCqi) noexcept;
	//void on(ConnectionManagerListener::TokenChanged, const ConnectionQueueItem* aCqi, string newToken) {

	void on(DownloadManagerListener::Requesting, const Download* aDownload) noexcept;	
	void on(DownloadManagerListener::Complete, const Download* aDownload, bool isTree) noexcept { 
		onTransferComplete(aDownload, false, Util::getFileName(aDownload->getPath()), isTree, (aDownload->getBundle() ? aDownload->getBundle()->getToken() : Util::emptyString));
	}
	void on(DownloadManagerListener::Failed, const Download* aDownload, const string& aReason) noexcept;
	void on(DownloadManagerListener::Starting, const Download* aDownload) noexcept;
	void on(DownloadManagerListener::Tick, const DownloadList& aDownload, const BundleList& aBundle) noexcept;
	void on(DownloadManagerListener::Status, const UserConnection*, const string&) noexcept;
	void on(DownloadManagerListener::BundleFinished, const string& bundleToken) noexcept { onBundleComplete(bundleToken, bundleToken, false); }
	void on(DownloadManagerListener::BundleUser, const string& bundleToken, const HintedUser& aUser) noexcept;
	void on(DownloadManagerListener::Target, const UserPtr& user, const string& aToken) noexcept;

	void on(UploadManagerListener::Starting, const Upload* aUpload) noexcept;
	void on(UploadManagerListener::Tick, const UploadList& aUpload, const UploadBundleList& bundles) noexcept;
	void on(UploadManagerListener::Complete, const Upload* aUpload) noexcept { 
		onTransferComplete(aUpload, true, aUpload->getPath(), false, (aUpload->getBundle() ? aUpload->getBundle()->getToken() : Util::emptyString)); 
	}
	void on(UploadManagerListener::BundleComplete, const string& bundleToken, const string& bundleName) noexcept { onBundleComplete(bundleToken, bundleName, true); }
	void on(UploadManagerListener::BundleRenamed, const string& bundleToken, const string& bundleTarget) noexcept { onBundleName(bundleToken, bundleTarget, true); }

	void on(QueueManagerListener::BundleFinished, const BundlePtr aBundle) noexcept { onBundleComplete(aBundle->getToken(), aBundle->getName(), false); }
	void on(QueueManagerListener::BundleWaiting, const BundlePtr aBundle) noexcept { onBundleStatus(aBundle, false); }
	void on(QueueManagerListener::BundleRemoved, const BundlePtr aBundle) noexcept { onBundleStatus(aBundle, true); }
	void on(QueueManagerListener::BundleRenamed, const BundlePtr aBundle) noexcept { onBundleName(aBundle->getToken(), aBundle->getTarget(), false); }

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void onBundleComplete(const string& bundleToken, const string& bundleName, bool isUpload);
	void onBundleStatus(const BundlePtr aBundle, bool removed);
	void onBundleUser(const BundlePtr aBundle);
	void onBundleName(const string& bundleToken, const string& aTarget, bool isUpload);
	void onTransferComplete(const Transfer* aTransfer, bool isUpload, const string& aFileName, bool isTree, const string& bundleToken);
	void starting(UpdateInfo* ui, const Transfer* t);
	
	void CollapseAll();
	void ExpandAll();

	ItemInfo* findItem(const UpdateInfo& ui, int& pos) const;
	void updateItem(int ii, uint32_t updateMask);
};

#endif // !defined(TRANSFER_VIEW_H)

/**
 * @file
 * $Id: TransferView.h 500 2010-06-25 22:08:18Z bigmuscle $
 */