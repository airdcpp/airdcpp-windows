/*
	Free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/
#include "stdinc.h"
#include "cmaxminddb.h"
#include <sstream>
#include <iostream>
#include "GeoManager.h"
#include <stdlib.h>
#include <stdio.h>
#include "File.h"
#include <maxminddb.h>
#include "LogManager.h"

namespace dcpp {
using namespace std;

cMaxMindDB::cMaxMindDB():
	mDBCO(NULL)
{
	mDBCO = TryCountryDB(MMDB_MODE_MMAP);
}

cMaxMindDB::~cMaxMindDB()
{
	if (mDBCO) {
		MMDB_close(mDBCO);
		free(mDBCO);
		mDBCO = NULL;
	}
}

bool cMaxMindDB::GetCC(const string &host, string &cc)
{
	if (host.substr(0, 4) == "127.") {
		cc = "L1";
		return true;
	}

	unsigned long sip = Ip2Num(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cc = "P1";
		return true;
	}

	bool res = false;
	string code = "--";
	string conv;

	if (mDBCO) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string(mDBCO, host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "country", "iso_code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // country code
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				res = true;
			}
		}
	}

	cc = conv;
	return res;
}

bool cMaxMindDB::GetCN(const string &host, string &cn)
{
	if (host.substr(0, 4) == "127.") {
		cn = "Local Network";
		return true;
	}

	unsigned long sip = Ip2Num(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cn = "Private Network";
		return true;
	}

	bool res = false;
	string name = "--";
	string conv;
	if (mDBCO) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string(mDBCO, host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "country", "names", "en", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english country name
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				res = true;
			}
		}
	}

	cn = conv;
	return res;
}

MMDB_s *cMaxMindDB::TryCountryDB(unsigned int flags)
{
	MMDB_s *mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

	if (mmdb) {
		int ok = MMDB_FILE_OPEN_ERROR;

		if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoIP2-Country.mmdb"))
			ok = MMDB_open("/usr/share/GeoIP/GeoIP2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoIP2-Country.mmdb"))
			ok = MMDB_open("/usr/local/share/GeoIP/GeoIP2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists("./GeoIP2-Country.mmdb"))
			ok = MMDB_open("./GeoIP2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoLite2-Country.mmdb"))
			ok = MMDB_open("/usr/share/GeoIP/GeoLite2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoLite2-Country.mmdb"))
			ok = MMDB_open("/usr/local/share/GeoIP/GeoLite2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists("./GeoLite2-Country.mmdb"))
			ok = MMDB_open("./GeoLite2-Country.mmdb", flags, mmdb);

		if ((ok != MMDB_SUCCESS) && FileExists(GeoManager::getDbPath().c_str()))
			ok = MMDB_open(GeoManager::getDbPath().c_str(), flags, mmdb);



		if (ok != MMDB_SUCCESS) {
			if (mmdb) {
				free(mmdb);
				mmdb = NULL;
			}
			ParamMap p;
			p["message"] =  string("Database error: ") + MMDB_strerror(ok) + " [ MaxMind Country > http://geolite.maxmind.com/download/geoip/database/GeoLite2-Country.tar.gz ]";
			LOG(LogManager::SYSTEM,p);
		}
	}

	return mmdb;
}

void cMaxMindDB::ReloadAll()
{
	if (mDBCO) {
		MMDB_close(mDBCO);
		free(mDBCO);
	}

	mDBCO = TryCountryDB(MMDB_MODE_MMAP);
}

bool cMaxMindDB::FileExists(const char *name)
{
	return File::getSize(name) != 0;
}
};
