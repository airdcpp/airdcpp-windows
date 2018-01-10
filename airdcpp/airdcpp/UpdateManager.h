/*
 * Copyright (C) 2012-2017 AirDC++ Project
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

#ifndef DCPLUSPLUS_DCPP_UPDATE_MANAGER_H
#define DCPLUSPLUS_DCPP_UPDATE_MANAGER_H

#include "HttpDownload.h"
#include "Singleton.h"
#include "Speaker.h"
#include "TimerManagerListener.h"
#include "Updater.h"
#include "UpdateManagerListener.h"

#include "version.h"

namespace dcpp {

class UpdateManager : public Singleton<UpdateManager>, public Speaker<UpdateManagerListener>, private TimerManagerListener
{

public:
	UpdateManager();
	~UpdateManager();

	struct {
		string homepage;
		string downloads;
		string geoip6;
		string geoip4;
		string guides;
		string customize;
		string discuss;
		string language;
		string ipcheck4;
		string ipcheck6;
	} links;

	static bool verifyVersionData(const string& data, const ByteVector& singature);

	enum {
		CONN_VERSION,
		CONN_GEO_V6,
		CONN_GEO_V4,
		CONN_LANGUAGE_FILE,
		CONN_LANGUAGE_CHECK,
		CONN_SIGNATURE,
		CONN_IP4,
		CONN_IP6,

		CONN_LAST
	};

	unique_ptr<HttpDownload> conns[CONN_LAST];

	void checkVersion(bool aManual);
	void checkLanguage();

	void checkIP(bool manual, bool v6);

	void checkGeoUpdate();

	void init();

	void checkAdditionalUpdates(bool manualCheck);
	string getVersionUrl() const;

	Updater& getUpdater() const noexcept {
		return *updater.get();
	}
private:
	unique_ptr<Updater> updater;

	void failVersionDownload(const string& aError, bool manualCheck);

	static const char* versionUrl[VERSION_LAST];

	uint64_t lastIPUpdate;
	static uint8_t publicKey[270];

	ByteVector versionSig;

	void updateGeo();

	void completeSignatureDownload(bool manual);
	void completeLanguageCheck();
	void completeGeoDownload();
	void completeVersionDownload(bool manualCheck);
	void completeLanguageDownload();
	void completeIPCheck(bool manualCheck, bool v6);

	void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;
};

} // namespace dcpp

#endif // UPDATE_MANAGER_H