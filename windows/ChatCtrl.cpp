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
#include "../client/DCPlusPlus.h"
#include "../client/FavoriteManager.h"
#include "../client/UploadManager.h"

#include "ChatCtrl.h"
#include "EmoticonsManager.h"
#include "PrivateFrame.h"
#include "atlstr.h"
#include "MainFrm.h"
#include "IgnoreManager.h"
#include "TextFrame.h"
#include "../client/highlightmanager.h"

EmoticonsManager* emoticonsManager = NULL;

#define MAX_EMOTICONS 48

static const tstring protocols[] = { _T("http://"), _T("https://"), _T("www."), _T("ftp://"), 
	_T("magnet:?"), _T("dchub://"), _T("irc://"), _T("ed2k://"), _T("mms://"), _T("file://"),
	_T("adc://"), _T("adcs://"), _T("nmdcs://"), _T("svn://") };

ChatCtrl::ChatCtrl() : ccw(_T("edit"), this), client(NULL), m_bPopupMenu(false) {
	if(emoticonsManager == NULL) {
		emoticonsManager = new EmoticonsManager();
	}
	
	emoticonsManager->inc();
}

ChatCtrl::~ChatCtrl() {
	magnets.clear();//ApexDC
	if(emoticonsManager->unique()) {
		emoticonsManager->dec();
		emoticonsManager = NULL;
	} else {
		emoticonsManager->dec();
	}
}
/*
void ChatCtrl::AdjustTextSize() {
	if(GetWindowTextLength() > 25000) {
		// We want to limit the buffer to 25000 characters...after that, w95 becomes sad...
		tstring buf;
		SetRedraw(FALSE);
		//SetSel(0, LineIndex(LineFromChar(2000)));
		//ApexDC
		buf.resize(SetSel(0, LineIndex(LineFromChar(2000))));
		buf.resize(GetSelText(&buf[0]));

		for(TStringMap::iterator i = magnets.begin(); i != magnets.end();) {
			tstring::size_type j = 0;
			if((j = buf.find(i->first)) != tstring::npos) {
				if(buf[j + i->first.size()] == _T(' ') || buf[j + i->first.size()] == _T('\n')) {
					magnets.erase(i++);
					continue;
				}
			}
			++i;
		}
		ReplaceSel(_T(""));
		SetRedraw(TRUE);

		scrollToEnd();
	}
}
*/
void ChatCtrl::AppendText(const Identity& i, const tstring& sMyNick, const tstring& sTime, tstring sMsg, CHARFORMAT2& cf, bool bUseEmo/* = true*/) {
	SetRedraw(FALSE);

	SCROLLINFO si = { 0 };
	POINT pt = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);
	GetScrollPos(&pt);

	LONG lSelBegin = 0, lSelEnd = 0, lTextLimit = 0, lNewTextLen = 0;
	LONG lSelBeginSaved, lSelEndSaved;

	// Unify line endings
	tstring::size_type j = 0; 
	while((j = sMsg.find(_T("\r"), j)) != tstring::npos)
		sMsg.erase(j, 1);

	GetSel(lSelBeginSaved, lSelEndSaved);
	lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);

	bool isMyMessage = i.getUser() == ClientManager::getInstance()->getMe();
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
			while(lRemoveChars < lNewTextLen)
				lRemoveChars = LineIndex(LineFromChar(multiplier++ * lTextLimit / 10));
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
	tstring sAuthor = Text::toT(i.getNick());
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
			bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(i.getUser());

			//if(isFavorite || i.isOp()) {
				SetSel(lSelBegin, lSelBegin + iLen + 1);
				SetSelectionCharFormat(cf);
				SetSel(lSelBegin + iLen + 1, lSelEnd);
				if(isFavorite){
					SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
				} else if(i.isOp()) {
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
                    bool isOp = false, isFavorite = false;

                    if(client != NULL) {
						tstring nick(sMsg.c_str() + 1);
						nick.erase(iAuthorLen - 1);
						
						const OnlineUserPtr ou = client->findUser(Text::fromT(nick));
						if(ou != NULL) {
							isFavorite = FavoriteManager::getInstance()->isFavoriteUser(ou->getUser());
							isOp = ou->getIdentity().isOp();
						}
                    }
                    
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
		(lSelBeginSaved == lSelEndSaved || !selectedUser.empty() || !selectedIP.empty() || !selectedURL.empty())))
	{
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
	} else {
		SetScrollPos(&pt);
	}

	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
}

void ChatCtrl::FormatChatLine(const tstring& sMyNick, const tstring& sText, CHARFORMAT2& cf, bool isMyMessage, const tstring& sAuthor, LONG lSelBegin, bool bUseEmo) {
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

	bool found = false;
	while((lMyNickStart = (long)sMsgLower.find(sNick, lSearchFrom)) != tstring::npos) {
		lMyNickEnd = lMyNickStart + (long)sNick.size();
		SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
		SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		lSearchFrom = lMyNickEnd;
		found = true;
	}
	
	if(found) {
		if(	!SETTING(CHATNAMEFILE).empty() && !BOOLSETTING(SOUNDS_DISABLED) &&
			!sAuthor.empty() && (stricmp(sAuthor.c_str(), sNick) != 0)) {
				::PlaySound(Text::toT(SETTING(CHATNAMEFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);	 	
        }
		if(BOOLSETTING(FLASH_WINDOW_ON_MYNICK) 
			&& !sAuthor.empty() && (stricmp(sAuthor.c_str(), sNick) != 0))
					WinUtil::FlashWindow();
	}

	// highlight all occurences of favourite users' nicks
	FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
	for(FavoriteManager::FavoriteMap::const_iterator i = ul.begin(); i != ul.end(); ++i) {
		const FavoriteUser& pUser = i->second;

		lSearchFrom = 0;
		sNick = Text::toT(pUser.getNick());
		std::transform(sNick.begin(), sNick.end(), sNick.begin(), _totlower);

		while((lMyNickStart = (long)sMsgLower.find(sNick, lSearchFrom)) != tstring::npos) {
			lMyNickEnd = lMyNickStart + (long)sNick.size();
			SetSel(lSelBegin + lMyNickStart, lSelBegin + lMyNickEnd);
			SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
			lSearchFrom = lMyNickEnd;
		}
	}
	if (BOOLSETTING(USE_HIGHLIGHT)) {
	


	ColorList *cList = HighlightManager::getInstance()->getList();
	CHARFORMAT2 hlcf;

			tstring msg = sText;


	logged = false;

	//compare the last line against all strings in the vector
	for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
		ColorSettings* cs = &(*i);
		int pos;

		//set start position for find
		if( cs->getIncludeNick() ) {
			pos = 0;
		} else {
			pos = msg.find(_T(">"));
			if(pos == tstring::npos)
				pos = msg.find(_T("**")) + nick.length();
		}

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
	FormatEmoticonsAndLinks(sText, sMsgLower, lSelBegin, bUseEmo);
}

void ChatCtrl::FormatEmoticonsAndLinks(const tstring& sMsg, tstring& sMsgLower, LONG lSelBegin, bool bUseEmo) {
	LONG lSelEnd = lSelBegin + sMsg.size();
	 bool detectMagnet=false;

	// hightlight all URLs and make them clickable
	for(size_t i = 0; i < (sizeof(protocols) / sizeof(protocols[0])); ++i) {
		size_t linkStart = sMsgLower.find(protocols[i]);
		bool isMagnet = (protocols[i] == _T("magnet:?"));
		while(linkStart != tstring::npos) {
			size_t linkEnd = linkStart + protocols[i].size();
			
			try {
				// TODO: complete regexp for URLs
				boost::wregex reg;
				if(isMagnet) // magnet links have totally indifferent structure than classic URL // -/?%&=~#'\\w\\.\\+\\*\\(\\)
					reg.assign(_T("^(\\w)+=[:\\w]+(&(\\w)+=[\\S]*)*[^\\s<>.,;!(){}\"']+"), boost::regex_constants::icase);
				else
					reg.assign(_T("^([@\\w-]+(\\.)*)+(:[\\d]+)?(/[\\S]*)*[^\\s<>.,;!(){}\"']+"), boost::regex_constants::icase);
					
				tstring::const_iterator start = sMsg.begin();
				tstring::const_iterator end = sMsg.end();
				boost::match_results<tstring::const_iterator> result;

				if(boost::regex_search(start + linkEnd, end, result, reg, boost::match_default)) {
					dcassert(!result.empty());
					
					linkEnd += result.length(0);
					SetSel(lSelBegin + linkStart, lSelBegin + linkEnd);
					if(isMagnet) {
						detectMagnet=true;
						tstring cURL = ((tstring)(result[0]));
						tstring::size_type dn = cURL.find(_T("dn="));
						if(dn != tstring::npos) {
							string sFileName = Util::encodeURI(Text::fromT(cURL).substr(dn + 3), true);
							int64_t filesize = Util::toInt64(Text::fromT(cURL.substr(cURL.find(_T("xl=")) + 3, cURL.find(_T("&")) - cURL.find(_T("xl=")))));
							tstring shortLink = Text::toT(sFileName) + _T(" (") + Util::formatBytesW(filesize) + _T(")");
							ReplaceSel(shortLink.c_str(), false);
							sMsgLower = sMsgLower.substr(0, linkStart) + shortLink + sMsgLower.substr(linkEnd);
							linkEnd = linkStart + shortLink.length();
							SetSel(lSelBegin + linkStart, lSelBegin + linkEnd);
							magnets[shortLink] = _T("magnet:?") + cURL;
						}
						}
					SetSelectionCharFormat(WinUtil::m_TextStyleURL);
				}
			} catch(...) {
			}
			
			linkStart = sMsgLower.find(protocols[i], linkEnd);			
		}
	}

	//Format release names and files as URL
	if(SETTING(FORMAT_RELEASE)) {
		if(!detectMagnet) {
			boost::wregex reg;
			reg.assign(_T("((?<=\\s)([A-Z0-9][A-Za-z0-9-]*)(\\.|_|(-(?=\\S*\\d{4}\\S*)))(\\S+)-(?=\\w*[A-Z]\\w*)(\\w+)(?=(\\W)?\\s))"));
			tstring::const_iterator start = sMsg.begin();
			tstring::const_iterator end = sMsg.end();
			boost::match_results<tstring::const_iterator> result;
			int pos=0;

			while(boost::regex_search(start, end, result, reg, boost::match_default)) {
				SetSel(pos + lSelBegin + result.position(), pos + lSelBegin + result.position() + result.length());
				SetSelectionCharFormat(WinUtil::m_TextStyleURL);
				start = result[0].second;
				pos=pos+result.position() + result.length();
			}
		} else {
			detectMagnet=false;
		}
	}
	else {
		detectMagnet=false;
	}


	// insert emoticons
	if(bUseEmo && emoticonsManager->getUseEmoticons()) {
		const Emoticon::List& emoticonsList = emoticonsManager->getEmoticonsList();
		tstring::size_type lastReplace = 0;
		uint8_t smiles = 0;

		while(true) {
			tstring::size_type curReplace = tstring::npos;
			Emoticon* foundEmoticon = NULL;

			for(Emoticon::Iter emoticon = emoticonsList.begin(); emoticon != emoticonsList.end(); ++emoticon) {
				tstring::size_type idxFound = sMsg.find((*emoticon)->getEmoticonText(), lastReplace);
				if(idxFound < curReplace || curReplace == tstring::npos) {
					curReplace = idxFound;
					foundEmoticon = (*emoticon);
				}
			}

			if(curReplace != tstring::npos && smiles < MAX_EMOTICONS) {
				CHARFORMAT2 cfSel;
				cfSel.cbSize = sizeof(cfSel);

				lSelBegin += (curReplace - lastReplace);
				lSelEnd = lSelBegin + foundEmoticon->getEmoticonText().size();
				SetSel(lSelBegin, lSelEnd);

				GetSelectionCharFormat(cfSel);
				if(!(cfSel.dwEffects & CFE_LINK)) {
					CImageDataObject::InsertBitmap(GetOleInterface(), foundEmoticon->getEmoticonBmp(cfSel.crBackColor));

					++smiles;
					++lSelBegin;
				} else lSelBegin = lSelEnd;
				lastReplace = curReplace + foundEmoticon->getEmoticonText().size();
			} else break;
		}
	}

}

bool ChatCtrl::HitNick(const POINT& p, tstring& sNick, int& iBegin, int& iEnd) {
	if(client == NULL) return false;
	
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

	if(client->findUser(Text::fromT(sN)) != NULL) {
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

bool ChatCtrl::HitIP(const POINT& p, tstring& sIP, int& iBegin, int& iEnd) {
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

bool ChatCtrl::HitURL() {
	long lSelBegin = 0, lSelEnd = 0;
	GetSel(lSelBegin, lSelEnd);

	CHARFORMAT2 cfSel;
	cfSel.cbSize = sizeof(cfSel);
    GetSelectionCharFormat(cfSel);

	return (cfSel.dwEffects & CFE_LINK) == CFE_LINK;
}

tstring ChatCtrl::LineFromPos(const POINT& p) const {
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

LRESULT ChatCtrl::OnRButtonDown(POINT pt) {
	selectedLine = LineFromPos(pt);
	selectedUser.clear();
	selectedIP.clear();
	selectedWord = WordFromPos(pt);

	release = isRelease(pt, false)? true : false;


	// Po kliku dovnitr oznaceneho textu si zkusime poznamenat pripadnej nick ci ip...
	// jinak by nam to neuznalo napriklad druhej klik na uz oznaceny nick =)
	long lSelBegin = 0, lSelEnd = 0;
	GetSel(lSelBegin, lSelEnd);
	int iCharPos = CharFromPos(pt), iBegin = 0, iEnd = 0;
	if((lSelEnd > lSelBegin) && (iCharPos >= lSelBegin) && (iCharPos <= lSelEnd)) {
		if(!HitIP(pt, selectedIP, iBegin, iEnd))
			HitNick(pt, selectedUser, iBegin, iEnd);

		return 1;
	}

	// hightlight IP or nick when clicking on it
	if(HitIP(pt, selectedIP, iBegin, iEnd) || HitNick(pt, selectedUser, iBegin, iEnd)) {
		SetSel(iBegin, iEnd);
		InvalidateRect(NULL);
	}
	return 1;
}

LRESULT ChatCtrl::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	m_bPopupMenu = false;
	bHandled = FALSE;
	return 0;
}

LRESULT ChatCtrl::onSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    if(m_bPopupMenu)
    {
        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW))) ;
		return 1;
    }
    bHandled = FALSE;
	return 0;
}

LRESULT ChatCtrl::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click

	if(pt.x == -1 && pt.y == -1) {
		CRect erc;
		GetRect(&erc);
		pt.x = erc.Width() / 2;
		pt.y = erc.Height() / 2;
		ClientToScreen(&pt);
	}

	POINT ptCl = pt;
	ScreenToClient(&ptCl); 
	OnRButtonDown(ptCl);

	bool boHitURL = HitURL();
	if (!boHitURL)
		selectedURL.clear();

	OMenu menu;
	menu.CreatePopupMenu();
	SearchMenu.CreatePopupMenu();

	if (copyMenu.m_hMenu != NULL) {
		// delete copy menu if it exists
		copyMenu.DestroyMenu();
		copyMenu.m_hMenu = NULL;
	}

	if(selectedUser.empty()) {

		if(!selectedIP.empty()) {
			menu.InsertSeparatorFirst(selectedIP);
			menu.AppendMenu(MF_STRING, IDC_WHOIS_IP, (TSTRING(WHO_IS) + _T(" ") + selectedIP).c_str() );
			 prepareMenu(menu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
			if (client && client->isOp()) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + selectedIP).c_str());
				menu.SetMenuDefaultItem(IDC_BAN_IP);
				menu.AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + selectedIP).c_str());
				menu.AppendMenu(MF_SEPARATOR);
			}
		} 
		else if (release) {
			menu.InsertSeparatorFirst(_T("Release"));
		}
		else {
			menu.InsertSeparatorFirst(_T("Text"));
		}

		menu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		menu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
		menu.AppendMenu(MF_STRING, IDC_SEARCH_BY_TTH, CTSTRING(SEARCH_BY_TTH));
		menu.AppendMenu(MF_SEPARATOR);
		
		if (release) {
			menu.AppendMenu(MF_STRING, IDC_GOOGLE_TITLE, CTSTRING(SEARCH_GOOGLE_TITLE));
			menu.AppendMenu(MF_STRING, IDC_GOOGLE_FULL, CTSTRING(SEARCH_GOOGLE_FULL));
			menu.AppendMenu(MF_STRING, IDC_TVCOM, CTSTRING(SEARCH_TVCOM));
			menu.AppendMenu(MF_STRING, IDC_IMDB, CTSTRING(SEARCH_IMDB));
			menu.AppendMenu(MF_STRING, IDC_METACRITIC, CTSTRING(SEARCH_METACRITIC));
		} else {
			menu.AppendMenu(MF_POPUP, (UINT)(HMENU)SearchMenu, CTSTRING(SEARCH_SITES));
			SearchMenu.AppendMenu(MF_STRING, IDC_GOOGLE_FULL, CTSTRING(SEARCH_GOOGLE));
			SearchMenu.AppendMenu(MF_STRING, IDC_TVCOM, CTSTRING(SEARCH_TVCOM));
			SearchMenu.AppendMenu(MF_STRING, IDC_IMDB, CTSTRING(SEARCH_IMDB));
			SearchMenu.AppendMenu(MF_STRING, IDC_METACRITIC, CTSTRING(SEARCH_METACRITIC));
		}


	} else {
		bool isMe = (selectedUser == Text::toT(client->getMyNick()));

		// click on nick
		copyMenu.CreatePopupMenu();
		copyMenu.InsertSeparatorFirst(TSTRING(COPY));

		for(int j=0; j < OnlineUser::COLUMN_LAST; j++) {
			copyMenu.AppendMenu(MF_STRING, IDC_COPY + j, CTSTRING_I(HubFrame::columnNames[j]));
		}

		menu.InsertSeparatorFirst(selectedUser);

		if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
			menu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
			menu.AppendMenu(MF_SEPARATOR);
		}		

		menu.AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
		menu.AppendMenu(MF_SEPARATOR);
		
		if(!isMe) {
			menu.AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
			menu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
			menu.AppendMenu(MF_SEPARATOR);
			
			const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
			if (client->isOp() || !ou->getIdentity().isOp()) {
				if(HubFrame::ignoreList.find(ou->getUser()) == HubFrame::ignoreList.end()) {
					menu.AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
				} else {    
					menu.AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
				}
				menu.AppendMenu(MF_SEPARATOR);
			}
		}
		
		menu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
		
		if(!isMe) {
			menu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
			menu.AppendMenu(MF_SEPARATOR);
			menu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
			menu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
			menu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
			
			// add user commands
			//prepareMenu(menu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
		}
			// add user commands on self req by endator
			prepareMenu(menu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
		// default doubleclick action
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
		} 
	}

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
	menu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	
	//flag to indicate pop up menu.
    m_bPopupMenu = true;
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

	return 0;
}

LRESULT ChatCtrl::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam != SIZE_MINIMIZED && HIWORD(lParam) > 0) {
		scrollToEnd();
	}

	bHandled = FALSE;
	return 0;
}

LRESULT ChatCtrl::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	selectedLine.clear();
	selectedIP.clear();
	selectedUser.clear();
	selectedURL.clear();

	bHandled = FALSE;
	return 0;
}

LRESULT ChatCtrl::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			GetTextRange(lBegin, lEnd, sURLTemp);
			tstring sURL = sURLTemp;
			//ApexDC
			if(magnets.find(sURL) == magnets.end()) {
				WinUtil::openLink(sURL);
			} else {
				WinUtil::parseMagnetUri(magnets[sURL]);
			}

			delete[] sURLTemp;
		}
	} else if(pEL->msg == WM_RBUTTONUP) {
		selectedURL = Util::emptyStringT;
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			GetTextRange(lBegin, lEnd, sURLTemp);
			//ApexDC
			if(magnets.find(sURLTemp) == magnets.end()) {
				selectedURL = sURLTemp;
			} else {
				selectedURL = magnets[sURLTemp];
			}

			delete[] sURLTemp;
		}

		SetSel(lBegin, lEnd);
		InvalidateRect(NULL);
	}

	return 0;
}

LRESULT ChatCtrl::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Copy();
	return 0;
}

LRESULT ChatCtrl::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetSelAll();
	return 0;
}

LRESULT ChatCtrl::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetWindowText(_T(""));
	return 0;
}

LRESULT ChatCtrl::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedLine.empty()) {
		WinUtil::setClipboard(selectedLine);
	}
	return 0;
}

LRESULT ChatCtrl::onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
		tstring s = _T("!banip ") + selectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT ChatCtrl::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
		tstring s = _T("!unban ") + selectedIP;
		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT ChatCtrl::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedURL.empty()) {
		WinUtil::setClipboard(selectedURL);
	}
	return 0;
}


LRESULT ChatCtrl::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
 		WinUtil::openLink(_T("http://www.ripe.net/perl/whois?form_type=simple&full_query_string=&searchtext=") + selectedIP);
 	}
	return 0;
}

LRESULT ChatCtrl::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		StringMap params;

		params["userNI"] = ou->getIdentity().getNick();
		params["hubNI"] = client->getHubName();
		params["myNI"] = client->getMyNick();
		params["userCID"] = ou->getUser()->getCID().toBase32();
		params["hubURL"] = client->getHubUrl();

		tstring file = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false)));
		if(Util::fileExists(Text::fromT(file))) {
		if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
			ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
			} else {
				TextFrame::openWindow(file);
			}
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}
	}

	return 0;
}

LRESULT ChatCtrl::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou)
		PrivateFrame::openWindow(HintedUser(ou->getUser(), client->getHubUrl()), Util::emptyStringT, client);

	return 0;
}

LRESULT ChatCtrl::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou)
		ou->getList(client->getHubUrl());

	return 0;
}

LRESULT ChatCtrl::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou)
		ou->matchQueue(client->getHubUrl());

	return 0;
}

LRESULT ChatCtrl::onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		uint64_t time = 0;
		switch(wID) {
			case IDC_GRANTSLOT:			time = 600; break;
			case IDC_GRANTSLOT_DAY:		time = 3600; break;
			case IDC_GRANTSLOT_HOUR:	time = 24*3600; break;
			case IDC_GRANTSLOT_WEEK:	time = 7*24*3600; break;
			case IDC_UNGRANTSLOT:		time = 0; break;
		}
		
		if(time > 0)
			UploadManager::getInstance()->reserveSlot(HintedUser(ou->getUser(), client->getHubUrl()), time);
		else
			UploadManager::getInstance()->unreserveSlot(ou->getUser());
	}

	return 0;
}

LRESULT ChatCtrl::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou)
		ou->addFav();

	return 0;
}

LRESULT ChatCtrl::onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/){
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		HubFrame::ignoreList.insert(ou->getUser());
		IgnoreManager::getInstance()->storeIgnore(ou->getUser());
	}
	return 0;
}

LRESULT ChatCtrl::onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/){
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		HubFrame::ignoreList.erase(ou->getUser());
		IgnoreManager::getInstance()->removeIgnore(ou->getUser());
	}
	return 0;
}

LRESULT ChatCtrl::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	
	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		sCopy = ou->getText(static_cast<uint8_t>(wID - IDC_COPY));
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

void ChatCtrl::runUserCommand(UserCommand& uc) {
	StringMap ucParams;

	if(!WinUtil::getUCParams(m_hWnd, uc, ucParams))
		return;

	client->getMyIdentity().getParams(ucParams, "my", true);
	client->getHubIdentity().getParams(ucParams, "hub", false);

	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou != NULL) {
		StringMap tmp = ucParams;
		ou->getIdentity().getParams(tmp, "user", true);
		client->escapeParams(tmp);
		client->sendUserCmd(uc, tmp);
	}
}

string ChatCtrl::escapeUnicode(tstring str) {
	TCHAR buf[8];
	memzero(buf, sizeof(buf));

	int dist = 0;
	tstring::iterator i;
	while((i = std::find_if(str.begin() + dist, str.end(), std::bind2nd(std::greater<TCHAR>(), 0x7f))) != str.end()) {
		dist = (i+1) - str.begin(); // Random Acess iterators FTW
		snwprintf(buf, sizeof(buf), _T("%hd"), int(*i));
		str.replace(i, i+1, _T("\\ud\\u") + tstring(buf) + _T("?"));
		memzero(buf, sizeof(buf));
	}
	return Text::fromT(str);
}

tstring ChatCtrl::rtfEscape(tstring str) {
	tstring::size_type i = 0;
	while((i = str.find_first_of(_T("{}\\\n"), i)) != tstring::npos) {
		switch(str[i]) {
			// no need to process \r handled elsewhere
			case '\n': str.replace(i, 1, _T("\\line\n")); i+=6; break;
			default: str.insert(i, _T("\\")); i+=2;
		}
	}
	return str;
}

void ChatCtrl::scrollToEnd() {
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

int ChatCtrl::FullTextMatch(ColorSettings* cs, CHARFORMAT2 &hlcf, const tstring &line, int pos, long &lineIndex) {
	int index = tstring::npos;
	tstring searchString;

	if( cs->getMyNick() ) {
		tstring::size_type p = cs->getMatch().find(_T("$MyNI$"));
		if(p != tstring::npos) {
			searchString = cs->getMatch();
			searchString = searchString.replace(p, 8, nick);
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
		tstring::size_type pos = line.find(_T("]"));
		if( pos == tstring::npos ) 
			return tstring::npos;  //hmm no ]? this can't be right, return
		
		begin += index +1;
		end = begin + pos -1;
	} else if( cs->getUsers() ) {
		end = begin + line.find(_T(">"));
		begin += index +1;
	} else if( cs->getWholeLine() ) {
		end = begin + line.length();
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
int ChatCtrl::RegExpMatch(ColorSettings* cs, CHARFORMAT2 &hlcf, const tstring &line, long &lineIndex) {
	//TODO: Clean it up a bit
	
	long begin, end;	
	bool found = false;

	//this is not a valid regexp
	if(cs->getMatch().length() < 5)
	return tstring::npos;

	string str = (Text::fromT(cs->getMatch())).substr(4);
	try {
		const boost::wregex reg(Text::toT(str));

	
		boost::wsregex_iterator iter(line.begin(), line.end(), reg);
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

	if(!found)
		return tstring::npos;
	
	CheckAction(cs, line);
	
	return tstring::npos;

}

void ChatCtrl::CheckAction(ColorSettings* cs, const tstring& line) {
	if(cs->getPopup() && !matchedPopup) {
		matchedPopup = true;
		tstring popupTitle;
		popupTitle = _T("Highlight");
		MainFrame::getMainFrame()->ShowBalloonTip(line.c_str(), popupTitle.c_str());
	}

	//Todo maybe
//	if(cs->getTab() && isSet(TAB))
//		matchedTab = true;

//	if(cs->getLog() && !logged && !skipLog){
//		logged = true;
//		AddLogLine(line);
//	}

	if(cs->getPlaySound() && !matchedSound ){
			matchedSound = true;
			PlaySound(cs->getSoundFile().c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NOWAIT);
	}

	if(cs->getFlashWindow())
		WinUtil::FlashWindow();
}
LRESULT ChatCtrl::onDoubleClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	bHandled = isRelease(pt, true);

	return bHandled = TRUE ? 0: 1;
}

BOOL ChatCtrl::isRelease(POINT pt, BOOL search) {
tstring word = WordFromPos(pt);

	boost::wregex reg;
	reg.assign(_T("(([A-Z0-9][A-Za-z0-9-]*)(\\.|_|(-(?=\\S*\\d{4}\\S*)))(\\S+)-(?=\\w*[A-Z]\\w*)(\\w+))"));
	if(regex_match(word, reg)) {
		if (search) {
			WinUtil::search(word, 0, false);
		}
		return TRUE;
	}
	return FALSE;
}

tstring ChatCtrl::WordFromPos(const POINT& p) {

	int iCharPos = CharFromPos(p), line = LineFromChar(iCharPos), len = LineLength(iCharPos) + 1;
	if(len < 3)
		return Util::emptyStringT;

	long begin =  0;
	long end  = 0;
	
	//Walk it thru, want the whole word / releasename instead of just until -
	
	int start = LineIndex(line), lineEnd = LineIndex(line) + LineLength(iCharPos);
	
	for( begin = iCharPos; begin >= start; begin-- ) {
		
		if( FindWordBreak( WB_ISDELIMITER, begin ))
			break;
	}

	begin++;

	for( end = iCharPos; end < lineEnd; end++ ) {
		if(FindWordBreak( WB_ISDELIMITER, end ))
			break;
	}

	len = end - begin;
	tstring sText;
	sText.resize(len);
	GetTextRange(begin, end, &sText[0]);
	
	return sText;

}
LRESULT ChatCtrl::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	CHARRANGE cr;
	GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		GetSelText(buf);
		searchTerm = Util::replace(buf, _T("\r"), _T("\r\n"));
		delete[] buf;
			} else {
			searchTerm = selectedWord;
	}
	WinUtil::search(searchTerm, 0, false);
	searchTerm = Util::emptyStringT;
	return 0;
}

LRESULT ChatCtrl::onSearchTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	CHARRANGE cr;
	GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		GetSelText(buf);
		searchTerm = Util::replace(buf, _T("\r"), _T("\r\n"));
		delete[] buf;
	}
	WinUtil::search(searchTerm, 0, true);
	searchTerm = Util::emptyStringT;
	return 0;
}


LRESULT ChatCtrl::onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	//I think we can use full searchterm here because its user selected from chat anyways.

	CHARRANGE cr;
	GetSel(cr);
	if(cr.cpMax != cr.cpMin) {
		TCHAR *buf = new TCHAR[cr.cpMax - cr.cpMin + 1];
		GetSelText(buf);
		searchTermFull = Util::replace(buf, _T("\r"), _T("\r\n"));
		searchTerm = WinUtil::getTitle(searchTermFull);
		//searchTerm = searchTermFull;
		delete[] buf;
	 } else if(!selectedWord.empty())  { 	 
	              searchTermFull = selectedWord; 
				  searchTerm = WinUtil::getTitle(searchTermFull);
	         }
		switch (wID) {
			case IDC_GOOGLE_TITLE:
				WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;

			case IDC_GOOGLE_FULL:
				WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTermFull))));
				break;

			case IDC_IMDB:
				WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;
			case IDC_TVCOM:
				WinUtil::openLink(_T("http://www.tv.com/search.php?type=11&stype=all&qs=") + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
				break;
			case IDC_METACRITIC:
				WinUtil::openLink(_T("http://www.metacritic.com/search/all/") + Text::toT(Util::encodeURI(Text::fromT(searchTerm)) + "/results"));
				break;
		}
	searchTerm = Util::emptyStringT;
	searchTermFull = Util::emptyStringT;
	return S_OK;
}
