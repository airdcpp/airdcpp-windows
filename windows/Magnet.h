/*
 * Copyright (C) 2012 AirDC++ Project
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

#include "../client/stdinc.h"

#include <string>
#include <map>
#include "../client/StringTokenizer.h"
#include "../client/forward.h"
#include "../client/Text.h"
#include "../client/Util.h"
#include "../client/HashValue.h"

namespace dcpp {

using std::string;

/** Struct for a magnet uri */
struct Magnet {
	tstring fname, type, param, fhash;
	//TTHValue tth;
	int64_t fsize;

	//extra information for downloading the files
	string target, nick;

	explicit Magnet(const tstring& aLink) { 
		// official types that are of interest to us
		//  xt = exact topic
		//  xs = exact substitute
		//  as = acceptable substitute
		//  dn = display name
		//  xl = exact length
		StringTokenizer<tstring> mag(aLink.substr(8), _T('&'));
		typedef map<tstring, tstring> MagMap;
		MagMap hashes;
		for(TStringList::const_iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			// break into pairs
			string::size_type pos = idx->find(_T('='));
			if(pos != string::npos) {
				type = Text::toT(Text::toLower(Util::encodeURI(Text::fromT(idx->substr(0, pos)), true)));
				param = Text::toT(Util::encodeURI(Text::fromT(idx->substr(pos+1)), true));
			} else {
				type = Text::toT(Util::encodeURI(Text::fromT(*idx), true));
				param.clear();
			}
			// extract what is of value
			if(param.length() == 85 && strnicmp(param.c_str(), _T("urn:bitprint:"), 13) == 0) {
				hashes[type] = param.substr(46);
			} else if(param.length() == 54 && strnicmp(param.c_str(), _T("urn:tree:tiger:"), 15) == 0) {
				hashes[type] = param.substr(15);
			} else if(param.length() == 55 && strnicmp(param.c_str(), _T("urn:tree:tiger/:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} else if(param.length() == 59 && strnicmp(param.c_str(), _T("urn:tree:tiger/1024:"), 20) == 0) {
				hashes[type] = param.substr(20);
			} else if(type.length() == 2 && strnicmp(type.c_str(), _T("dn"), 2) == 0) {
				fname = param;
			} else if(type.length() == 2 && strnicmp(type.c_str(), _T("xl"), 2) == 0) {
				fsize = _tstoi64(param.c_str());
			}
		}

		//tstring fhash;
		// pick the most authoritative hash out of all of them.
		if(hashes.find(_T("as")) != hashes.end()) {
			fhash = hashes[_T("as")];
		}
		if(hashes.find(_T("xs")) != hashes.end()) {
			fhash = hashes[_T("xs")];
		}
		if(hashes.find(_T("xt")) != hashes.end()) {
			fhash = hashes[_T("xt")];
		}

		/*if(!fhash.empty() && Encoder::isBase32(Text::fromT(fhash).c_str())){
			tth = TTHValue(Text::fromT(fhash));
		} */
	}

	int8_t isQueueDupe() { return QueueManager::getInstance()->isFileQueued(TTHValue(Text::fromT(fhash)), Text::fromT(fname)); }
	bool isShareDupe() { return ShareManager::getInstance()->isFileShared(TTHValue(Text::fromT(fhash)), Text::fromT(fname)); }
	TTHValue getTTH() { return TTHValue(Text::fromT(fhash)); }

	/*bool operator==(const UserPtr& rhs) const {
		return user == rhs;
	}
	bool operator==(const HintedUser& rhs) const {
		return user == rhs.user;
		// ignore the hint, we don't want lists with multiple instances of the same user...
	} */
};

}
