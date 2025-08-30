/* 
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
#include <windows/resource.h>

#include <atlstr.h>

#include <windows/RichTextBox.h>

#include <airdcpp/util/DupeUtil.h>
#include <airdcpp/favorites/FavoriteManager.h>
#include <airdcpp/user/ignore/IgnoreManager.h>
#include <airdcpp/util/LinkUtil.h>
#include <airdcpp/core/classes/Magnet.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/util/RegexUtil.h>
#include <airdcpp/share/temp_share/TempShareManager.h>
#include <airdcpp/transfer/upload/UploadManager.h>
#include <airdcpp/util/Util.h>
#include <airdcpp/core/version.h>

#include <airdcpp/modules/AutoSearchManager.h>
#include <airdcpp/modules/HighlightManager.h>

#include <boost/lambda/lambda.hpp>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#include <windows/util/ActionUtil.h>
#include <windows/EmoticonsManager.h>
#include <windows/HtmlToRtf.h>
#include <windows/HttpLinks.h>
#include <windows/frames/hub/HubFrame.h>
#include <windows/MainFrm.h>
#include <windows/frames/private_chat/PrivateFrame.h>
#include <windows/ResourceLoader.h>
#include <windows/frames/text/TextFrame.h>


namespace wingui {

#define MAX_EMOTICONS 48
UINT RichTextBox::WM_FINDREPLACE = RegisterWindowMessage(FINDMSGSTRING);


RichTextBox::RichTextBox() : UserInfoBaseHandler(true, true), ccw(const_cast<LPTSTR>(GetWndClassName()), this) {
	showHandCursor=false;
	lastTick = GET_TICK();
	t_height = WinUtil::getTextHeight(m_hWnd, WinUtil::font); //? right height?

	arrowCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	handCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));

	findBuffer = new TCHAR[findBufferSize];
	findBuffer[0] = _T('\0');
}

RichTextBox::~RichTextBox() {
	delete[] findBuffer;
}

std::string RichTextBox::unicodeEscapeFormatter(const tstring_range& match) {
	if(match.empty())
		return std::string();
	return (boost::format("\\ud\\u%dh")%(int(*match.begin()))).str();
}

std::string RichTextBox::escapeUnicode(const tstring& str) {
	tstring ret;
	boost::find_format_all_copy(std::back_inserter(ret), str,
		boost::first_finder(L"\x7f", std::greater<TCHAR>()), unicodeEscapeFormatter);
	return Text::fromT(ret);
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

void RichTextBox::SetText(const string& aText, const MessageHighlight::SortedList& aHighlights, bool bUseEmo /* = false*/) {
	SetRedraw(FALSE);
	
	auto text = Text::toT(aText);
	SetWindowText(text.c_str());
	FormatHighlights(text, aText, aHighlights, 0);
	if (bUseEmo) {
		FormatEmoticons(text, 0);
	}

	SetRedraw(TRUE);
}

void RichTextBox::FormatHighlights(tstring& text_, const string& aText, const MessageHighlight::SortedList& aHighlights, LONG lSelBegin) {
	auto messagePos = 0;
	auto messagePosT = 0;

	for (const auto& highlight: aHighlights) {
		auto textBetween = aText.substr(messagePos, highlight->getStart() - messagePos);

		auto startT = messagePosT + Text::toT(textBetween).length();
		auto endT = startT + Text::toT(highlight->getText()).length();
		SetSel(lSelBegin + startT, lSelBegin + endT);

		/*auto tmp = text_.substr(startT, endT - startT);
		TCHAR* buf = new TCHAR[tmp.length() + 1];
		GetSelText(buf);*/

		// Style
		formatSelectedHighlight(highlight);

		// Replace text
		auto displayText = Text::toT(getHighlightDisplayText(highlight));
		text_.replace(startT, endT - startT, displayText.c_str());
		setText(displayText);

		// Store highlight with positions
		CHARRANGE cr;
		cr.cpMin = lSelBegin + startT;
		cr.cpMax = lSelBegin + startT + displayText.length();
		links.insert_sorted(ChatLinkPair(cr, highlight));

		messagePosT = startT + displayText.length();
		messagePos = highlight->getEnd();
	}
}

void RichTextBox::CheckMessageNotifications(const Message& aMessage) {
	// Notifications
	if (aMessage.chatMessage && aMessage.chatMessage->getFrom()->getUser() != ClientManager::getInstance()->getMe()) {
		auto myNick = find_if(aMessage.getHighlights().begin(), aMessage.getHighlights().end(), [](const auto& highlight) {
			return highlight->getTag() == MessageHighlight::TAG_ME;
			});

		if (myNick != aMessage.getHighlights().end()) {
			if (!SETTING(CHATNAMEFILE).empty() && !SETTING(SOUNDS_DISABLED)) {
				WinUtil::playSound(Text::toT(SETTING(CHATNAMEFILE)));
			}

			if (SETTING(FLASH_WINDOW_ON_MYNICK)) {
				WinUtil::FlashWindow();
			}
		}
	}
}

LONG RichTextBox::FormatTimestampAuthor(bool aIsThirdPerson, const Identity& aIdentity, const tstring& sTime, CHARFORMAT2& cf, LONG lSelBegin) {
	auto lSelEnd = lSelBegin;
	bool isFavorite = false, isOp = false;
	if (aIdentity.getUser()) {
		isFavorite = aIdentity.getUser()->isFavorite();
		isOp = aIdentity.isOp();
	}

	// Format TimeStamp
	if (!sTime.empty()) {
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
	tstring sAuthor = Text::toT(aIdentity.getNick());
	if (!sAuthor.empty()) {
		LONG iLen = aIsThirdPerson ? 1 : 0;
		LONG iAuthorLen = sAuthor.size() + 1;

		lSelBegin = lSelEnd;
		lSelEnd += iAuthorLen + iLen;

		bool isMyMessage = aIdentity.getUser() == ClientManager::getInstance()->getMe();
		if (isMyMessage) {
			SetSel(lSelBegin, lSelBegin + iLen + 1);
			SetSelectionCharFormat(WinUtil::m_ChatTextMyOwn);
			SetSel(lSelBegin + iLen + 1, lSelBegin + iLen + iAuthorLen);
			SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
		} else {
			SetSel(lSelBegin, lSelBegin + iLen + 1);
			SetSelectionCharFormat(cf);
			SetSel(lSelBegin + iLen + 1, lSelEnd);
			if (isFavorite) {
				SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
			} else if (aIdentity.isOp()) {
				SetSelectionCharFormat(WinUtil::m_TextStyleOPs);
			} else {
				SetSelectionCharFormat(WinUtil::m_TextStyleNormUsers);
			}
		}

		lSelEnd += aIsThirdPerson ? 1 : 2;
	} else if (!sTime.empty())  {
		lSelEnd += 4;
	}

	return lSelEnd;
}


LONG RichTextBox::AppendText(tstring& sLine, POINT& pt, LONG& lSelBeginSaved, LONG& lSelEndSaved) {
	LONG lSelBegin = 0, lSelEnd = 0, lTextLimit = 0, lNewTextLen = 0;
	lSelEnd = lSelBegin = GetTextLengthEx(GTL_NUMCHARS);

	// Remove old chat if size exceeds
	lNewTextLen = sLine.size();
	lTextLimit = GetLimitText();

	if (lSelEnd + lNewTextLen > lTextLimit) {
		LONG lRemoveChars = 0;
		int multiplier = 1;

		if (lNewTextLen >= lTextLimit) {
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

		// Remove old links (the list must be in the same order than in text)
		for (auto i = links.begin(); i != links.end();) {
			if ((*i).first.cpMin < lRemoveChars) {
				links.erase(i);
				i = links.begin();
			} else {
				// Update link position
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
	return lSelEnd;
}

bool RichTextBox::AppendMessage(const Message& aMessage, CHARFORMAT2& cf, bool bUseEmoAndHighligts) {
	SetRedraw(FALSE);
	matchedTab = false;

	// Get previous scroll position and selected text
	SCROLLINFO si = { 0 };
	POINT pt = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_VERT, &si);
	GetScrollPos(&pt);

	LONG lSelBeginSaved, lSelEndSaved;
	GetSel(lSelBeginSaved, lSelEndSaved);

	auto from = aMessage.type == Message::TYPE_CHAT ? aMessage.chatMessage->getFrom() : nullptr;
	auto isThirdPerson = aMessage.type == Message::TYPE_CHAT ? aMessage.chatMessage->getThirdPerson() : false;
	auto identity = from ? from->getIdentity() : Identity();

	// Construct line
	auto timestamp = aMessage.getTime() == 0 ? Util::emptyStringT : WinUtil::formatMessageTimestamp(identity, aMessage.getTime());
	auto text = Text::toT(aMessage.getText());
	auto formattedLine = timestamp + Text::toT(aMessage.format()) + _T("\n");
	auto isMyMessage = identity.getUser() == ClientManager::getInstance()->getMe();

	// Append
	auto lSelBegin = AppendText(formattedLine, pt, lSelBeginSaved, lSelEndSaved);

	// Style
	SetSel(lSelBegin, -1);
	SetSelectionCharFormat(isMyMessage ? WinUtil::m_ChatTextMyOwn : cf);

	// Format timestamp and author
	lSelBegin = FormatTimestampAuthor(isThirdPerson, identity, timestamp, cf, lSelBegin);

	// Format the message part

	// Highlights
	if (bUseEmoAndHighligts) {
		// Handle legacy highlights first so that they won't override link formatting
		FormatLegacyHighlights(text, lSelBegin);
	}
	FormatHighlights(text, aMessage.getText(), aMessage.getHighlights(), lSelBegin);

	// Emoticons
	if (bUseEmoAndHighligts) {
		FormatEmoticons(text, lSelBegin);
	}

	CheckMessageNotifications(aMessage);

	// Scroll position and previous selection
	SetSel(lSelBeginSaved, lSelEndSaved);
	auto isBottom = (si.nPage == 0 || (size_t)si.nPos >= (size_t)si.nMax - si.nPage - 5);
	auto scrollToBottom = isMyMessage || (isBottom &&
		(lSelBeginSaved == lSelEndSaved || !selectedUser.empty() || !selectedIP.empty() || selectedHighlight));

	if (scrollToBottom) {
		PostMessage(EM_SCROLL, SB_BOTTOM, 0);
	} else {
		SetScrollPos(&pt);
	}

	// Force window to redraw
	SetRedraw(TRUE);
	InvalidateRect(NULL);
	return matchedTab;
}

void RichTextBox::FormatLegacyHighlights(tstring& sText, LONG lSelBegin) {
	// Set text format

	if(SETTING(USE_HIGHLIGHT)) {

		ColorList *cList = HighlightManager::getInstance()->getList();
		CHARFORMAT2 hlcf;
		logged = false;
		//compare the last line against all strings in the vector
		for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
			ColorSettings* cs = &(*i);
			if(cs->getContext() != HighlightManager::CONTEXT_CHAT) 
				continue;
			size_t pos = 0;
			tstring msg = sText;

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
}

void RichTextBox::parsePathHighlights(const string& aText, MessageHighlight::SortedList& highlights_) noexcept {
	auto start = aText.cbegin();
	auto end = aText.cend();
	boost::match_results<string::const_iterator> result;
	int pos = 0;
	boost::regex reg(RegexUtil::getPathReg());

	while (boost::regex_search(start, end, result, reg, boost::match_default)) {
		std::string path(result[0].first, result[0].second);
		highlights_.insert_sorted(make_shared<MessageHighlight>(pos + result.position(), path, MessageHighlight::HighlightType::TYPE_LINK_TEXT, "path"));

		start = result[0].second;
		pos = pos + result.position() + result.length();
	}
}

void RichTextBox::FormatEmoticons(tstring& sMsg, LONG lSelBegin) {
	// insert emoticons
	auto em = EmoticonsManager::getInstance();
	if (em->getUseEmoticons()) {
		const auto& emoticonsList = em->getEmoticonsList();
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
					for (auto& l : links | views::reverse) {
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

void RichTextBox::formatHighlight(const MessageHighlightPtr& aHighlight, CHARRANGE& cr) {
	CHARRANGE oldCr;
	GetSel(oldCr);

	SetSel(cr);
	formatSelectedHighlight(aHighlight);
	SetSel(oldCr);
}

void RichTextBox::formatSelectedHighlight(const MessageHighlightPtr& aHighlight) {
	auto dupe = aHighlight->getDupe();
	if (SETTING(DUPES_IN_CHAT) && dupe != DUPE_NONE) {
		if (DupeUtil::isShareDupe(dupe)) {
			SetSelectionCharFormat(WinUtil::m_TextStyleDupe);
		} else if (DupeUtil::isQueueDupe(dupe)) {
			SetSelectionCharFormat(WinUtil::m_TextStyleQueue);
		} else if (DupeUtil::isFinishedDupe(dupe)) {
			CHARFORMAT2 newFormat = WinUtil::m_TextStyleQueue;
			newFormat.crTextColor = WinUtil::getDupeColors(dupe).first;
			SetSelectionCharFormat(newFormat);
		}
	} else {
		switch (aHighlight->getType()) {
			case MessageHighlight::HighlightType::TYPE_BOLD: {
				SetSelectionCharFormat(WinUtil::m_TextStyleBold);
				break;
			}
			case MessageHighlight::HighlightType::TYPE_USER: {
				if (aHighlight->getTag() == MessageHighlight::TAG_ME) {
					SetSelectionCharFormat(WinUtil::m_TextStyleMyNick);
				} else if (aHighlight->getTag() == MessageHighlight::TAG_FAVORITE) {
					SetSelectionCharFormat(WinUtil::m_TextStyleFavUsers);
				} else {
					SetSelectionCharFormat(WinUtil::m_TextStyleNormUsers);
				}
				
				break;
			}
			case MessageHighlight::HighlightType::TYPE_LINK_TEXT:
			case MessageHighlight::HighlightType::TYPE_LINK_URL:
			default: {
				SetSelectionCharFormat(WinUtil::m_TextStyleURL);
			}
		}
	}
}

string RichTextBox::getHighlightDisplayText(const MessageHighlightPtr& aHighlight) const noexcept {
	if (aHighlight->getMagnet()) {
		auto m = *aHighlight->getMagnet();
		return m.fname + " (" + Util::formatBytes(m.fsize) + ")";
	}

	return aHighlight->getText();
}

void RichTextBox::clearSelInfo() {
	selectedLine.clear();
	selectedUser.clear();
	selectedIP.clear();
	selectedHighlight = nullptr, isTTH = false, isPath = false;
	selectedWord.clear();
}

void RichTextBox::updateSelectedText(POINT pt, bool aSelectLink) {
	selectedLine = LineFromPos(pt);

	auto p = getLink(pt);
	if (p != links.crend()) {
		selectedHighlight = p->second;
		selectedWord = Text::toT(selectedHighlight->getText());

		// Check if the dupe status has changed
		CHARRANGE cr = p->first;
		formatHighlight(selectedHighlight, cr);

		if (aSelectLink) {
			SetSel(p->first.cpMin, p->first.cpMax);
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

			// Replace shortened links in the range.
			for (const auto& l : links | views::reverse) {
				if (l.first.cpMin >= cr.cpMin && l.first.cpMax <= cr.cpMax) {
					selectedWord.replace(l.first.cpMin - cr.cpMin, getHighlightDisplayText(l.second).length(), Text::toT(l.second->getText()));
				}
			}

			//Change line breaks
			selectedWord = Util::replaceT(selectedWord, _T("\r"), _T("\r\n"));
		} else {
			selectedWord = WordFromPos(pt);
			if (selectedWord.length() == 39) {
				isTTH = true;
			}
		}
	}
}

void RichTextBox::OnRButtonDown(POINT pt) {
	clearSelInfo();

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
	updateAuthor();
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

	if (selectedUser.empty()) {
		auto isMagnet = false, isRelease = false;
		auto dupeType = DupeType::DUPE_NONE;
		if (selectedHighlight) {
			isMagnet = !!selectedHighlight->getMagnet();
			dupeType = selectedHighlight->getDupe();
			isRelease = selectedHighlight->getTag() == MessageHighlight::TAG_RELEASE;
		}

		if(!selectedIP.empty()) {
			menu.InsertSeparatorFirst(selectedIP);
			menu.AppendMenu(MF_STRING, IDC_WHOIS_IP, (TSTRING(WHO_IS) + _T(" ") + selectedIP).c_str() );
			if (client)
				prepareMenu(menu, UserCommand::CONTEXT_USER, getHubUrl());

			if (client && client->isOp()) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + selectedIP).c_str());
				menu.SetMenuDefaultItem(IDC_BAN_IP);
				menu.AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + selectedIP).c_str());
				menu.AppendMenu(MF_SEPARATOR);
			}
		} else if (isRelease) {
			menu.InsertSeparatorFirst(CTSTRING(RELEASE));
		} else if (!selectedWord.empty() && LinkUtil::isHubLink(Text::fromT(selectedWord))) {
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

			if (dupeType != DUPE_NONE) {
				menu.appendSeparator();
				menu.appendItem(TSTRING(OPEN_FOLDER), [this] { handleOpenFolder(); });
			}

			menu.appendSeparator();
			if (isMagnet) {
				auto isMyLink = client && Text::toT(client->getMyNick()) == author;
				auto magnet = *selectedHighlight->getMagnet();
				if (TempShareManager::getInstance()->isTempShared(getTempShareUser(), magnet.getTTH())) {
					/* show an option to remove the item */
					menu.appendItem(TSTRING(STOP_SHARING), [this] { handleRemoveTemp(); });
				} else if (!isMyLink) {
					appendDownloadMenu(menu, DownloadBaseHandler::TYPE_PRIMARY, false, magnet.getTTH(), nullopt);
				}

				if ((!author.empty() && !isMyLink) || DupeUtil::allowOpenFileDupe(dupeType))
					menu.appendItem(TSTRING(OPEN), [this] { handleOpenFile(); });
			} else if (isRelease) {
				//autosearch menus
				appendDownloadMenu(menu, DownloadBaseHandler::TYPE_SECONDARY, true, nullopt, Text::fromT(selectedWord) + PATH_SEPARATOR, false);
			}
		}

		if (contextMenuHandler) {
			contextMenuHandler(menu, selectedHighlight);
		}
	} else {
		bool isMe = (selectedUser == Text::toT(client->getMyNick()));

		// click on nick
		copyMenu.CreatePopupMenu();
		copyMenu.InsertSeparatorFirst(TSTRING(COPY));

		for(int j=0; j < UserUtil::COLUMN_LAST; j++) {
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
		prepareMenu(menu, UserCommand::CONTEXT_USER, getHubUrl());

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
			if(LinkUtil::isAdcHub(getHubUrl())) {
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
		if (!PathUtil::isDirectoryPath(path)) {
			menu.appendSeparator();
			menu.appendItem(TSTRING(SEARCH_FILENAME), [this] { handleSearch(); });
			if (PathUtil::fileExists(path)) {
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
	ActionUtil::search(Text::toT(DupeUtil::getReleaseDirLocal(Text::fromT(selectedWord), true)), true);
}

void RichTextBox::handleDeleteFile() {
	string path = Text::fromT(selectedWord);
	string msg = STRING_F(DELETE_FILE_CONFIRM, path);
	if(WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_FILE_DELETIONS, Text::toT(msg))) {
		MainFrame::getMainFrame()->addThreadedTask([=] { File::deleteFileEx(path, 3); });
	}

	SetSelNone();
}

void RichTextBox::handleAddAutoSearchFile() {
	string targetPath;
	if (selectedWord.find(PATH_SEPARATOR) != tstring::npos)
		targetPath = PathUtil::getFilePath(Text::fromT(selectedWord));
	auto fileName = PathUtil::getFileName(Text::fromT(selectedWord));

	AutoSearchManager::getInstance()->addAutoSearch(fileName, targetPath, false, AutoSearch::CHAT_DOWNLOAD, true);

	SetSelNone();
}

void RichTextBox::handleAddAutoSearchDir() {
	string targetPath = PathUtil::getParentDir(Text::fromT(selectedWord), PATH_SEPARATOR, true);
	string dirName = PathUtil::getLastDir(!PathUtil::isDirectoryPath(selectedWord) ? PathUtil::getFilePath(Text::fromT(selectedWord)) : Text::fromT(selectedWord));

	AutoSearchManager::getInstance()->addAutoSearch(dirName, targetPath, true, AutoSearch::CHAT_DOWNLOAD, true);

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
	if (!selectedHighlight || !selectedHighlight->getMagnet()) {
		return;
	}

	auto m = *selectedHighlight->getMagnet();
	auto u = getMagnetSource();
	MainFrame::getMainFrame()->addThreadedTask([=] {
		if (selectedHighlight->getDupe() != DUPE_NONE) {
			auto paths = DupeUtil::getFileDupePaths(selectedHighlight->getDupe(), m.getTTH());
			if (!paths.empty())
				ActionUtil::openFile(Text::toT(paths.front()));
		} else {
			ActionUtil::openTextFile(m.fname, m.fsize, m.getTTH(), u, false);
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
		return HintedUser(u, getHubUrl());

	return HintedUser();
}

UserPtr RichTextBox::getTempShareUser() const noexcept {
	return (pmUser && !pmUser->isSet(User::BOT) && !pmUser->isSet(User::NMDC)) ? pmUser : nullptr;
}

void RichTextBox::handleRemoveTemp() {
	if (!selectedHighlight || !selectedHighlight->getMagnet()) {
		return;
	}

	auto magnet = *selectedHighlight->getMagnet();
	auto tempShareToken = TempShareManager::getInstance()->isTempShared(getTempShareUser(), magnet.getTTH());
	if (!tempShareToken) {
		return;
	}

	TempShareManager::getInstance()->removeTempShare(*tempShareToken);
	for (auto& p: links) {
		decltype(auto) highlight = p.second;
		if (highlight->getMagnet() && highlight->getMagnet()->getTTH() == magnet.getTTH()) {
			CHARRANGE cr = p.first;
			formatHighlight(highlight, cr);
		}
	}
}

void RichTextBox::handleOpenFolder() {
	StringList paths;
	try{
		if (isPath) {
			paths.push_back(Text::fromT(PathUtil::getFilePath(selectedWord)));
		} else if (selectedHighlight && selectedHighlight->getDupe() != DUPE_NONE) {
			if (selectedHighlight->getMagnet()) {
				auto m = *selectedHighlight->getMagnet();
				if (m.hash.empty())
					return;

				paths = DupeUtil::getFileDupePaths(selectedHighlight->getDupe(), m.getTTH());
			} else {
				paths = DupeUtil::getAdcDirectoryDupePaths(selectedHighlight->getDupe(), Text::fromT(selectedWord));
			}
		}
	} catch(...) {}

	if (!paths.empty())
		ActionUtil::openFolder(Text::toT(paths.front()));
}

void RichTextBox::handleDownload(const string& aTarget, Priority p, bool aIsRelease) {
	if (!aIsRelease && selectedHighlight && selectedHighlight->getMagnet()) {
		auto u = std::move(getMagnetSource());
		auto magnet = *selectedHighlight->getMagnet();
		if (pmUser && ShareManager::getInstance()->isRealPathShared(PathUtil::getFilePath(aTarget)) &&
			!WinUtil::showQuestionBox(TSTRING_F(PM_MAGNET_SHARED_WARNING, Text::toT(PathUtil::getFilePath(aTarget))), MB_ICONQUESTION)) {
				return;
		}

		auto target = aTarget + (!PathUtil::isDirectoryPath(aTarget) ? Util::emptyString : magnet.fname);
		ActionUtil::addFileDownload(target, magnet.fsize, magnet.getTTH(), u, 0, pmUser ? QueueItem::FLAG_PRIVATE : 0, p);
	} else {
		AutoSearchManager::getInstance()->addAutoSearch(Text::fromT(selectedWord), aTarget, true, AutoSearch::CHAT_DOWNLOAD);
	}
}

bool RichTextBox::showDirDialog(string& fileName) {
	if (selectedHighlight && selectedHighlight->getMagnet()) {
		fileName = (*selectedHighlight->getMagnet()).fname;
		return false;
	}
	return true;
}

int64_t RichTextBox::getDownloadSize(bool /*isWhole*/) {
	if (selectedHighlight && selectedHighlight->getMagnet()) {
		return (*selectedHighlight->getMagnet()).fsize;
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
	clearSelInfo();

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

	return ranges::find_if(links | views::reverse, [iCharPos](const ChatLinkPair& l) { 
		return iCharPos >= l.first.cpMin && iCharPos <= l.first.cpMax; 
	});
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
	if (selectedHighlight) {
		openLink(selectedHighlight);
	}
}

LRESULT RichTextBox::onClientEnLink(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	auto p = getLink(pt);
	if (p == links.crend())
		return FALSE;

	auto highlight = p->second;
	selectedWord = Text::toT(highlight->getText()); // for magnets
	updateSelectedText(pt, false);
	updateAuthor();

	openLink(highlight);

	return TRUE;
}

void RichTextBox::openLink(const MessageHighlightPtr& aHighlight) {
	if (aHighlight->getMagnet()) {
		auto u = getMagnetSource();
		ActionUtil::parseMagnetUri(Text::toT(aHighlight->getText()), u, this);
	} else {
		// The url regex also detects web links without any protocol part
		auto link = aHighlight->getText();
		if (aHighlight->getType() == MessageHighlight::HighlightType::TYPE_LINK_URL)
			link = LinkUtil::parseLink(link);

		ActionUtil::openLink(Text::toT(link));
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
		client->sendMessageHooked(OutgoingChatMessage(Text::fromT(s), this, WinUtil::ownerId, false), error);
	}
	SetSelNone();
	return 0;
}

LRESULT RichTextBox::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!selectedIP.empty()) {
		tstring s = _T("!unban ") + selectedIP;

		string error;
		client->sendMessageHooked(OutgoingChatMessage(Text::fromT(s), this, WinUtil::ownerId, false), error);
	}
	SetSelNone();
	return 0;
}


LRESULT RichTextBox::onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	handleSearchIP(selectedIP);
	SetSelNone();
	return 0;
}

void RichTextBox::handleSearchIP(const tstring& aIP) noexcept {
	if (!aIP.empty()) {
		ActionUtil::openLink(HttpLinks::ipSearchBase + aIP);
	}
}

LRESULT RichTextBox::onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		auto file = ou->getLogPath();
		if(PathUtil::fileExists(file)) {
			ActionUtil::viewLog(file, wID == IDC_USER_HISTORY);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}
	}

	return 0;
}

void RichTextBox::handleIgnore() {
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		IgnoreManager::getInstance()->storeIgnore(ou->getUser());
	}
}

void RichTextBox::handleUnignore(){
	OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou){
		IgnoreManager::getInstance()->removeIgnore(ou->getUser());
	}
}

LRESULT RichTextBox::onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	
	const OnlineUserPtr ou = client->findUser(Text::fromT(selectedUser));
	if(ou) {
		sCopy = UserUtil::getUserText(ou, static_cast<uint8_t>(wID - IDC_COPY), true);
	}

	if (!sCopy.empty())
		WinUtil::setClipboard(sCopy);

	return 0;
}

void RichTextBox::runUserCommand(UserCommand& uc) {
	ParamMap ucParams;

	if (!ActionUtil::getUCParams(m_hWnd, uc, ucParams))
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
		auto tmp = line.find_last_of(_T(" \t\r"), index);
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

	auto begin = Line.find_last_of(_T(" \t\r"), iCharPos) + 1;	
	auto end = Line.find_first_of(_T(" \t\r"), begin);
	if (end == tstring::npos) {
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

			ActionUtil::openLink(getLinkText(link));
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
	if (selectedHighlight && selectedHighlight->getMagnet()) {
		ActionUtil::search(Text::toT((*selectedHighlight->getMagnet()).fname));
	} else if (isPath) {
		ActionUtil::search(PathUtil::getFileName(selectedWord));
	} else {
		ActionUtil::search(selectedWord);
	}
	SetSelNone();
}

void RichTextBox::handleSearchTTH() {
	if (selectedHighlight && selectedHighlight->getMagnet()) {
		auto m = *selectedHighlight->getMagnet();
		ActionUtil::searchHash(m.getTTH(), m.fname, m.fsize);
	} else {
		ActionUtil::searchHash(TTHValue(Text::fromT(selectedWord)), Util::emptyString, 0);
	}
	SetSelNone();
}
}