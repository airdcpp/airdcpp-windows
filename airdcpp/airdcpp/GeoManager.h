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

#ifndef DCPLUSPLUS_DCPP_GEO_MANAGER_H
#define DCPLUSPLUS_DCPP_GEO_MANAGER_H

#include "forward.h"
#include "cmaxminddb.h"
#include "Singleton.h"

#include <memory>
#include <string>

namespace dcpp {

using std::string;
using std::unique_ptr;

/** Manages IP->country mappings. */
class GeoManager : public Singleton<GeoManager>
{
public:
	/** Prepare the databases and fill internal caches. */
	void init();
	/** Update a database and its internal caches. Call after a new one have been downloaded. */
	void update();
	/** Unload databases and clear internal caches. */
	void close();
	/** Map an IP address to a country.*/
	const string getCountry(const string& ip);

	static string getDbPath();

private:
	friend class Singleton<GeoManager>;
	cMaxMindDB *pMaxMindDB;
	bool decompress() const;
	GeoManager(): pMaxMindDB(NULL) { }
	virtual ~GeoManager() { }
};

} // namespace dcpp

#endif
