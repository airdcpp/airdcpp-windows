/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
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

#include <windows/util/FormatUtil.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/core/geo/GeoManager.h>
#include <airdcpp/core/localization/Localization.h>
#include <airdcpp/util/SystemUtil.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/util/Util.h>


namespace wingui {
tstring FormatUtil::getNicks(const CID& cid) {
	return Text::toT(Util::listToString(ClientManager::getInstance()->getNicks(cid)));
}

tstring FormatUtil::getNicks(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getFormattedNicks(user));
}


tstring FormatUtil::formatFolderContent(const DirectoryContentInfo& aContentInfo) {
	return Text::toT(Util::formatDirectoryContent(aContentInfo));
}

tstring FormatUtil::formatFileType(const string& aFileName) {
	auto type = PathUtil::getFileExt(aFileName);
	if (type.size() > 0 && type[0] == '.')
		type.erase(0, 1);

	return Text::toT(type);
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if (hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::listToString(hubs)), true);
	}
}

pair<tstring, bool> FormatUtil::getHubNames(const CID& cid) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid));
}

tstring FormatUtil::getHubNames(const HintedUser& aUser) {
	return Text::toT(ClientManager::getInstance()->getFormattedHubNames(aUser));
}

FormatUtil::CountryFlagInfo FormatUtil::toCountryInfo(const string& aIP) noexcept {
	auto ip = aIP;
	uint8_t flagIndex = 0;
	if (!ip.empty()) {
		// Only attempt to grab a country mapping if we actually have an IP address
		auto tmpCountry = GeoManager::getInstance()->getCountry(aIP);
		if (!tmpCountry.empty()) {
			ip = tmpCountry + " (" + ip + ")";
			flagIndex = Localization::getFlagIndexByCode(tmpCountry.c_str());
		}
	}

	return {
		Text::toT(ip),
		flagIndex
	};
}


string FormatUtil::formatDateTime(time_t t) noexcept {
	if (t == 0)
		return Util::emptyString;

	char buf[64];
	tm _tm;
	auto err = localtime_s(&_tm, &t);
	if (err > 0) {
		dcdebug("Failed to parse date " I64_FMT ": %s\n", t, SystemUtil::translateError(err).c_str());
		return Util::emptyString;
	}

	strftime(buf, 64, SETTING(DATE_FORMAT).c_str(), &_tm);

	return buf;
}

tstring FormatUtil::formatDateTimeW(time_t t) noexcept {
	return Text::toT(formatDateTime(t));
}

string FormatUtil::formatConnectionSpeed(int64_t aBytes) noexcept {
	aBytes *= 8;
	char buf[64];
	if (aBytes < 1000000) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes / (1000.0), CSTRING(KBITS));
	}
	else if (aBytes < 1000000000) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes / (1000000.0), CSTRING(MBITS));
	}
	else if (aBytes < (int64_t)1000000000000) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes / (1000000000.0), CSTRING(GBITS));
	}
	else if (aBytes < (int64_t)1000000000000000) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes / (1000000000000.0), CSTRING(TBITS));
	}
	else if (aBytes < (int64_t)1000000000000000000) {
		snprintf(buf, sizeof(buf), "%.02f %s", (double)aBytes / (1000000000000000.0), CSTRING(PBITS));
	}

	return buf;
}

wstring FormatUtil::formatConnectionSpeedW(int64_t aBytes) noexcept {
	wchar_t buf[64];
	aBytes *= 8;
	if (aBytes < 1000000) {
		snwprintf(buf, sizeof(buf), L"%.02f %s", (double)aBytes / (1000.0), CWSTRING(KBITS));
	}
	else if (aBytes < 1000000000) {
		snwprintf(buf, sizeof(buf), L"%.02f %s", (double)aBytes / (1000000.0), CWSTRING(MBITS));
	}
	else if (aBytes < (int64_t)1000000000000) {
		snwprintf(buf, sizeof(buf), L"%.02f %s", (double)aBytes / (1000000000.0), CWSTRING(GBITS));
	}
	else if (aBytes < (int64_t)1000000000000000) {
		snwprintf(buf, sizeof(buf), L"%.02f %s", (double)aBytes / (1000000000000.0), CWSTRING(TBITS));
	}
	else if (aBytes < (int64_t)1000000000000000000) {
		snwprintf(buf, sizeof(buf), L"%.02f %s", (double)aBytes / (1000000000000000.0), CWSTRING(PBITS));
	}

	return buf;
}

wstring FormatUtil::formatExactSizeW(int64_t aBytes) noexcept {
	wchar_t buf[64];
	wchar_t number[64];
	NUMBERFMT nf;
	snwprintf(number, sizeof(number), _T("%I64d"), aBytes);
	wchar_t Dummy[16];
	TCHAR sep[2] = _T(",");

	/*No need to read these values from the system because they are not
	used to format the exact size*/
	nf.NumDigits = 0;
	nf.LeadingZero = 0;
	nf.NegativeOrder = 0;
	nf.lpDecimalSep = sep;

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, Dummy, 16);
	nf.Grouping = _wtoi(Dummy);
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, Dummy, 16);
	nf.lpThousandSep = Dummy;

	GetNumberFormat(LOCALE_USER_DEFAULT, 0, number, &nf, buf, 64);

	snwprintf(buf, sizeof(buf), _T("%s %s"), buf, CTSTRING(B));
	return buf;
}

string FormatUtil::formatExactSize(int64_t aBytes) noexcept {
	TCHAR tbuf[128];
	TCHAR number[64];
	NUMBERFMT nf;
	_sntprintf(number, 64, _T("%I64d"), aBytes);
	TCHAR Dummy[16];
	TCHAR sep[2] = _T(",");

	/*No need to read these values from the system because they are not
	used to format the exact size*/
	nf.NumDigits = 0;
	nf.LeadingZero = 0;
	nf.NegativeOrder = 0;
	nf.lpDecimalSep = sep;

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, Dummy, 16);
	nf.Grouping = Util::toInt(Text::fromT(Dummy));
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, Dummy, 16);
	nf.lpThousandSep = Dummy;

	GetNumberFormat(LOCALE_USER_DEFAULT, 0, number, &nf, tbuf, sizeof(tbuf) / sizeof(tbuf[0]));

	char buf[128];
	_snprintf(buf, sizeof(buf), "%s %s", Text::fromT(tbuf).c_str(), CSTRING(B));
	return buf;
}

}