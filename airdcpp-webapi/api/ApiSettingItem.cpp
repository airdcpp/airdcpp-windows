/*
* Copyright (C) 2011-2015 AirDC++ Project
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
#include <airdcpp/SearchManager.h>
#include <airdcpp/SettingHolder.h>
#include <airdcpp/SettingItem.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/StringTokenizer.h>

namespace webserver {
	ApiSettingItem::ApiSettingItem(const string& aName, int aKey, ResourceManager::Strings aDesc, Type aType) :
		name(aName), type(aType), SettingItem({ aKey, aDesc }) {

	}

	//NamedSettingItem::NamedSettingItem(ResourceManager::Strings aDesc) : isTitle(true), SettingItem({ 0, aDesc }) {

	//}

	json ApiSettingItem::toJson() const noexcept {
		json v;
		bool autoValue = false;

		if ((type == TYPE_CONN_V4 && SETTING(AUTO_DETECT_CONNECTION)) || (type == TYPE_CONN_V6 && SETTING(AUTO_DETECT_CONNECTION6)) ||
			(type == TYPE_CONN_GEN && (SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)))) {

			if (key == SettingsManager::TCP_PORT) {
				v = ConnectionManager::getInstance()->getPort();
			} else if (key == SettingsManager::UDP_PORT) {
				v = SearchManager::getInstance()->getPort();
			} else if (key == SettingsManager::TLS_PORT) {
				v = ConnectionManager::getInstance()->getSecurePort();
			} else {
				if (key >= SettingsManager::STR_FIRST && key < SettingsManager::STR_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(key));
				} else if (key >= SettingsManager::INT_FIRST && key < SettingsManager::INT_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(key));
				} else if (key >= SettingsManager::BOOL_FIRST && key < SettingsManager::BOOL_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(key));
				} else {
					dcassert(0);
				}
			}

			autoValue = true;
		} else if (type == TYPE_LIMITS_DL && SETTING(DL_AUTODETECT)) {
			if (key == SettingsManager::DOWNLOAD_SLOTS)
				v = AirUtil::getSlots(true);
			else if (key == SettingsManager::MAX_DOWNLOAD_SPEED)
				v = AirUtil::getSpeedLimit(true);
			//else
			//	v = AirUtil::getMaxAutoOpened();

			autoValue = true;
		} else if (type == TYPE_LIMITS_UL && SETTING(UL_AUTODETECT)) {
			if (key == SettingsManager::SLOTS)
				v = AirUtil::getSlots(false);
			else if (key == SettingsManager::MIN_UPLOAD_SPEED)
				v = AirUtil::getSpeedLimit(false);
			else if (key == SettingsManager::AUTO_SLOTS)
				v = AirUtil::getMaxAutoOpened();

			autoValue = true;
		} else if (type == TYPE_LIMITS_MCN && SETTING(MCN_AUTODETECT)) {
			v = AirUtil::getSlotsPerUser(key == SettingsManager::MAX_MCN_DOWNLOADS);
			autoValue = true;
		}

		string value;
		if (v.is_null()) {
			if (key >= SettingsManager::STR_FIRST && key < SettingsManager::STR_LAST) {
				v = SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(key), true);
			} else if (key >= SettingsManager::INT_FIRST && key < SettingsManager::INT_LAST) {
				v = SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(key), true);
			} else if (key >= SettingsManager::BOOL_FIRST && key < SettingsManager::BOOL_LAST) {
				v = SettingsManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(key), true);
			} else {
				dcassert(0);
			}
		}

		json ret;
		ret["value"] = v;
		ret["key"] = name;
		ret["title"] = getDescription();
		if (autoValue) {
			ret["auto"] = true;
		}

		auto enumStrings = SettingsManager::getEnumStrings(key, false);
		if (!enumStrings.empty()) {
			for (const auto& i : enumStrings) {
				ret["values"][Util::toString(i.first)] = i.second;
			}
		}

		return ret;
	}

	bool ApiSettingItem::setCurValue(const json& aJson) const {
		if ((type == TYPE_CONN_V4 && SETTING(AUTO_DETECT_CONNECTION)) ||
			(type == TYPE_CONN_V6 && SETTING(AUTO_DETECT_CONNECTION6))) {
			//display::Manager::get()->cmdMessage("Note: Connection autodetection is enabled for the edited protocol. The changed setting won't take effect before auto detection has been disabled.");
		}

		if ((type == TYPE_LIMITS_DL && SETTING(DL_AUTODETECT)) ||
			(type == TYPE_LIMITS_UL && SETTING(UL_AUTODETECT)) ||
			(type == TYPE_LIMITS_MCN && SETTING(MCN_AUTODETECT))) {

			//display::Manager::get()->cmdMessage("Note: auto detection is enabled for the edited settings group. The changed setting won't take effect before auto detection has been disabled.");
		}

		if (key >= SettingsManager::STR_FIRST && key < SettingsManager::STR_LAST) {
			const string value = aJson;
			SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(key), value);
		} else if (key >= SettingsManager::INT_FIRST && key < SettingsManager::INT_LAST) {
			int value = aJson;
			SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(key), value);
		} else if (key >= SettingsManager::BOOL_FIRST && key < SettingsManager::BOOL_LAST) {
			bool value = aJson;
			SettingsManager::getInstance()->set(static_cast<SettingsManager::BoolSetting>(key), value);
		} else {
			dcassert(0);
			return false;
		}

		return true;
	}
}
