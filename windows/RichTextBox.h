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

#include "../client/AirUtil.h"
#include "../client/Client.h"
#include "../client/ColorSettings.h"
#include "../client/TargetUtil.h"
#include "../client/ChatMessage.h"

#include <boost/algorithm/string.hpp>

#include "TypedListViewCtrl.h"
#include "ImageDataObject.h"
#include "UCHandler.h"
#include "DownloadBaseHandler.h"

#ifndef MSFTEDIT_CLASS
#define MSFTEDIT_CLASS L"RICHEDIT50W";
#endif

class UserInfo;

class RichTextBox: public CRichEditCtrl, public CMessageMap, public UCHandler<RichTextBox>, public DownloadBaseHandler<RichTextBox>
{
public:

	typedef UCHandler<RichTextBox> ucBase;

	BEGIN_MSG_MAP(RichTextBox)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)
		MESSAGE_HANDLER(WM_SETCURSOR, onSetCursor)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove)
		MESSAGE_HANDLER(WM_FINDREPLACE, onFind)
		MESSAGE_HANDLER(WM_CHAR, onChar)

		MESSAGE_HANDLER(WM_LBUTTONUP, onLeftButtonUp)

		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

		COMMAND_ID_HANDLER(IDC_COPY_ACTUAL_LINE, onCopyActualLine)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_BAN_IP, onBanIP)
		COMMAND_ID_HANDLER(IDC_UNBAN_IP, onUnBanIP)
		COMMAND_ID_HANDLER(IDC_WHOIS_IP, onWhoisIP)

		COMMAND_ID_HANDLER(IDC_FIND_TEXT, onFindText)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_PRIVATEMESSAGE, onPrivateMessage)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
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
		COMMAND_ID_HANDLER(IDC_CONNECT_WITH, onConnectWith)
		COMMAND_ID_HANDLER(IDC_OPEN_LINK, onOpenLink)
		COMMAND_ID_HANDLER(IDC_OPEN, onOpen)

		COMMAND_ID_HANDLER(IDC_REMOVE, onRemoveTemp)
		COMMAND_ID_HANDLER(IDC_DELETE_FILE, onDeleteFile)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchDir)

		COMMAND_ID_HANDLER(IDC_ADD_AUTO_SEARCH_FILE, onAddAutoSearchFile)
		COMMAND_ID_HANDLER(IDC_ADD_AUTO_SEARCH_DIR, onAddAutoSearchDir)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenDupe)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + OnlineUser::COLUMN_LAST, onCopyUserInfo)

		CHAIN_COMMANDS(ucBase)

		MESSAGE_HANDLER(WM_COMMAND, onCommand)
	END_MSG_MAP()

	RichTextBox();
	~RichTextBox();

	LRESULT onDeleteFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddAutoSearchFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddAutoSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnRButtonDown(POINT pt);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) { return SendMessage(GetParent(), uMsg, wParam, lParam); }
	LRESULT onLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool onClientEnLink(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onOpenLink(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onConnectWith(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchTTH(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
//	void setClient(Client* pClient) { client = pClient; }

	LRESULT onOpenDupe(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveTemp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFindText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

	void findText(tstring& aTxt=Util::emptyStringT);
	void runUserCommand(UserCommand& uc);

	typedef boost::iterator_range<boost::range_const_iterator<tstring>::type> tstring_range;
	static std::string unicodeEscapeFormatter(const tstring_range& match);
	static tstring rtfEscapeFormatter(const tstring_range& match);

	static std::string escapeUnicode(const tstring& str);
	/// escape Rich Edit control chars: {, }, and \, as well as \n which becomes \line.
	static tstring rtfEscape(const tstring& str);

	//void AdjustTextSize();
	void AppendText(tstring& sMsg);
	bool AppendChat(const Identity& i, const tstring& sMyNick, const tstring& sTime, tstring sMsg, CHARFORMAT2& cf, bool bUseEmo = true);
	void AppendHTML(const string& aTxt);
	tstring getLinkText(const ENLINK& link);
	LRESULT handleLink(ENLINK& link);

	void setSelectedUser(const tstring& s) { selectedUser = s; }
	const tstring& getSelectedUser() { return selectedUser; }

	size_t	FullTextMatch(ColorSettings* cs, CHARFORMAT2 &cf, const tstring &line, size_t pos, long &lineIndex);
	size_t	RegExpMatch(ColorSettings* cs, CHARFORMAT2 &cf, const tstring &line, long &lineIndex);
	void	CheckAction(ColorSettings* cs, const tstring& line);
	void Subclass() {
		ccw.SubclassWindow(this->CRichEditCtrl::m_hWnd);
		// We wanna control the scrolling...
	}

	string getTempShareKey() const;

	void FormatEmoticonsAndLinks(tstring& sText, /*tstring& sTextLower,*/ LONG lSelBegin, bool bUseEmo);
	GETSET(Client*, client, Client);
	GETSET(UserPtr, user, User);

	GETSET(bool, autoScrollToEnd, AutoScrollToEnd);
	GETSET(bool, formatLinks, FormatLinks);
	GETSET(bool, formatPaths, FormatPaths);
	GETSET(bool, formatReleases, FormatReleases);
	GETSET(bool, allowClear, AllowClear);

	/* DownloadBaseHandler functions */
	void appendDownloadItems(OMenu& aMenu, bool isWhole);
	void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool isRelease, TargetUtil::TargetType aTargetType, bool isSizeUnknown);
	int64_t getDownloadSize(bool isWhole);
	bool showDirDialog(string& fileName);
	HintedUser getMagnetSource();
private:
	void clearSelInfo();
	void updateSelectedWord(POINT pt);
	static UINT	WM_FINDREPLACE;
	TCHAR*		findBuffer;
	const WORD	findBufferSize;

	bool HitNick(const POINT& p, tstring& sNick, int& iBegin , int& iEnd);
	bool HitIP(const POINT& p, tstring& sIP, int& iBegin, int& iEnd);
	bool updateAuthor();
	static void unifyLineEndings(tstring& aText);

	tstring WordFromPos(const POINT& p);
	tstring LineFromPos(const POINT& p) const;
	void FormatChatLine(const tstring& sMyNick, tstring& sMsg, CHARFORMAT2& cf, bool isMyMessage, const tstring& sAuthor, LONG lSelBegin, bool bUseEmo);

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

	bool		isPath;
	bool		isRelease;
	DupeType	dupeType;
	bool		isMagnet;
	bool		isTTH;
	tstring		author;
	int t_height; //text height
	
	bool m_bPopupMenu;
	
	OMenu copyMenu;
	CContainedWindow ccw;

	tstring selectedLine;
	tstring selectedIP;
	tstring selectedUser;
	tstring selectedWord;
	uint64_t lastTick;
	bool isLink(POINT pt);
	ChatLink* getLink(POINT pt, CHARRANGE& cr);
	bool showHandCursor;

	vector<pair<CHARRANGE, ChatLink*>> links;

	HCURSOR		handCursor;
	HCURSOR		arrowCursor;

	void formatLink(DupeType aDupeType, bool isRelease);
	DupeType updateDupeType(ChatLink* aChatLink);
};


#endif //!defined(AFX_CHAT_CTRL_H__595F1372_081B_11D1_890D_00A0244AB9FD__INCLUDED_)
