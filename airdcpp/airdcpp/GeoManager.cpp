/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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
#include "GeoManager.h"
#include "File.h"
#include "ZUtils.h"
#include "cmaxminddb.h"
#include "Util.h"

namespace dcpp {

void GeoManager::init() {
	pMaxMindDB = new cMaxMindDB();
	pMaxMindDB->ReloadAll();
	
}

void GeoManager::update() {
	if(decompress()) {
	if(pMaxMindDB)
		pMaxMindDB->ReloadAll();
	}
}

bool GeoManager::decompress() const {

	if (File::getSize(getDbPath()+ ".gz") <= 0) {

		return false;

	}
	try { GZ::decompress(getDbPath() + ".gz", getDbPath()); }

	catch (const Exception&) { return false; }
	return true;
}

void GeoManager::close() {
//TODO: proper close?
	delete pMaxMindDB;
}

const string GeoManager::getCountry(const string& ip) {
	if(!ip.empty()) {
		string cn = "--";
		string cc = "--";
		pMaxMindDB->GetCC(ip,cc);
		pMaxMindDB->GetCN(ip,cn);
		return cc +" - "+cn;
	}

	return Util::emptyString;
}

string GeoManager::getDbPath() {
	return Util::getPath(Util::PATH_USER_CONFIG) + "GeoLite2-Country.mmdb";
}

} // namespace dcpp
