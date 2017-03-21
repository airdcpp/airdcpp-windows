/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#include <web-server/stdinc.h>
#include <api/ApiSettingItem.h>
#include <web-server/JsonUtil.h>

#include <airdcpp/AirUtil.h>
#include <airdcpp/ConnectionManager.h>
#include <airdcpp/ConnectivityManager.h>
#include <airdcpp/ResourceManager.h>
#include <airdcpp/SearchManager.h>
#include <airdcpp/SettingHolder.h>
#include <airdcpp/SettingItem.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/StringTokenizer.h>

namespace webserver {
	const ApiSettingItem::MinMax ApiSettingItem::defaultMinMax = { 0, MAX_INT_VALUE };

	ApiSettingItem::ApiSettingItem(const string& aName, Type aType) :
		name(aName), type(aType) {

	}

	string ApiSettingItem::typeToStr(Type aType) noexcept {
		switch (aType) {
			case TYPE_BOOLEAN: return "boolean";
			case TYPE_NUMBER: return "number";
			case TYPE_STRING: return "string";
			case TYPE_FILE_PATH: return "file_path";
			case TYPE_DIRECTORY_PATH: return "directory_path";
			case TYPE_TEXT: return "text";
		}

		dcassert(0);
		return Util::emptyString;
	}



	json ApiSettingItem::serializeDefinitions(bool) const noexcept {
		// Serialize the setting
		json ret = {
			{ "key", name },
			{ "title", getTitle() },
			{ "type", typeToStr(type) },
		};

		if (isOptional()) {
			ret["optional"] = true;
		}

		{
			for (const auto& opt: getEnumOptions()) {
				ret["values"].push_back({
					{ "id", opt.id },
					{ "name", opt.text },
				});
			}
		}

		if (type == TYPE_NUMBER) {
			const auto& minMax = getMinMax();
			
			if (minMax.min != 0) {
				ret["min"] = minMax.min;
			}

			if (minMax.max != MAX_INT_VALUE) {
				ret["max"] = minMax.max;
			}
		}

		return ret;
	}

	ServerSettingItem::ServerSettingItem(const string& aKey, const string& aTitle, const json& aDefaultValue, Type aType, bool aOptional, const MinMax& aMinMax) :
		ApiSettingItem(aKey, aType), desc(aTitle), defaultValue(aDefaultValue), value(aDefaultValue), optional(aOptional), minMax(aMinMax) {

	}

	json ServerSettingItem::serializeDefinitions(bool aForceAutoValues) const noexcept {
		return ApiSettingItem::serializeDefinitions(aForceAutoValues);
	}

	// Returns the value and bool indicating whether it's an auto detected value
	pair<json, bool> ServerSettingItem::valueToJson(bool /*aForceAutoValues*/) const noexcept {
		return { value, false };
	}

	void ServerSettingItem::unset() noexcept {
		value = defaultValue;
	}

	bool ServerSettingItem::setCurValue(const json& aJson) {
		if (aJson.is_null()) {
			unset();
		} else {
			// The value should have been validated before
			value = aJson;
		}

		return true;
	}

	int ServerSettingItem::num() {
		return value.get<int>();
	}

	uint64_t ServerSettingItem::uint64() {
		return value.get<uint64_t>();
	}

	string ServerSettingItem::str() {
		if (value.is_number()) {
			return Util::toString(num());
		}

		return value.get<string>();
	}

	bool ServerSettingItem::boolean() {
		return value.get<bool>();
	}

	bool ServerSettingItem::isDefault() const noexcept {
		return value == defaultValue;
	}

	json ServerSettingItem::getDefaultValue() const noexcept {
		return defaultValue;
	}

	ApiSettingItem::EnumOption::List ServerSettingItem::getEnumOptions() const noexcept {
		ApiSettingItem::EnumOption::List ret;
		return ret;
	}


	const ApiSettingItem::MinMax& ServerSettingItem::getMinMax() const noexcept {
		return minMax;
	}

	map<int, CoreSettingItem::Group> groupMappings = {
		{ SettingsManager::TCP_PORT, CoreSettingItem::GROUP_CONN_GEN },
		{ SettingsManager::UDP_PORT, CoreSettingItem::GROUP_CONN_GEN },
		{ SettingsManager::TLS_PORT, CoreSettingItem::GROUP_CONN_GEN },
		{ SettingsManager::MAPPER, CoreSettingItem::GROUP_CONN_GEN },

		{ SettingsManager::BIND_ADDRESS, CoreSettingItem::GROUP_CONN_V4 },
		{ SettingsManager::INCOMING_CONNECTIONS, CoreSettingItem::GROUP_CONN_V4 },
		{ SettingsManager::EXTERNAL_IP, CoreSettingItem::GROUP_CONN_V4 },
		{ SettingsManager::IP_UPDATE, CoreSettingItem::GROUP_CONN_V4 },
		{ SettingsManager::NO_IP_OVERRIDE, CoreSettingItem::GROUP_CONN_V4 },

		{ SettingsManager::BIND_ADDRESS6, CoreSettingItem::GROUP_CONN_V6 },
		{ SettingsManager::INCOMING_CONNECTIONS6, CoreSettingItem::GROUP_CONN_V6 },
		{ SettingsManager::EXTERNAL_IP6, CoreSettingItem::GROUP_CONN_V6 },
		{ SettingsManager::IP_UPDATE6, CoreSettingItem::GROUP_CONN_V6 },
		{ SettingsManager::NO_IP_OVERRIDE6, CoreSettingItem::GROUP_CONN_V6 },

		{ SettingsManager::DOWNLOAD_SLOTS, CoreSettingItem::GROUP_LIMITS_DL },
		{ SettingsManager::MAX_DOWNLOAD_SPEED, CoreSettingItem::GROUP_LIMITS_DL },

		{ SettingsManager::MIN_UPLOAD_SPEED, CoreSettingItem::GROUP_LIMITS_UL },
		{ SettingsManager::AUTO_SLOTS, CoreSettingItem::GROUP_LIMITS_UL },
		{ SettingsManager::SLOTS, CoreSettingItem::GROUP_LIMITS_UL },

		{ SettingsManager::MAX_MCN_DOWNLOADS, CoreSettingItem::GROUP_LIMITS_MCN },
		{ SettingsManager::MAX_MCN_UPLOADS, CoreSettingItem::GROUP_LIMITS_MCN },
	};

	map<int, CoreSettingItem::MinMax> minMaxMappings = {
		{ SettingsManager::TCP_PORT, { 1, 65535 } },
		{ SettingsManager::UDP_PORT, { 1, 65535 } },
		{ SettingsManager::TLS_PORT, { 1, 65535 } },

		{ SettingsManager::MAX_HASHING_THREADS, { 1, 100 } },
		{ SettingsManager::HASHERS_PER_VOLUME, { 1, 100 } },

		{ SettingsManager::MAX_COMPRESSION, { 0, 9 } },
		{ SettingsManager::MINIMUM_SEARCH_INTERVAL, { 5, 1000 } },

		{ SettingsManager::SLOTS, { 1, 250 } },
		{ SettingsManager::DOWNLOAD_SLOTS, { 1, 250 } },

		// No validation for other enums at the moment but negative value would cause issues otherwise...
		{ SettingsManager::INCOMING_CONNECTIONS, { SettingsManager::INCOMING_DISABLED, SettingsManager::INCOMING_LAST } },
		{ SettingsManager::INCOMING_CONNECTIONS6, { SettingsManager::INCOMING_DISABLED, SettingsManager::INCOMING_LAST } },
	};

	set<int> optionalSettingKeys = {
		SettingsManager::DESCRIPTION,
		SettingsManager::EMAIL,

		SettingsManager::EXTERNAL_IP,
		SettingsManager::EXTERNAL_IP6,

		SettingsManager::DEFAULT_AWAY_MESSAGE,
		SettingsManager::SKIPLIST_DOWNLOAD,
		SettingsManager::SKIPLIST_SHARE,
		SettingsManager::FREE_SLOTS_EXTENSIONS,
	};

	CoreSettingItem::CoreSettingItem(const string& aName, int aKey, ResourceManager::Strings aDesc, Type aType, ResourceManager::Strings aUnit) :
		ApiSettingItem(aName, parseAutoType(aType, aKey)), si({ aKey, aDesc }), unit(aUnit) {

	}

	ApiSettingItem::Type CoreSettingItem::parseAutoType(Type aType, int aKey) noexcept {
		if (aKey >= SettingsManager::STR_FIRST && aKey < SettingsManager::STR_LAST) {
			if (aType == TYPE_LAST) return TYPE_STRING;
			dcassert(isString(aType));
		} else if (aKey >= SettingsManager::INT_FIRST && aKey < SettingsManager::INT_LAST) {
			if (aType == TYPE_LAST) return TYPE_NUMBER;
			dcassert(aType == TYPE_NUMBER);
		} else if (aKey >= SettingsManager::BOOL_FIRST && aKey < SettingsManager::BOOL_LAST) {
			if (aType == TYPE_LAST) return TYPE_BOOLEAN;
			dcassert(aType == TYPE_BOOLEAN);
		} else {
			dcassert(0);
		}

		return aType;
	}


	#define USE_AUTO(aType, aSetting) ((groupMappings.find(SettingsManager::aSetting) != groupMappings.end() && groupMappings.at(SettingsManager::aSetting) == aType) && (aForceAutoValues || SETTING(aSetting)))
	json CoreSettingItem::autoValueToJson(bool aForceAutoValues) const noexcept {
		json v;
		if (USE_AUTO(GROUP_CONN_V4, AUTO_DETECT_CONNECTION) || USE_AUTO(GROUP_CONN_V6, AUTO_DETECT_CONNECTION6) ||
			(type == GROUP_CONN_GEN && (SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)))) {

			if (si.key == SettingsManager::TCP_PORT) {
				v = ConnectionManager::getInstance()->getPort();
			} else if (si.key == SettingsManager::UDP_PORT) {
				v = SearchManager::getInstance()->getPort();
			} else if (si.key == SettingsManager::TLS_PORT) {
				v = ConnectionManager::getInstance()->getSecurePort();
			} else {
				if (isString(type)) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(si.key));
				} else if (type == TYPE_NUMBER) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(si.key));
				} else if (type == TYPE_BOOLEAN) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(si.key));
				} else {
					dcassert(0);
				}
			}
		} else if (USE_AUTO(GROUP_LIMITS_DL, DL_AUTODETECT)) {
			if (si.key == SettingsManager::DOWNLOAD_SLOTS) {
				v = AirUtil::getSlots(true);
			} else if (si.key == SettingsManager::MAX_DOWNLOAD_SPEED) {
				v = AirUtil::getSpeedLimit(true);
			}
		} else if (USE_AUTO(GROUP_LIMITS_UL, UL_AUTODETECT)) {
			if (si.key == SettingsManager::SLOTS) {
				v = AirUtil::getSlots(false);
			} else if (si.key == SettingsManager::MIN_UPLOAD_SPEED) {
				v = AirUtil::getSpeedLimit(false);
			} else if (si.key == SettingsManager::AUTO_SLOTS) {
				v = AirUtil::getMaxAutoOpened();
			}
		} else if (USE_AUTO(GROUP_LIMITS_MCN, MCN_AUTODETECT)) {
			v = AirUtil::getSlotsPerUser(si.key == SettingsManager::MAX_MCN_DOWNLOADS);
		}

		return v;
	}

	const ApiSettingItem::MinMax& CoreSettingItem::getMinMax() const noexcept {
		auto i = minMaxMappings.find(si.key);
		return i != minMaxMappings.end() ? i->second : defaultMinMax;
	}

	bool CoreSettingItem::isOptional() const noexcept {
		return optionalSettingKeys.find(si.key) != optionalSettingKeys.end();
	}

	pair<json, bool> CoreSettingItem::valueToJson(bool aForceAutoValues) const noexcept {
		auto v = autoValueToJson(aForceAutoValues);
		if (!v.is_null()) {
			return { v, true };
		}

		if (isString(type)) {
			v = SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(si.key), true);
		} else if (type == TYPE_NUMBER) {
			v = SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(si.key), true);
		} else if (type == TYPE_BOOLEAN) {
			v = SettingsManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(si.key), true);
		} else {
			dcassert(0);
		}

		return { v, false };
	}

	json CoreSettingItem::getDefaultValue() const noexcept {
		if (isString(type)) {
			return SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::StrSetting>(si.key));
		} else if (type == TYPE_NUMBER) {
			return SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(si.key));
		} else if (type == TYPE_BOOLEAN) {
			return SettingsManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(si.key));
		} else {
			dcassert(0);
		}

		return 0;
	}

	ApiSettingItem::EnumOption::List CoreSettingItem::getEnumOptions() const noexcept {
		EnumOption::List ret;

		auto enumStrings = SettingsManager::getEnumStrings(si.key, false);
		if (!enumStrings.empty()) {
			for (const auto& i : enumStrings) {
				ret.emplace_back(EnumOption({ i.first, ResourceManager::getInstance()->getString(i.second) }));
			}
		} else if (si.key == SettingsManager::BIND_ADDRESS || si.key == SettingsManager::BIND_ADDRESS6) {
			auto bindAddresses = AirUtil::getBindAdapters(si.key == SettingsManager::BIND_ADDRESS6);
			for (const auto& adapter : bindAddresses) {
				auto name = adapter.ip + (!adapter.adapterName.empty() ? " (" + adapter.adapterName + ")" : Util::emptyString);
				ret.emplace_back(EnumOption({ adapter.ip, name }));
			}
		} else if (si.key == SettingsManager::MAPPER) {
			auto mappers = ConnectivityManager::getInstance()->getMappers(false);
			for (const auto& mapper : mappers) {
				ret.emplace_back(EnumOption({ mapper, mapper }));
			}
		}

		return ret;
	}

	json CoreSettingItem::serializeDefinitions(bool aForceAutoValues) const noexcept {
		// Serialize the setting
		json ret = ApiSettingItem::serializeDefinitions(aForceAutoValues);

		// TODO: this shouldn't be here
		if (!autoValueToJson(aForceAutoValues).is_null()) {
			ret["auto"] = true;
		}

		return ret;
	}

	string CoreSettingItem::getTitle() const noexcept {
		auto title = si.getDescription();

		if (unit != ResourceManager::LAST) {
			title += " " + ResourceManager::getInstance()->getString(unit);
		}

		return title;
	}

	void CoreSettingItem::unset() noexcept {
		si.unset();
	}

	bool CoreSettingItem::setCurValue(const json& aJson) {
		if (isString(type)) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(si.key), JsonUtil::parseValue<string>(name, aJson));
		} else if (type == TYPE_NUMBER) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(si.key), JsonUtil::parseValue<int>(name, aJson));
		} else if (type == TYPE_BOOLEAN) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::BoolSetting>(si.key), JsonUtil::parseValue<bool>(name, aJson));
		} else {
			dcassert(0);
			return false;
		}

		return true;
	}
}
