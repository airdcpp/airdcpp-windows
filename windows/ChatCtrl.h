/* 
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

#ifndef CHAT_CTRL_H
#define CHAT_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Client.h"
#include "../client/ColorSettings.h"

#include "TypedListViewCtrl.h"
#include "ImageDataObject.h"
#include "UCHandler.h"
#include "../client/pme.h"

#ifndef _RICHEDIT_VER
# define _RICHEDIT_VER 0x0300
#endif

class UserInfo;

class ChatCtrl: public CRichEditCtrl, public CMessageMap, public UCHandler<ChatCtrl>
{
public:

	typedef UCHandler<ChatCtrl> ucBase;

	BEGIN_MSG_MAP(ChatCtrl)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onDoubleClick)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)
		MESSAGE_HANDLER(WM_SETCURSOR, onSetCursor)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove)

		MESSAGE_HANDLER(WM_LBUTTONUP, onLeftButtonUp)
		MESSAGE_HANDLER(WM_RBUTTONUP, onRightButtonUp)

		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

		COMMAND_ID_HANDLER(IDC_COPY_ACTUAL_LINE, onCopyActualLine)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_BAN_IP, onBanIP)
		COMMAND_ID_HANDLER(IDC_UNBAN_IP, onUnBanIP)
		COMMAND_ID_HANDLER(IDC_COPY_URL, onCopyURL)
		COMMAND_ID_HANDLER(IDC_WHOIS_IP, onWhoisIP)

		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_ID_HANDLER(IDC_SEARCH_BY_TTH, onSearchTTH)
		COMMAND_ID_HANDLER(IDC_GOOGLE_TITLE, onSearchSite)
		COMMAND_ID_HANDLER(IDC_GOOGLE_FULL, onSearchSite)
		COMMAND_ID_HANDLER(IDC_IMDB, onSearchSite)
		COMMAND_ID_HANDLER(IDC_TVCOM, onSearchSite)
		COMMAND_ID_HANDLER(IDC_METACRITIC, onSearchSite)
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + OnlineUser::COLUMN_LAST, onCopyUserInfo)

		CHAIN_COMMANDS(ucBase)

		MESSAGE_HANDLER(WM_COMMAND, onCommand)
	END_MSG_MAP()

	ChatCtrl();
	~ChatCtrl();

	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnRButtonDown(POINT pt);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) { return SendMessage(GetParent(), uMsg, wParam, lParam); }
	LRESULT onLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onRightButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool onClientEnLink(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled, bool rightButton=false);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchTTH(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
//	void setClient(Client* pClient) { client = pClient; }

	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) ;

	void runUserCommand(UserCommand& uc);

	//void AdjustTextSize();
	void AppendText(const Identity& i, const tstring& sMyNick, const tstring& sTime, tstring sMsg, CHARFORMAT2& cf, bool bUseEmo = true);

	void setSelectedUser(const tstring& s) { selectedUser = s; }
	const tstring& getSelectedUser() { return selectedUser; }

	int		FullTextMatch(ColorSettings* cs, CHARFORMAT2 &cf, const tstring &line, int pos, long &lineIndex);
	int		RegExpMatch(ColorSettings* cs, CHARFORMAT2 &cf, const tstring &line, long &lineIndex);
	void	CheckAction(ColorSettings* cs, const tstring& line);
	void Subclass() {
		ccw.SubclassWindow(this->CRichEditCtrl::m_hWnd);
		// We wanna control the scrolling...
	}

	GETSET(Client*, client, Client);
	
private:
	bool HitNick(const POINT& p, tstring& sNick, int& iBegin , int& iEnd);
	bool HitIP(const POINT& p, tstring& sIP, int& iBegin, int& iEnd);
	bool HitURL();

	void enableAutosearch() {
		//enable the autosearch once added something for it.. will change when move autosearch into frame anyway.
		if(!BOOLSETTING(AUTOSEARCH_ENABLED)) 
		SettingsManager::getInstance()->set(SettingsManager::AUTOSEARCH_ENABLED, true);
		if(!BOOLSETTING(AUTOSEARCH_ENABLED_TIME)) 
		SettingsManager::getInstance()->set(SettingsManager::AUTOSEARCH_ENABLED_TIME, true);
	}
	tstring getSearchString();
	tstring WordFromPos(const POINT& p);
	BOOL isRelease(POINT pt, BOOL search);
	tstring LineFromPos(const POINT& p) const;
	void FormatChatLine(const tstring& sMyNick, tstring& sMsg, CHARFORMAT2& cf, bool isMyMessage, const tstring& sAuthor, LONG lSelBegin, bool bUseEmo);
	void FormatEmoticonsAndLinks(tstring& sText, /*tstring& sTextLower,*/ LONG lSelBegin, bool bUseEmo);

	static string escapeUnicode(tstring str);
	static tstring rtfEscape(tstring str);

	void setText(const tstring& text) {
		string tmp = "{\\urtf " + escapeUnicode(rtfEscape(text)) + "}";
		// The cast below isn't a mistake...
		SetTextEx((LPCTSTR)tmp.c_str(), ST_SELECTION, CP_ACP);
	}
	void scrollToEnd();
	
	bool		matchedSound;
	bool		matchedPopup;
	bool		matchedTab;
	bool		logged;
	bool		skipLog;
	bool		timeStamps;
	bool		release;
	tstring		nick;
	tstring		searchTerm;
	tstring		searchTermFull;

    //Client* client;
	TStringMap shortLinks; //ApexDC
	
	bool m_bPopupMenu;
	
	OMenu copyMenu;
	OMenu SearchMenu;
	OMenu targetMenu;
	CContainedWindow ccw;

	tstring selectedLine;
	tstring selectedIP;
	tstring selectedUser;
	tstring selectedURL;
	tstring selectedWord;
	PME regUrl, regRelease;
	int64_t lastTick;
	bool isLink(POINT pt);
	bool handCursor;
};


#endif //!defined(AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_)
