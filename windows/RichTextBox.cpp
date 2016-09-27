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

#include "stdafx.h"
#include "Resource.h"

#include <airdcpp/AutoSearchManager.h>
#include <airdcpp/FavoriteManager.h>
#include <airdcpp/HighlightManager.h>
#include <airdcpp/MessageManager.h>
#include <airdcpp/Magnet.h>
#include <airdcpp/UploadManager.h>
#include <airdcpp/QueueManager.h>
#include <airdcpp/Util.h>
#include <airdcpp/version.h>

#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#include "RichTextBox.h"
#include "EmoticonsManager.h"
#include "HubFrame.h"
#include "PrivateFrame.h"
#include "LineDlg.h"
#include "atlstr.h"
#include "MainFrm.h"
#include "TextFrame.h"
#include "ResourceLoader.h"

#include "HtmlToRtf.h"

EmoticonsManager* emoticonsManager = NULL;

#define MAX_EMOTICONS 48
UINT RichTextBox::WM_FINDREPLACE = RegisterWindowMessage(FINDMSGSTRING);

RichTextBox::ChatLink::ChatLink(const string& aUrl, LinkType aLinkType, const UserPtr& aUser) : url(aUrl), type(aLinkType), dupe(DUPE_NONE) {
	updateDupeType(aUser);
}

DupeType RichTextBox::ChatLink::updateDupeType(const UserPtr& aUser) {
	if (type == TYPE_RELEASE) {
		dupe = AirUtil::checkDirDupe(url, 0);
	} else if (type == TYPE_MAGNET) {
		Magnet m = Magnet(url);
		dupe = m.getDupeType();
		if (dupe == DUPE_NONE && ShareManager::getInstance()->isTempShared(aUser ? aUser->getCID().toBase32() : Util::emptyString, m.getTTH())) {
			dupe = DUPE_SHARE_FULL;
		}
	}
	return dupe;
}

string RichTextBox::ChatLink::getDisplayText() {
	if (type == TYPE_SPOTIFY) {
		boost::regex regSpotify;
		regSpotify.assign("(spotify:(artist|track|album):[A-Z0-9]{22})", boost::regex_constants::icase);

		if (boost::regex_match(url, regSpotify)) {
			string sType, displayText;
			size_t found = url.find_first_of(":");
			string tmp = url.substr(found + 1, url.length());
			found = tmp.find_first_of(":");
			if (found != string::npos) {
				sType = tmp.substr(0, found);
			}

			if (Util::stricmp(sType.c_str(), "track") == 0) {
				displayText = STRING(SPOTIFY_TRACK);
			}
			else if (Util::stricmp(sType.c_str(), "artist") == 0) {
				displayText = STRING(SPOTIFY_ARTIST);
			}
			else if (Util::stricmp(sType.c_str(), "album") == 0) {
				displayText = STRING(SPOTIFY_ALBUM);
			}
			return displayText;
		}
		//some other spotify link, just show the original url
	}
	else if (type == TYPE_MAGNET) {
		Magnet m = Magnet(url);
		if (!m.fname.empty()) {
			return m.fname + " (" + Util::formatBytes(m.fsize) + ")";
		}
	}

	return url;
}


RichTextBox::RichTextBox() : UserInfoBaseHandler(true, true), ccw(const_cast<LPTSTR>(GetWndClassName()), this), client(NULL), m_bPopupMenu(false), autoScrollToEnd(true), findBufferSize(100), pmUser(nullptr), formatLinks(false),
	formatPaths(false), formatReleases(false), allowClear(false) {
	if(emoticonsManager == NULL) {
		emoticonsManager = new EmoticonsManager();
	}

	showHandCursor=false;
	lastTick = GET_TICK();
	emoticonsManager->inc();
	t_height = WinUtil::getTextHeight(m_hWnd, WinUtil::font); //? right height?

	arrowCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	handCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));

	findBuffer = new TCHAR[findBufferSize];
	findBuffer[0] = _T('\0');
}

RichTextBox::~RichTextBox() {
	if(emoticonsManager->unique()) {
		emoticonsManager->dec();
		emoticonsManager = NULL;
	} else {
		emoticonsManager->dec();
	}

	for (auto& cl: links)
		delete cl.second;

	delete[] findBuffer;
}

std::string RichTextBox::unicodeEscapeFormatter(const tstring_range& match) {
	if(match.empty())
		return std::string();
	return (boost::format("\\ud\\u%dh")%(int(*match.begin()))).str();
}

std::string RichTextBox::escapeUnicode(const tstring& str) {
	std::string ret;
	boost::find_format_all_copy(std::back_inserter(ret), str,
		boost::first_finder(L"\x7f", std::greater<TCHAR>()), unicodeEscapeFormatter);
	return ret;
}

tstring RichTextBox::rtfEscapeFormatter(const tstring_range& match) {
	if(match.empty())
		return tstring();
	tstring s(1, *match.begin());
	if (s == _T("\r")) return _T("");
	//if (s == _T("\n")) return _T("\\line\n");
	return _T("\\") + s;
}

tstring RichTextBox::rtfEscape(const tstring& str) {
	using boost::lambda::_1;
	tstring escaped;
	boost::find_format_all_copy(std::back_inserter(escaped), str,
		boost::first_finder(L"\x7f", _1 == '{' || _1 == '}' || _1 == '\\' || _1 == '\n' || _1 == '\r'), rtfEscapeFormatter);
	return escaped;
}

void RichTextBox::unifyLineEndings(tstring& aText) {
	tstring::size_type j = 0; 
	while((j = aText.find(_T("\r"), j)) != tstring::npos)
		aText.erase(j, 1);
}

void RichTextBox::AppendText(tstring& sMsg) {
	SetRedraw(FALSE);
	unifyLineEndings(sMsg);
	SetWindowText(sMsg.c_str());
	FormatEmoticonsAndLinks(sMsg, 0, false);
	SetRedraw(TRUE);
}

bool RichTextBox::AppendChat(const Identity& identity, const tstring& sMyNick, const tstring& sTime, tstring sMsg, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	SetRedraw(FALSE);
	matchedTab = false;

	SCROLLINFO si = { 0 };
	POINT pt = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);
	GetScrollPos(&pt);

	LONG lSelBegin = 0, lSelEnd = 0, lTextLimit = 0, lNewTextLen = 0;
	LONG lSelBeginSaved, lSelEndSaved;

	// Unify line endings
	unifyLineEndings(sMsg);

	GetSel(lSelBeginSaved, lSelEndSaved);
	lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);

	bool isMyMessage = identity.getUser() == ClientManager::getInstance()->getMe();
	tstring sLine = sTime + sMsg;

	// Remove old chat if size exceeds
	lNewTextLen = sLine.size();
	lTextLimit = GetLimitText();

	if(lSelEnd + lNewTextLen > lTextLimit) {
		LONG lRemoveChars = 0;
		int multiplier = 1;

		if(lNewTextLen >= lTextLimit) {
			lRemoveChars = lSelEnd;
		} else {
			int lastRemoved = -1;
			while (lRemoveChars < lNewTextLen) {
				lRemoveChars = LineIndex(LineFromChar(multiplier++ * lTextLimit / 10));
				if (lRemoveChars == lastRemoved) // it may get stuck which means an infinite loop
					break;
				lastRemoved = lRemoveChars;
			}
		}

		//remove old links (the list must be in the same order than in text)
		for(auto i = links.begin(); i != links.end();) {
			if ((*i).first.cpMin < lRemoveChars) {
				delete i->second;
				links.erase(i);
				i = links.begin();
			} else {
				//update the position
				(*i).first.cpMin -= lRemoveChars;
				(*i).first.cpMax -= lRemoveChars;
				++i;
			}
		}

		// Update selection ranges
		lSelEnd = lSelBegin -= lRemoveChars;
		lSelEndSaved -= lRemoveChars;
		lSelBeginSaved -= lRemoveChars;

		// ...and the scroll position
		pt.y -= PosFromChar(lRemoveChars).y;

		SetSel(0, lRemoveChars);
		ReplaceSel(_T(""));
	}

	// Add to the end
	SetSel(lSelBegin, lSelEnd);
	setText(sLine);

	CHARFORMAT2 enc;
	enc.bCharSet = RUSSIAN_CHARSET;
	enc.dwMask = CFM_CHARSET;

	SetSel(0, sLine.length());
	SetSelectionCharFormat(enc);

	bool isFavorite = false, isOp = false;
	if (identity.getUser()) {
		isFavorite = identity.getUser()->isFavorite();
		isOp = identity.isOp();
	}

	// Format TimeStamp
	if(!sTime.empty()) {
		lSelEnd += sTime.size();
		SetSel(lSelBegin, lSelEnd - 1);
		SetSelectionCharFormat(WinUtil::m_TextStyleTimestamp);

		PARAFORMAT2 pf;
		memzero(&pf, sizeof(PARAFORMAT2));
		pf.dwMask = PFM_STARTINDENT; 
		pf.dxStartIndent = 0;
		SetParaFormat(pf);
	}

	// Authors nick
	tstring sAuthor = Text::toT(identity.getNick());
	if(!sAuthor.empty()) {
		LONG iLen = (sMsg[0] == _T('*')) ? 1 : 0;
		LONG iAuthorLen = sAuthor.size() + 1;
		sMsg.erase(0, iAuthorLen + iLen);
   		
		lSelBegin = lSelEnd;
		lSelEnd += iAuthorLen + iLen;
		
		if(isMyMessage) {
			SetSel(lSelBegin, lSelBegin + iLen + 1);
			SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
			SetSel(lSelBegin + iLen + 1, lSelBegin + iLen + iAuthorLen);
			SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		} else {

			//if(isFavorite || i.isOp()) {
				SetSel(lSelBegin, lSelBegin + iLen + 1);
				SetSelectionCharFormat(cf);
				SetSel(lSelBegin + iLen + 1, lSelEnd);
				if(isFavorite){
					SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
				} else if(identity.isOp()) {
					SetSelectionCharFormat(WinUtil::m_TextStyleOPs);
				} else {
					SetSelectionCharFormat(WinUtil::m_TextStyleNormUsers);
				}
			//} else {
			//	SetSel(lSelBegin, lSelEnd);
			//	SetSelectionCharFormat(cf);
            //}
		}
	} else {
		bool thirdPerson = false;
        switch(sMsg[0]) {
			case _T('*'):
				if(sMsg[1] != _T(' ')) break;
				thirdPerson = true;
            case _T('<'):
				tstring::size_type iAuthorLen = sMsg.find(thirdPerson ? _T(' ') : _T('>'), thirdPerson ? 2 : 1);
				if(iAuthorLen != tstring::npos) {
					tstring nick(sMsg.c_str() + 1);
					nick.erase(iAuthorLen - 1);
                    
					lSelBegin = lSelEnd;
					lSelEnd += iAuthorLen;
					sMsg.erase(0, iAuthorLen);

        			//if(isFavorite || isOp) {
        				SetSel(lSelBegin, lSelBegin + 1);
        				SetSelectionCharFormat(cf);
						SetSel(lSelBegin + 1, lSelEnd);
						if(isFavorite){
							SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
						} else if(isOp) {
							SetSelectionCharFormat(WinUtil::m_TextStyleOPs);
						} else {
							SetSelectionCharFormat(WinUtil::m_TextStyleNormUsers);
						}
        			//} else {
        			//	SetSel(lSelBegin, lSelEnd);
        			//	SetSelectionCharFormat(cf);
                    //}
				}
        }
	}

	// Format the message part
	FormatChatLine(sMyNick, sMsg, cf, isMyMessage, sAuthor, lSelEnd, bUseEmo);

	SetSel(lSelBeginSaved, lSelEndSaved);
	if(	isMyMessage || ((si.nPage == 0 || (size_t)si.nPos >= (size_t)si.nMax - si.nPage - 5) &&
		(lSelBeginSaved == lSelEndSaved || !selectedUser.empty() || !selectedIP.empty())))
	{
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
	} else {
		SetScrollPos(&pt);
	}

	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
	return matchedTab;
}

void RichTextBox::FormatChatLine(const tstring& sMyNick, tstring& sText, CHARFORMAT2& cf, bool isMyMessage, const tstring& sAuthor, LONG lSelBegin, bool bUseEmo) {
	// Set text format
	tstring sMsgLower(sText.length(), NULL);
	std::transform(sText.begin(), sText.end(), sMsgLower.begin(), _totlower);

	LONG lSelEnd = lSelBegin + sText.size();
	SetSel(lSelBegin, lSelEnd);
	SetSelectionCharFormat(isMyMessage ? WinUtil::m_ChatTextMyOwn : cf);
	
	// highlight all occurences of my nick
	long lMyNickStart = -1, lMyNickEnd = -1;
	size_t lSearchFrom = 0;	
	tstring sNick(sMyNick.length(), NULL);
	std::transform(sMyNick.begin(), sMyNick.end(), sNick.begin(), _totlower);

	if (!sNick.empty()) {
		bool found = false;
		while ((lMyNickStart = (long) sMsgLower.find(sNick, lSearchFrom)) != tstring::npos) {
			lMyNickEnd = lMyNickStart + (long) sNick.size();
			SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
			SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
			lSearchFrom = lMyNickEnd;
			found = true;
		}

		if (found) {
			if (!SETTING(CHATNAMEFILE).empty() && !SETTING(SOUNDS_DISABLED) &&
				!sAuthor.empty() && (stricmp(sAuthor.c_str(), sNick) != 0)) {
					WinUtil::playSound(Text::toT(SETTING(CHATNAMEFILE)));
			}
			if (SETTING(FLASH_WINDOW_ON_MYNICK)
				&& !sAuthor.empty() && (stricmp(sAuthor.c_str(), sNick) != 0))
				WinUtil::FlashWindow();
		}
	}

	// highlight all occurences of favourite users' nicks
	{
		RLock l(FavoriteManager::getInstance()->cs);
		auto& ul = FavoriteManager::getInstance()->getFavoriteUsers();
		for (const auto& pUser : ul | map_values) {
			lSearchFrom = 0;
			sNick = Text::toT(pUser.getNick());
			if (sNick.empty()) continue;
			std::transform(sNick.begin(), sNick.end(), sNick.begin(), _totlower);

			while ((lMyNickStart = (long) sMsgLower.find(sNick, lSearchFrom)) != tstring::npos) {
				lMyNickEnd = lMyNickStart + (long) sNick.size();
				SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
				SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
				lSearchFrom = lMyNickEnd;
			}
		}
	}
	

	if(SETTING(USE_HIGHLIGHT)) {

		ColorList *cList = HighlightManager::getInstance()->getList();
		CHARFORMAT2 hlcf;
		logged = false;
		//compare the last line against all strings in the vector
		for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
			ColorSettings* cs = &(*i);
			if(cs->getContext() != HighlightManager::CONTEXT_CHAT) 
				continue;
			size_t pos;
			tstring msg = sText;

			//set start position for find
			pos = msg.find(_T(">"));

			//prepare the charformat
			memset(&hlcf, 0, sizeof(CHARFORMAT2));
			hlcf.cbSize = sizeof(hlcf);
			hlcf.dwReserved = 0;
			hlcf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;

			if(cs->getBold())		hlcf.dwEffects |= CFE_BOLD;
			if(cs->getItalic())		hlcf.dwEffects |= CFE_ITALIC;
			if(cs->getUnderline())	hlcf.dwEffects |= CFM_UNDERLINE;
			if(cs->getStrikeout())	hlcf.dwEffects |= CFM_STRIKEOUT;
		
			if(cs->getHasBgColor())
				hlcf.crBackColor = cs->getBgColor();
			else
				hlcf.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	
			if(cs->getHasFgColor())
				hlcf.crTextColor = cs->getFgColor();
			else
				hlcf.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
		
			while( pos != string::npos ){
				if(cs->usingRegexp()) 
					pos = RegExpMatch(cs, hlcf, msg, lSelBegin);
				else 
					pos = FullTextMatch(cs, hlcf, msg, pos, lSelBegin);
			}

			matchedPopup = false;
			matchedSound = false;
		}//end for
	}


	// Links and smilies
	FormatEmoticonsAndLinks(sText, /*sMsgLower,*/ lSelBegin, bUseEmo);
}

void RichTextBox::FormatEmoticonsAndLinks(tstring& sMsg, /*tstring& sMsgLower,*/ LONG lSelBegin, bool bUseEmo) {
	if (formatLinks) {
		try {
			auto start = sMsg.cbegin();
			auto end = sMsg.cend();
			boost::match_results<tstring::const_iterator> result;
			int pos=0;

			while(boost::regex_search(start, end, result, WinUtil::chatLinkReg, boost::match_default)) {
				size_t linkStart = pos + lSelBegin + result.position();
				size_t linkEnd = pos + lSelBegin + result.position() + result.length();
				SetSel(linkStart, linkEnd);
				tstring link( result[0].first, result[0].second );

				//create the link
				ChatLink::LinkType type = ChatLink::TYPE_URL;
				if (link.find(_T("magnet:?")) != tstring::npos) {
					type = ChatLink::TYPE_MAGNET;
				} else if (link.find(_T("spotify:")) != tstring::npos) {
					type = ChatLink::TYPE_SPOTIFY;
				}

				ChatLink* cl = new ChatLink(Text::fromT(link), type, pmUser);
				formatLink(cl->getDupe(), false);

				//replace the text displayed in chat
				tstring displayText = Text::toT(cl->getDisplayText());
				sMsg.replace(result[0].first, result[0].second, displayText.c_str());
				setText(displayText);

				//store the link and the range
				CHARRANGE cr;
				cr.cpMin = linkStart;
				cr.cpMax = linkStart + displayText.length();
				//links.emplace_back(cr, cl);
				links.insert_sorted(ChatLinkPair(cr, cl));

				pos=pos+result.position() + displayText.length();
				start = result[0].first + displayText.length();
				end = sMsg.end();
			}
	
		} catch(...) { 
			//...
		}
	}

	//Format release names
	if(formatReleases && (SETTING(FORMAT_RELEASE) || SETTING(DUPES_IN_CHAT))) {
		auto start = sMsg.cbegin();
		auto end = sMsg.cend();
		boost::match_results<tstring::const_iterator> result;
		int pos=0;

		while(boost::regex_search(start, end, result, WinUtil::chatReleaseReg, boost::match_default)) {
			CHARRANGE cr;
			cr.cpMin = pos + lSelBegin + result.position();
			cr.cpMax = pos + lSelBegin + result.position() + result.length();

			SetSel(cr.cpMin, cr.cpMax);

			std::string link (result[0].first, result[0].second);
			ChatLink* cl = new ChatLink(link, ChatLink::TYPE_RELEASE, pmUser);

			formatLink(cl->getDupe(), true);

			links.insert_sorted(ChatLinkPair(cr, cl));
			start = result[0].second;
			pos=pos+result.position() + result.length();
		}
	}

	//Format local paths
	if (formatPaths) {
		auto start = sMsg.cbegin();
		auto end = sMsg.cend();
		boost::match_results<tstring::const_iterator> result;
		int pos=0;

		while(boost::regex_search(start, end, result, WinUtil::pathReg, boost::match_default)) {
			CHARRANGE cr;
			cr.cpMin = pos + lSelBegin + result.position();
			cr.cpMax = pos + lSelBegin + result.position() + result.length();

			SetSel(cr.cpMin, cr.cpMax);
			SetSelectionCharFormat(WinUtil::m_ChatTextServer);

			std::string path (result[0].first, result[0].second);
			ChatLink* cl = new ChatLink(path, ChatLink::TYPE_PATH, nullptr);
			//links.emplace_back(cr, cl);
			links.insert_sorted(ChatLinkPair(cr, cl));

			start = result[0].second;
			pos=pos+result.position() + result.length();
		}
	}

	// insert emoticons
	if(bUseEmo && emoticonsManager->getUseEmoticons()) {
		const Emoticon::List& emoticonsList = emoticonsManager->getEmoticonsList();
		tstring::size_type lastReplace = 0;
		uint8_t smiles = 0;
		LONG lSelEnd=0;

		while(true) {
			tstring::size_type curReplace = tstring::npos;
			Emoticon* foundEmoticon = NULL;

			for(auto emoticon: emoticonsList) {
				tstring::size_type idxFound = sMsg.find(emoticon->getEmoticonText(), lastReplace);
				if(idxFound < curReplace || curReplace == tstring::npos) {
					curReplace = idxFound;
					foundEmoticon = emoticon;
				}
			}

			if(curReplace != tstring::npos && smiles < MAX_EMOTICONS) {
				CHARFORMAT2 cfSel;
				cfSel.cbSize = sizeof(cfSel);
				lSelBegin += (curReplace - lastReplace);
				lSelEnd = lSelBegin + foundEmoticon->getEmoticonText().size();

				// Check the position (don't replace partial word segments)
				if (curReplace != lastReplace && curReplace > 0 && iswgraph(sMsg[curReplace-1])) {
					lSelBegin = lSelEnd;
				} else {
					SetSel(lSelBegin, lSelEnd);

					GetSelectionCharFormat(cfSel);
					CImageDataObject::InsertBitmap(GetOleInterface(), foundEmoticon->getEmoticonBmp(cfSel.crBackColor));

					++smiles;
					++lSelBegin;

					//fix the positions for links after this emoticon....
					for (auto& l : links | reversed) {
						if (l.first.cpMin > lSelBegin) {
							l.first.cpMin -= foundEmoticon->getEmoticonText().size() - 1;
							l.first.cpMax -= foundEmoticon->getEmoticonText().size() - 1;
						} else {
							break;
						}
					}
				}

				lastReplace = curReplace + foundEmoticon->getEmoticonText().size();
			} else break;
		}
	}

}

bool RichTextBox::HitNick(const POINT& p, tstring& sNick, int& iBegin, int& iEnd) {
	if(client == nullptr) return false;
	
	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	long lSelBegin = 0, lSelEnd = 0;
	if(len < 3)
		return false;

	// Metoda FindWordBreak nestaci, protoze v nicku mohou byt znaky povazovane za konec slova
	int iFindBegin = LineIndex(line), iEnd1 = LineIndex(line) + LineLength(iCharPos);

	for(lSelBegin = iCharPos; lSelBegin >= iFindBegin; lSelBegin--) {
		if(FindWordBreak(WB_ISDELIMITER, lSelBegin))
			break;
	}
	lSelBegin++;
	for(lSelEnd = iCharPos; lSelEnd < iEnd1; lSelEnd++) {
		if(FindWordBreak(WB_ISDELIMITER, lSelEnd))
			break;
	}

	len = lSelEnd - lSelBegin;
	if(len <= 0)
		return false;

	tstring sText;
	sText.resize(len);

	GetTextRange(lSelBegin, lSelEnd, &sText[0]);

	size_t iLeft = 0, iRight = 0, iCRLF = sText.size(), iPos = sText.find(_T('<'));
	if(iPos != tstring::npos) {
		iLeft = iPos + 1;
		iPos = sText.find(_T('>'), iLeft);
		if(iPos == tstring::npos) 
			return false;

		iRight = iPos - 1;
		iCRLF = iRight - iLeft + 1;
	} else {
		iLeft = 0;
	}

	tstring sN = sText.substr(iLeft, iCRLF);
	if(sN.empty())
		return false;

	if(client && client->findUser(Text::fromT(sN))) {
		sNick = sN;
		iBegin = lSelBegin + iLeft;
		iEnd = lSelBegin + iLeft + iCRLF;
		return true;
	}
    
	// Jeste pokus odmazat eventualni koncovou ':' nebo '>' 
	// Nebo pro obecnost posledni znak 
	// A taky prvni znak 
	// A pak prvni i posledni :-)
	if(iCRLF > 1) {
		sN = sText.substr(iLeft, iCRLF - 1);
		if(client->findUser(Text::fromT(sN)) != NULL) {
			sNick = sN;
   			iBegin = lSelBegin + iLeft;
   			iEnd = lSelBegin + iLeft + iCRLF - 1;
			return true;
		}

		sN = sText.substr(iLeft + 1, iCRLF - 1);
		if(client->findUser(Text::fromT(sN)) != NULL) {
        	sNick = sN;
			iBegin = lSelBegin + iLeft + 1;
			iEnd = lSelBegin + iLeft + iCRLF;
			return true;
		}

		sN = sText.substr(iLeft + 1, iCRLF - 2);
		if(client->findUser(Text::fromT(sN)) != NULL) {
			sNick = sN;
   			iBegin = lSelBegin + iLeft + 1;
			iEnd = lSelBegin + iLeft + iCRLF - 1;
			return true;
		}
	}	
	return false;
}

bool RichTextBox::HitIP(const POINT& p, tstring& sIP, int& iBegin, int& iEnd) {
	int iCharPos = CharFromPos(p), len = LineLength(iCharPos) + 1;
	if(len < 3)
		return false;

	DWORD lPosBegin = FindWordBreak(WB_LEFT, iCharPos);
	DWORD lPosEnd = FindWordBreak(WB_RIGHTBREAK, iCharPos);
	len = lPosEnd - lPosBegin;

	tstring sText;
	sText.resize(len);
	GetTextRange(lPosBegin, lPosEnd, &sText[0]);

	for(int i = 0; i < len; i++) {
		if(!((sText[i] == 0) || (sText[i] == '.') || ((sText[i] >= '0') && (sText[i] <= '9')))) {
			return false;
		}
	}

	sText += _T('.');
	size_t iFindBegin = 0, iPos = tstring::npos, iEnd2 = 0;

	for(int i = 0; i < 4; ++i) {
		iPos = sText.find(_T('.'), iFindBegin);
		if(iPos == tstring::npos) {
			return false;
		}
		iEnd2 = atoi(Text::fromT(sText.substr(iFindBegin)).c_str());
		if((iEnd2 < 0) || (iEnd2 > 255)) {
			return false;
		}
		iFindBegin = iPos + 1;
	}

	sIP = sText.substr(0, iPos);
	iBegin = lPosBegin;
	iEnd = lPosEnd;

	return true;
}

tstring RichTextBox::LineFromPos(const POINT& p) const {
	int iCharPos = CharFromPos(p);
	int len = LineLength(iCharPos);

	if(len < 3) {
		return Util::emptyStringT;
	}

	tstring tmp;
	tmp.resize(len);

	GetLine(LineFromChar(iCharPos), &tmp[0], len);

	return tmp;
}

void RichTextBox::formatLink(DupeType aDupeType, bool aIsRelease) {
	if (SETTING(DUPES_IN_CHAT) && AirUtil::isShareDupe(aDupeType)) {
		SetSelectionCharFormat(WinUtil::m_TextStyleDupe);
	} else if (SETTING(DUPES_IN_CHAT) && AirUtil::isQueueDupe(aDupeType)) {
		SetSelectionCharFormat(WinUtil::m_TextStyleQueue);
	} else if (SETTING(DUPES_IN_CHAT) && AirUtil::isFinishedDupe(aDupeType)) {
		CHARFORMAT2 newFormat = WinUtil::m_TextStyleQueue;
		newFormat.crTextColor = WinUtil::getDupeColors(aDupeType).first;
		SetSelectionCharFormat(newFormat);
	} else if (!aIsRelease || (SETTING(FORMAT_RELEASE)) && aDupeType == DUPE_NONE) {
		SetSelectionCharFormat(WinUtil::m_TextStyleURL);
	}
}

DupeType RichTextBox::updateDupeType(ChatLink* aChatLink) {
	auto oldDT = aChatLink->getDupe();
	if (oldDT != aChatLink->updateDupeType(pmUser)) {
		formatLink(aChatLink->getDupe(), isRelease);
	}
	return aChatLink->getDupe();
}

void RichTextBox::clearSelInfo() {
	selectedLine.clear();
	selectedUser.clear();
	selectedIP.clear();
	dupeType = DUPE_NONE, isMagnet = false, isTTH = false, isRelease = false, isPath = false;
	selectedWord.clear();
}

void RichTextBox::updateSelectedText(POINT pt, bool selectLink) {
	selectedLine = LineFromPos(pt);

	auto p = getLink(pt);
	if (p != links.crend()) {
		auto cl = p->second;

		selectedWord = Text::toT(cl->url);
		if (selectLink)
			SetSel(p->first.cpMin, p->first.cpMax);

		isMagnet = cl->getType() == ChatLink::TYPE_MAGNET;
		isRelease = cl->getType() == ChatLink::TYPE_RELEASE;
		isPath = cl->getType() == ChatLink::TYPE_PATH;

		if (isMagnet || isRelease) {
			//check if the dupe status has changed
			dupeType = updateDupeType(cl);
		}
	} else {
		CHARRANGE cr;
		GetSel(cr);
		if (cr.cpMax != cr.cpMin) {
			//Get the text
			TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
			GetSelText(buf);
			selectedWord = buf;
			delete [] buf;

			//Replace shortened links in the range.
			for (const auto& l : links | reversed) {
				if (l.first.cpMin >= cr.cpMin && l.first.cpMax <= cr.cpMax) {
					selectedWord.replace(l.first.cpMin - cr.cpMin, l.second->getDisplayText().length(), Text::toT(l.second->url));
				}
			}

			//Change line breaks
			selectedWord = Util::replaceT(selectedWord, _T("\r"), _T("\r\n"));
		}
		else {
			selectedWord = WordFromPos(pt);
			if (selectedWord.length() == 39) {
				isTTH = true;
			}
		}
	}
}

void RichTextBox::OnRButtonDown(POINT pt) {
	clearSelInfo();
	updateAuthor();

	long lSelBegin = 0, lSelEnd = 0;
	GetSel(lSelBegin, lSelEnd);
	int iCharPos = CharFromPos(pt), iBegin = 0, iEnd = 0;
	
	//Check if we clicked inside an already selected text
	bool clickedInSel = ((lSelEnd > lSelBegin) && (iCharPos >= lSelBegin) && (iCharPos <= lSelEnd));

	// highlight IP or nick when clicking on it, unless we have right clicked inside our own selection 
	if((HitIP(pt, selectedIP, iBegin, iEnd) || HitNick(pt, selectedUser, iBegin, iEnd)) && !clickedInSel) {
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	}

	updateSelectedText(pt, true);
}

bool RichTextBox::updateAuthor() {
	size_t pos = selectedLine.find(_T(" <"));
	if (pos != tstring::npos) {
		tstring::size_type iAuthorLen = selectedLine.find(_T('>'), 1);
		if(iAuthorLen != tstring::npos && iAuthorLen > pos) {
			tstring nick(selectedLine.c_str() + pos + 2);
			nick.erase(iAuthorLen - pos - 2);
			author = nick;
			return true;
		}
	}
	return false;
}

LRESULT RichTextBox::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	m_bPopupMenu = false;
	bHandled = FALSE;
	return 0;
}

LRESULT RichTextBox::onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    if(m_bPopupMenu)
    {
        SetCursor(arrowCursor) ;
		return 1;
    }

	if (showHandCursor) {
		SetCursor(handCursor);
		return 1;
	}
    bHandled = FALSE;
	return 0;
}

const UserPtr& RichTextBox::getUser() const {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if (ou) {
		return ou->getUser();
	}
	
	return pmUser;
}

const string& RichTextBox::getHubUrl() const {
	return client ? client->getHubUrl() : Util::emptyString;
}

LRESULT RichTextBox::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	if (pt.x == -1 && pt.y == -1) {
		CRect erc;
		GetRect(&erc);
		pt.x = erc.Width() / 2;
		pt.y = erc.Height() / 2;
		ClientToScreen(&pt);
	}

	//check if we are clicking on a scrollbar
	if (WinUtil::isOnScrollbar(m_hWnd, pt)) {
		//let the system handle those
		bHandled = FALSE;
		return 0;
	}

	POINT ptCl = pt;
	ScreenToClient(&ptCl); 
	OnRButtonDown(ptCl);

	ShellMenu menu;
	menu.CreatePopupMenu();

	if (copyMenu.m_hMenu != NULL) {
		// delete copy menu if it exists
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
	}

	if(selectedUser.empty()) {

		if(!selectedIP.empty()) {
			menu.InsertSeparatorFirst(selectedIP);
			menu.AppendMenu(MF_STRING, IDC_WHOIS_IP, (TSTRING(WHO_IS) + _T(" ") + selectedIP).c_str() );
			if (client)
				prepareMenu(menu, ::UserCommand::CONTEXT_USER, client->getHubUrl());

			if (client && client->isOp()) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + selectedIP).c_str());
				menu.SetMenuDefaultItem(IDC_BAN_IP);
				menu.AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + selectedIP).c_str());
				menu.AppendMenu(MF_SEPARATOR);
			}
		} else if (isRelease) {
			menu.InsertSeparatorFirst(CTSTRING(RELEASE));
		}  else if (!selectedWord.empty() && AirUtil::isHubLink(Text::fromT(selectedWord))) {
			menu.InsertSeparatorFirst(CTSTRING(LINK));
			menu.appendItem(TSTRING(CONNECT), [this] { handleOpenLink(); });
			menu.appendSeparator();
		} else if (!isPath) {
			menu.InsertSeparatorFirst(CTSTRING(TEXT));
		}

		menu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		menu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if (!isPath) {
			menu.appendSeparator();
			menu.appendItem(TSTRING(SEARCH), [this] { handleSearch(); });
			if (isTTH || isMagnet) {
				menu.appendItem(TSTRING(SEARCH_TTH), [this] { handleSearchTTH(); });
			}

			if (isMagnet || isRelease) {
				if (dupeType > 0) {
					menu.appendSeparator();
					menu.appendItem(TSTRING(OPEN_FOLDER), [this] { handleOpenFolder(); });
				}

				menu.appendSeparator();
				if (isMagnet) {
					auto isMyLink = client && Text::toT(client->getMyNick()) == author;
					Magnet m = Magnet(Text::fromT(selectedWord));
					if (ShareManager::getInstance()->isTempShared(getTempShareKey(), m.getTTH())) {
						/* show an option to remove the item */
						menu.appendItem(TSTRING(STOP_SHARING), [this] { handleRemoveTemp(); });
					} else if (!isMyLink) {
						appendDownloadMenu(menu, DownloadBaseHandler::TYPE_PRIMARY, false, m.getTTH(), boost::none);
					}

					if ((!author.empty() && !isMyLink) || AirUtil::allowOpenDupe(dupeType))
						menu.appendItem(TSTRING(OPEN), [this] { handleOpenFile(); });
				} else if (isRelease) {
					//autosearch menus
					appendDownloadMenu(menu, DownloadBaseHandler::TYPE_SECONDARY, true, boost::none, Text::fromT(selectedWord) + PATH_SEPARATOR, false);
				}
			}
		}

		WinUtil::appendSearchMenu(menu, Text::fromT(selectedWord), false);
	} else {
		bool isMe = (selectedUser == Text::toT(client->getMyNick()));

		// click on nick
		copyMenu.CreatePopupMenu();
		copyMenu.InsertSeparatorFirst(TSTRING(COPY));

		for(int j=0; j < OnlineUser::COLUMN_LAST; j++) {
			copyMenu.AppendMenu(MF_STRING, IDC_COPY + j, CTSTRING_I(HubFrame::columnNames[j]));
		}

		menu.InsertSeparatorFirst(selectedUser);

		if(SETTING(LOG_PRIVATE_CHAT)) {
			menu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
			menu.AppendMenu(MF_STRING, IDC_USER_HISTORY,  CTSTRING(VIEW_HISTORY));
			menu.AppendMenu(MF_SEPARATOR);
		}		

		if (!pmUser) {
			menu.AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
			menu.AppendMenu(MF_SEPARATOR);
		}
		
		if(!isMe) {
			if (!pmUser || pmUser->isSet(User::BOT)) {
				menu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
				//menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
				//menu.AppendMenu(MF_SEPARATOR);
			}
			
			const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
			appendUserItems(menu, true);

			if (client->isOp() || !ou->getIdentity().isOp()) {
				if(!ou->getUser()->isIgnored()) {
					menu.appendItem(TSTRING(IGNORE_USER), [this] { handleIgnore(); });
				} else { 
					menu.appendItem(TSTRING(UNIGNORE_USER), [this] { handleUnignore(); });
				}
				menu.appendSeparator();
			}
		}
		
		menu.AppendMenu(MF_POPUP, (HMENU)copyMenu, CTSTRING(COPY));
		
		// add user commands
		prepareMenu(menu, ::UserCommand::CONTEXT_USER, client->getHubUrl());

		/*// default doubleclick action
		switch(SETTING(CHAT_DBLCLICK)) {
        case 0:
			menu.SetMenuDefaultItem(IDC_SELECT_USER);
			break;
        case 1:
			menu.SetMenuDefaultItem(IDC_PUBLIC_MESSAGE);
			break;
        case 2:
			menu.SetMenuDefaultItem(IDC_PRIVATEMESSAGE);
			break;
        case 3:
			menu.SetMenuDefaultItem(IDC_GETLIST);
			break;
        case 4:
			menu.SetMenuDefaultItem(IDC_MATCH_QUEUE);
			break;
        case 6:
			menu.SetMenuDefaultItem(IDC_ADD_TO_FAVORITES);
			break;
		} */ 
			//Bold the Best solution for getting user users list instead.
			if(AirUtil::isAdcHub(client->getHubUrl())) {
				menu.SetMenuDefaultItem(IDC_BROWSELIST);
			} else {
				menu.SetMenuDefaultItem(IDC_GETLIST);
			}
	}

	menu.appendSeparator();
	menu.appendItem(TSTRING(FIND_TEXT), [this] { findText(); });
	menu.appendItem(TSTRING(SELECT_ALL), [this] { handleEditSelectAll(); });
	if (allowClear)
		menu.appendItem(TSTRING(CLEAR_CHAT), [this] { handleEditClearAll(); });

	if (isPath) {
		menu.InsertSeparatorFirst(CTSTRING(PATH));
		menu.appendItem(TSTRING(SEARCH_DIRECTORY), [this] { handleSearchDir(); });
		menu.appendItem(TSTRING(ADD_AUTO_SEARCH_DIR), [this] { handleAddAutoSearchDir(); });

		auto path = Text::fromT(selectedWord);
		if (path.back() != PATH_SEPARATOR) {
			menu.appendSeparator();
			menu.appendItem(TSTRING(SEARCH_FILENAME), [this] { handleSearch(); });
			if (Util::fileExists(path)) {
				menu.appendItem(TSTRING(DELETE_FILE), [this] { handleDeleteFile(); });
			} else {
				menu.appendItem(TSTRING(ADD_AUTO_SEARCH_FILE), [this] { handleAddAutoSearchFile(); });
			}
		}

		menu.appendShellMenu({ path });
	}
	
	//flag to indicate pop up menu.
    m_bPopupMenu = true;
	menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);

	return 0;
}

void RichTextBox::handleSearchDir() {
	WinUtil::searchAny(Text::toT(AirUtil::getReleaseDirLocal(Text::fromT(selectedWord), true)));
}

void RichTextBox::handleDeleteFile() {
	string path = Text::fromT(selectedWord);
	string msg = STRING_F(DELETE_FILE_CONFIRM, path);
	if(WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_FILE_DELETIONS, Text::toT(msg).c_str())) {
		MainFrame::getMainFrame()->addThreadedTask([=] { File::deleteFileEx(path, 3); });
	}

	SetSelNone();
}

void RichTextBox::handleAddAutoSearchFile() {
	string targetPath;
	if (selectedWord.find(PATH_SEPARATOR) != tstring::npos)
		targetPath = Util::getFilePath(Text::fromT(selectedWord));
	auto fileName = Util::getFileName(Text::fromT(selectedWord));

	AutoSearchManager::getInstance()->addAutoSearch(fileName, targetPath, TargetUtil::TARGET_PATH, false, AutoSearch::CHAT_DOWNLOAD, true, 60);

	SetSelNone();
}

void RichTextBox::handleAddAutoSearchDir() {
	string targetPath = Util::getParentDir(Text::fromT(selectedWord), PATH_SEPARATOR, true);
	string dirName = Util::getLastDir(selectedWord[selectedWord.length()-1] != PATH_SEPARATOR ? Util::getFilePath(Text::fromT(selectedWord)) : Text::fromT(selectedWord));

	AutoSearchManager::getInstance()->addAutoSearch(dirName, targetPath, TargetUtil::TARGET_PATH, true, AutoSearch::CHAT_DOWNLOAD, true, 60);

	SetSelNone();
}

LRESULT RichTextBox::onFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	LPFINDREPLACE fr = reinterpret_cast<LPFINDREPLACE>(lParam);

	if(fr->Flags & FR_DIALOGTERM){
		WinUtil::findDialog = nullptr;
	} else if(fr->Flags & FR_FINDNEXT){
		//prepare the flags used to search
		int flags = 0;
		if(fr->Flags & FR_WHOLEWORD)
			flags = FR_WHOLEWORD;
		if(fr->Flags & FR_MATCHCASE)
			flags |= FR_MATCHCASE;
		if(fr->Flags & FR_DOWN)
			flags |= FR_DOWN;

		//initiate the structure, cpMax -1 means the whole document is searched
		FINDTEXTEX ft;
		ft.lpstrText = fr->lpstrFindWhat;

		CHARRANGE cr;
		GetSel(cr);
		ft.chrg.cpMin = fr->Flags & FR_DOWN ? cr.cpMax : cr.cpMin;
		ft.chrg.cpMax = -1;

		//if we find the end of the document, notify the user and return
		int result = (int)SendMessage(EM_FINDTEXTEX, (WPARAM)flags, (LPARAM)&ft);
		if(-1 == result){
			::MessageBox(WinUtil::findDialog, CTSTRING(NO_RESULTS_FOUND), Text::toT(shortVersionString).c_str(), MB_OK | MB_ICONINFORMATION);
			return 0;
		}

		//select the result and scroll it into view
		//SetFocus();
		SetSel(result, result + _tcslen(ft.lpstrText));
		ScrollCaret();
	}

	return 0;
}

void RichTextBox::findText(tstring& /*aTxt*/) {
	LPFINDREPLACE fr = new FINDREPLACE;
	ZeroMemory(fr, sizeof(FINDREPLACE));
	fr->lStructSize = sizeof(FINDREPLACE);
	fr->hwndOwner = m_hWnd;
	fr->hInstance = NULL;
	fr->Flags = FR_DOWN;
	
	fr->lpstrFindWhat = findBuffer;
	fr->wFindWhatLen = findBufferSize;

	if(WinUtil::findDialog == NULL)
		WinUtil::findDialog = ::FindText(fr);
}

LRESULT RichTextBox::onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (((GetKeyState(VkKeyScan('F') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0) || wParam == VK_F3){
		findText();
		return 1;
	}

	if ((GetKeyState(VkKeyScan('C') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0) {
		POINT pt = { -1, -1 };
		updateSelectedText(pt, false);
		WinUtil::setClipboard(selectedWord);
		clearSelInfo();
		return 1;
	}

	bHandled = FALSE;
	return 0;
}

void RichTextBox::handleOpenFile() {
	Magnet m = Magnet(Text::fromT(selectedWord));
	if (m.hash.empty())
		return;

	auto u = getMagnetSource();
	MainFrame::getMainFrame()->addThreadedTask([=] {
		if (dupeType > 0) {
			auto p = AirUtil::getFileDupePaths(dupeType, m.getTTH());
			if (!p.empty())
				WinUtil::openFile(Text::toT(p.front()));
		} else {
			WinUtil::openFile(m.fname, m.fsize, m.getTTH(), u, false);
		}
	});
}

HintedUser RichTextBox::getMagnetSource() {
	UserPtr u = nullptr;
	if (pmUser && !pmUser->isSet(User::BOT)) {
		u = pmUser;
	} else if (client && !author.empty()) {
		OnlineUserPtr ou = client->findUser(Text::fromT(author));
		if (ou && !ou->getUser()->isSet(User::BOT)) {
			u = ou->getUser();
		}
	}

	if (u && client)
		return HintedUser(u, client->getHubUrl());

	return HintedUser();
}

string RichTextBox::getTempShareKey() const {
	return (pmUser && !pmUser->isSet(User::BOT) && !pmUser->isSet(User::NMDC)) ? pmUser->getCID().toBase32() : Util::emptyString;
}

void RichTextBox::handleRemoveTemp() {
	string link = Text::fromT(selectedWord);
	Magnet m = Magnet(link);
	ShareManager::getInstance()->removeTempShare(getTempShareKey(), m.getTTH());
	for (auto& cl: links | map_values) {
		if (cl->getType() == ChatLink::TYPE_MAGNET && cl->url == link) {
			updateDupeType(cl);
		}
	}
}

void RichTextBox::handleOpenFolder() {
	StringList paths;
	try{
		if (isRelease) {
			paths = AirUtil::getDirDupePaths(dupeType, Text::fromT(selectedWord));
		} else if (isPath) {
			paths.push_back(Text::fromT(Util::getFilePath(selectedWord)));
		} else {
			Magnet m = Magnet(Text::fromT(selectedWord));
			if (m.hash.empty())
				return;

			paths = AirUtil::getFileDupePaths(dupeType, m.getTTH());

		}
	} catch(...) {}

	if (!paths.empty())
		WinUtil::openFolder(Text::toT(paths.front()));
}

void RichTextBox::handleDownload(const string& aTarget, QueueItemBase::Priority p, bool aIsRelease, TargetUtil::TargetType aTargetType, bool /*isSizeUnknown*/) {
	if (!aIsRelease) {
		auto u = move(getMagnetSource());
		Magnet m = Magnet(Text::fromT(selectedWord));
		if (pmUser && ShareManager::getInstance()->isNmdcDirShared(aTarget, m.fsize) > 0 &&
			!WinUtil::showQuestionBox(TSTRING_F(PM_MAGNET_SHARED_WARNING, Text::toT(Util::getFilePath(aTarget))), MB_ICONQUESTION)) {
				return;
		}

		WinUtil::addFileDownload(aTarget + (aTarget[aTarget.length() - 1] != PATH_SEPARATOR ? Util::emptyString : m.fname), m.fsize, m.getTTH(), u, 0, pmUser ? QueueItem::FLAG_PRIVATE : 0, p);
	} else {
		AutoSearchManager::getInstance()->addAutoSearch(Text::fromT(selectedWord), aTarget, aTargetType, true, AutoSearch::CHAT_DOWNLOAD);
	}
}

bool RichTextBox::showDirDialog(string& fileName) {
	if (isMagnet) {
		Magnet m = Magnet(Text::fromT(selectedWord));
		fileName = m.fname;
		return false;
	}
	return true;
}

int64_t RichTextBox::getDownloadSize(bool /*isWhole*/) {
	if (isMagnet) {
		Magnet m = Magnet(Text::fromT(selectedWord));
		return m.fsize;
	} else {
		return 0;
	}
}

LRESULT RichTextBox::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam != SIZE_MINIMIZED && HIWORD(lParam) > 0 && autoScrollToEnd) {
		scrollToEnd();
	}

	bHandled = FALSE;
	return 0;
}

LRESULT RichTextBox::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	selectedLine.clear();
	selectedIP.clear();
	selectedUser.clear();
	selectedWord.clear();

	bHandled = FALSE;
	return 0;
}


LRESULT RichTextBox::onMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	

	bHandled=FALSE;
	if (lastTick+20 > GET_TICK())
		return FALSE;
	lastTick=GET_TICK();

	if(wParam != 0) { //dont update cursor and check for links when marking something.
		if (showHandCursor)
			SetCursor(arrowCursor);
		showHandCursor=false;
		return TRUE;
	}

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	if (isLink(pt)) {
		if (!showHandCursor) {
			showHandCursor=true;
			SetCursor(handCursor);
		}
		return TRUE;
	} else if (showHandCursor) {
		showHandCursor=false;
		SetCursor(arrowCursor);
		return TRUE;
	}

	return FALSE;
}

bool RichTextBox::isLink(POINT& pt) {
	return getLink(pt) != links.crend();
}

RichTextBox::LinkList::const_reverse_iterator RichTextBox::getLink(POINT& pt) {
	int iCharPos = CharFromPos(pt), /*line = LineFromChar(iCharPos),*/ len = LineLength(iCharPos) + 1;
	if (len < 3) {
		return links.crend();
	}

	// check if we are beyond the end of the line
	auto iNextWordPos = FindWordBreak(WB_MOVEWORDRIGHT, iCharPos);
	if (LineFromChar(iCharPos) != LineFromChar(iNextWordPos))
		return links.crend();

	return find_if(links.crbegin(), links.crend(), [iCharPos](const ChatLinkPair& l) { return iCharPos >= l.first.cpMin && iCharPos <= l.first.cpMax; });
}

LRESULT RichTextBox::onLeftButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	CHARRANGE cr;
	GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		bHandled = FALSE;
		return 0;
	}

	bHandled = onClientEnLink(uMsg, wParam, lParam, bHandled);
	return bHandled = TRUE ? 0: 1;
}

void RichTextBox::handleOpenLink() {
	string link = Text::fromT(selectedWord);
	for(const auto& l: links | reversed) {
		if(compare(l.second->url, link) == 0) {
			openLink(l.second);
			return;
		}
	}
}

bool RichTextBox::onClientEnLink(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	auto p = getLink(pt);
	if (p == links.crend())
		return false;

	auto cl = p->second;
	selectedWord = Text::toT(cl->url); // for magnets
	updateSelectedText(pt, false);
	updateAuthor();

	openLink(cl);

	return 1;
}

void RichTextBox::openLink(const ChatLink* cl) {
	if (cl->getType() == ChatLink::TYPE_MAGNET) {
		auto u = getMagnetSource();
		WinUtil::parseMagnetUri(Text::toT(cl->url), u, this);
	} else {
		//the url regex also detects web links without any protocol part
		auto link = cl->url;
		if (cl->getType() == ChatLink::TYPE_URL && link.find(':') == string::npos)
			link = "http://" + link;

		WinUtil::openLink(Text::toT(link));
	}
}

LRESULT RichTextBox::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::setClipboard(selectedWord);
	return 0;
}

void RichTextBox::handleEditSelectAll() {
	SetSelAll();
}

void RichTextBox::handleEditClearAll() {
	links.clear();
	SetWindowText(_T(""));
}

LRESULT RichTextBox::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedLine.empty()) {
		WinUtil::setClipboard(selectedLine);
	}
	return 0;
}

LRESULT RichTextBox::onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
		tstring s = _T("!banip ") + selectedIP;

		string error;
		client->hubMessage(Text::fromT(s), error);
	}
	SetSelNone();
	return 0;
}

LRESULT RichTextBox::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
		tstring s = _T("!unban ") + selectedIP;

		string error;
		client->hubMessage(Text::fromT(s), error);
	}
	SetSelNone();
	return 0;
}


LRESULT RichTextBox::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
 		WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + selectedIP);
 	}
	SetSelNone();
	return 0;
}

LRESULT RichTextBox::onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		auto file = ou->getLogPath();
		if(Util::fileExists(file)) {
			WinUtil::viewLog(file, wID == IDC_USER_HISTORY);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}
	}

	return 0;
}

void RichTextBox::handleIgnore() {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		MessageManager::getInstance()->storeIgnore(ou->getUser());
	}
}

void RichTextBox::handleUnignore(){
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		MessageManager::getInstance()->removeIgnore(ou->getUser());
	}
}

LRESULT RichTextBox::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	
	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		sCopy = ou->getText(static_cast<uint8_t>(wID - IDC_COPY), true);
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

void RichTextBox::runUserCommand(UserCommand& uc) {
	ParamMap ucParams;

	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou != NULL) {
		auto tmp = ucParams;
		ou->getIdentity().getParams(tmp, "user", true);
		client->sendUserCmd(uc, tmp);
	}
}

void RichTextBox::scrollToEnd() {
	SCROLLINFO si = { 0 };
	POINT pt = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);
	GetScrollPos(&pt);

	// this must be called twice to work properly :(
	PostMessage(EM_SCROLL, SB_BOTTOM, 0);
	PostMessage(EM_SCROLL, SB_BOTTOM, 0);

	SetScrollPos(&pt);
}

size_t RichTextBox::FullTextMatch(ColorSettings* cs, CHARFORMAT2 &hlcf, const tstring &line, size_t pos, long &lineIndex) {
	//this shit needs a total cleanup, we cant highlight authors or timestamps this way and we dont need to.
	
	size_t index = tstring::npos;
	tstring searchString;

	if( cs->getMyNick() ) {
		tstring::size_type p = cs->getMatch().find(_T("$MyNI$"));
		if(p != tstring::npos) {
			searchString = cs->getMatch();
			searchString = searchString.replace(p, 8, Text::toT(client->getMyNick()));
		} 
	} else {
		searchString = cs->getMatch();
	}
	
	//we don't have any nick to search for
	//happens in pm's have to find a solution for this
	if(searchString.empty())
		return tstring::npos;

	
	//do we want to highlight the timestamps?
	if( cs->getTimestamps() ) {
		if( line[0] != _T('[') )
			return tstring::npos;
		index = 0;
	} else if( cs->getUsers() ) {
		if(timeStamps) {
			index = line.find(_T("] <"));
			// /me might cause this to happen
			if(index == tstring::npos)
				return tstring::npos;
			//compensate for "] "
			index += 2;
		} else if( line[0] == _T('<')) {
			index = 0;
		}
	}else{
		if( cs->getCaseSensitive() ) {
			index = line.find(searchString, pos);
		}else {
			index = Util::findSubString(line, searchString, pos);	
			//index = Text::toLower(line).find(Text::toLower(searchString), pos);
		}
	}
	//return if no matches where found
	if( index == tstring::npos )
		return tstring::npos;

	pos = index + searchString.length();
	
	//found the string, now make sure it matches
	//the way the user specified
	int length = 0;
		
	if( !cs->getUsers() && !cs->getTimestamps() ) {
		length = searchString.length();
		int p = 0;
			
		switch(cs->getMatchType()){
			case 0: //Begins
				p = index-1;
                if(line[p] != _T(' ') && line[p] != _T('\r') &&	line[p] != _T('\t') )
					return tstring::npos;
				break;
			case 1: //Contains
				break;
			case 2: // Ends
				p = index+length;
				if(line[p] != _T(' ') && line[p] != _T('\r') &&	line[p] != _T('\t') )
					return tstring::npos;
				break;
			case 3: // Equals
				if( !( (index == 0 || line[index-1] == _T(' ') || line[index-1] == _T('\t') || line[index-1] == _T('\r')) && 
					(line[index+length] == _T(' ') || line[index+length] == _T('\r') || 
					line[index+length] == _T('\t') || index+length == line.size()) ) )
					return tstring::npos;
				break;
		}
	}

	long begin, end;

	begin = lineIndex;
	
	if( cs->getTimestamps() ) {
		tstring::size_type endPos = line.find(_T("]"));
		if(endPos == tstring::npos )
			return tstring::npos;  //hmm no ]? this can't be right, return
		
		begin += index;
		end = begin + endPos +1;
	} else if( cs->getUsers() ) {
		
		end = begin + line.find(_T(">")) +1;
		begin += index;
	} else if( cs->getWholeLine() ) {
		end = begin + line.length() -1;
	} else if( cs->getWholeWord() ) {
		int tmp;

		tmp = line.find_last_of(_T(" \t\r"), index);
		if(tmp != tstring::npos )
			begin += tmp+1;
		
		tmp = line.find_first_of(_T(" \t\r"), index);
		if(tmp != tstring::npos )
			end = lineIndex + tmp;
		else
			end = lineIndex + line.length();
	} else {
		begin += index;
		end = begin + searchString.length();
	}

	SetSel(begin, end);
		
	SetSelectionCharFormat(hlcf);


	CheckAction(cs, line);
	
	if( cs->getTimestamps() || cs->getUsers() )
		return tstring::npos;
	
	return pos;
}
size_t RichTextBox::RegExpMatch(ColorSettings* cs, CHARFORMAT2 &hlcf, const tstring &line, long &lineIndex) {
	//TODO: Clean it up a bit
	
	long begin, end;	
	bool found = false;

	//this is not a valid regexp
	if(cs->getMatch().length() < 5)
		return tstring::npos;

	try {

		boost::wsregex_iterator iter(line.begin(), line.end(), cs->regexp);
		boost::wsregex_iterator enditer;
		for(; iter != enditer; ++iter) {
			begin = lineIndex + iter->position();
			end = lineIndex + iter->position() + iter->length();

			if( cs->getWholeLine() ) {
				end = begin + line.length();
			} 
			SetSel(begin, end);
			SetSelectionCharFormat(hlcf);
		
			found = true;
		}
	} catch(...) {
	}

	if(found)
		CheckAction(cs, line);
	
	return tstring::npos;

}

void RichTextBox::CheckAction(ColorSettings* cs, const tstring& line) {
	if(cs->getPopup() && !matchedPopup) {
		matchedPopup = true;
		tstring popupTitle;
		popupTitle = _T("Highlight");
		WinUtil::showPopup(line.c_str(), popupTitle.c_str());
	}


	if(cs->getTab())
		matchedTab = true;
	
	//Todo maybe
//	if(cs->getLog() && !logged && !skipLog){
//		logged = true;
//		AddLogLine(line);
//	}

	if(cs->getPlaySound() && !matchedSound ){
		matchedSound = true;
		WinUtil::playSound(Text::toT(cs->getSoundFile()));
	}

	if(cs->getFlashWindow())
		WinUtil::FlashWindow();
}

tstring RichTextBox::WordFromPos(const POINT& p) {

	int iCharPos = CharFromPos(p), /*line = LineFromChar(iCharPos),*/ len = LineLength(iCharPos) + 1;
	if(len < 3)
		return Util::emptyStringT;

	POINT p_ichar = PosFromChar(iCharPos);
	
	//check the mouse positions again, to avoid getting the word even if we are past the end of text. 
	//better way to do this?
	if(p.x > (p_ichar.x + 3)) { //+3 is close enough, dont want to be too strict about it?
		return Util::emptyStringT;
	}

	if(p.y > (p_ichar.y +  (t_height*1.5))) { //times 1.5 so dont need to be totally exact
		return Util::emptyStringT;
	}

	int begin =  0;
	int end  = 0;
	
	FINDTEXT findt;
	findt.chrg.cpMin = iCharPos;
	findt.chrg.cpMax = -1;
	findt.lpstrText = _T("\r");

	long l_Start = SendMessage(EM_FINDTEXT, 0, (LPARAM)&findt) + 1;
	long l_End = SendMessage(EM_FINDTEXT, FR_DOWN, (LPARAM)&findt);

	if(l_Start < 0)
		l_Start = 0;

	if(l_End == -1)
		l_End =  GetTextLengthEx(GTL_NUMCHARS);
		
	//long l_Start = LineIndex(line);
	//long l_End = LineIndex(line) + LineLength(iCharPos);
	
	
	tstring Line;
	len = l_End - l_Start;
	if(len < 3)
		return Util::emptyStringT;

	Line.resize(len);
	GetTextRange(l_Start, l_End, &Line[0]); //pick the current line from text for starters

	iCharPos = iCharPos - l_Start; //modify the charpos within the range of our new line.

	begin = Line.find_last_of(_T(" \t\r"), iCharPos) + 1;	
	end = Line.find_first_of(_T(" \t\r"), begin);
			if(end == tstring::npos) {
				end = Line.length();
			}
	len = end - begin;
	
	/*a hack, limit to 512, scrolling becomes sad with long words...
	links longer than 512? set �t higher or maybe just limit the cursor detecting?*/
	if((len <= 3) || (len >= 512)) 
		return Util::emptyStringT;

	tstring sText;
	sText = Line.substr(begin, end-begin);
	
	if(!sText.empty()) 
		return sText;
	
	return Util::emptyStringT;
}

void RichTextBox::AppendHTML(const string& aTxt) {
	tstring msg = HtmlToRtf::convert(aTxt, this);
	msg = _T("{\\urtf1\n") + msg + _T("}\n");
	auto txt = escapeUnicode(msg);

	SetSel(-1, -1);

	SETTEXTEX config = { ST_SELECTION, CP_ACP };
	::SendMessage(m_hWnd, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&config), reinterpret_cast<LPARAM>(txt.c_str()));
}

tstring RichTextBox::getLinkText(const ENLINK& link) {
	boost::scoped_array<TCHAR> buf(new TCHAR[link.chrg.cpMax - link.chrg.cpMin + 1]);
	TEXTRANGE text = { link.chrg, buf.get() };
	SendMessage(m_hWnd, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&text));
	return buf.get();
}

LRESULT RichTextBox::handleLink(ENLINK& link) {
	/* the control doesn't handle click events, just "mouse down" & "mouse up". so we have to make
	sure the mouse hasn't moved between "down" & "up". */
	static LPARAM clickPos = 0;

	switch(link.msg) {
	case WM_LBUTTONDOWN:
		{
			clickPos = link.lParam;
			break;
		}

	case WM_LBUTTONUP:
		{
			if(link.lParam != clickPos)
				break;

			WinUtil::openLink(getLinkText(link));
			break;
		}

	/*case WM_SETCURSOR:
		{
			auto pos = ::GetMessagePos();
			if(pos == linkTipPos)
				break;
			linkTipPos = pos;

			currentLink = getLinkText(link);
			linkTip->refresh();
			break;
		}*/
	}
	return 0;
}

void RichTextBox::handleSearch() {
	if (isMagnet) {
		Magnet m = Magnet(Text::fromT(selectedWord));
		WinUtil::searchAny(Text::toT(m.fname));
	} else if (isPath) {
		WinUtil::searchAny(Util::getFileName(selectedWord));
	} else {
		WinUtil::searchAny(selectedWord);
	}
	SetSelNone();
}

void RichTextBox::handleSearchTTH() {
	if (isMagnet) {
		Magnet m = Magnet(Text::fromT(selectedWord));
		WinUtil::searchHash(m.getTTH(), m.fname, m.fsize);
	} else if (!SettingsManager::lanMode) {
		WinUtil::searchHash(TTHValue(Text::fromT(selectedWord)), Util::emptyString, 0);
	}
	SetSelNone();
}
