/*
* Copyright (C) 2011-2019 AirDC++ Project
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

#include "stdinc.h"

#include "MessageHighlight.h"

#include <airdcpp/AirUtil.h>
#include <airdcpp/Magnet.h>
#include <airdcpp/MessageCache.h>
#include <airdcpp/ShareManager.h>
#include <airdcpp/OnlineUser.h>


namespace webserver {
	MessageHighlight::MessageHighlight(size_t aStart, const string& aText, HighlightType aType, DupeType aDupeType) : start(aStart), end(aStart + aText.size()), text(aText), type(aType), dupe(aDupeType) {

	}

	int MessageHighlight::LinkSortOrder::operator()(size_t a, size_t b) const noexcept {
		return compare(a, b);
	}

	MessageHighlight::List MessageHighlight::parseHighlights(const string& aTextUtf8, const string& aMyNick, const UserPtr& aUser) {
		MessageHighlight::List ret;

		const auto textW = Text::utf8ToWide(aTextUtf8);

		// My nick
		if (!aMyNick.empty()) {
			const auto myNick = Text::utf8ToWide(aMyNick);
			size_t lMyNickStart = string::npos;
			size_t lSearchFrom = 0;
			while ((lMyNickStart = textW.find(myNick, lSearchFrom)) != string::npos) {
				auto lMyNickEnd = lMyNickStart + aMyNick.size();
				lSearchFrom = lMyNickEnd;

				ret.insert_sorted(MessageHighlight(lMyNickStart, aMyNick, MessageHighlight::HighlightType::TYPE_ME));
			}
		}


		// Parse release names
		if (SETTING(FORMAT_RELEASE) || SETTING(DUPES_IN_CHAT)) {
			auto start = textW.cbegin();
			auto end = textW.cend();

			boost::match_results<wstring::const_iterator> result;
			boost::regex::difference_type pos = 0;

			while (boost::regex_search(start, end, result, AirUtil::releaseRegChatW, boost::match_default)) {
				auto link = Text::wideToUtf8(tstring(result[0].first, result[0].second));

				auto dupe = AirUtil::checkAdcDirectoryDupe(link, 0);

				ret.insert_sorted(MessageHighlight(pos + result.position(), link, MessageHighlight::HighlightType::TYPE_RELEASE, dupe));
				start = result[0].second;
				pos += result.position() + result.length();
			}
		}

		// Parse links
		{
			try {
				auto start = textW.cbegin();
				auto end = textW.cend();
				boost::match_results<wstring::const_iterator> result;
				boost::regex::difference_type pos = 0;

				while (boost::regex_search(start, end, result, AirUtil::urlRegW, boost::match_default)) {
					auto link = Text::wideToUtf8(tstring(result[0].first, result[0].second));

					auto highlight = MessageHighlight(pos + result.position(), link, MessageHighlight::HighlightType::TYPE_URL);

					auto dupe = DupeType::DUPE_NONE;
					if (link.find("magnet:?") == 0) {
						auto m = Magnet::parseMagnet(link);
						if (m) {
							dupe = (*m).getDupeType();
							highlight.setMagnet(m);

							if (dupe == DUPE_NONE && ShareManager::getInstance()->isTempShared(aUser, (*m).getTTH())) {
								dupe = DUPE_SHARE_FULL;
								highlight.setType(MessageHighlight::HighlightType::TYPE_TEMP_SHARE);
							}
						}
					}
					highlight.setDupe(dupe);

					ret.insert_sorted(std::move(highlight));

					start = result[0].second;
					pos += result.position() + result.length();
				}

			} catch (...) {
				//...
			}
		}

		return ret;
	}
}