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
#include <web-server/JsonUtil.h>

#include <api/common/SettingUtils.h>

#include <airdcpp/Util.h>


namespace webserver {
	json SettingUtils::validateValue(const ApiSettingItem& aItem, const json& aValue) {
		if (aItem.type == ApiSettingItem::TYPE_NUMBER) {
			auto num = JsonUtil::parseValue<int>(aItem.name, aValue, aItem.isOptional());

			// Validate range
			JsonUtil::validateRange(aItem.name, num, aItem.getMinMax().min, aItem.getMinMax().max);

			return num;
		} else if (ApiSettingItem::isString(aItem.type)) {
			auto value = JsonUtil::parseValue<string>(aItem.name, aValue, aItem.isOptional());

			// Validate paths
			if (aItem.type == ApiSettingItem::TYPE_DIRECTORY_PATH) {
				value = Util::validatePath(value, true);
			} else if (aItem.type == ApiSettingItem::TYPE_FILE_PATH) {
				value = Util::validateFileName(value);
			}

			return value;
		} else if (aItem.type == ApiSettingItem::TYPE_BOOLEAN) {
			return JsonUtil::parseValue<bool>(aItem.name, aValue, aItem.isOptional());
		}

		dcassert(0);
		return nullptr;
	}

	ServerSettingItem::List SettingUtils::deserializeDefinitions(const json& aJson) {
		ServerSettingItem::List ret;

		for (const auto& def: aJson) {
			ret.push_back(deserializeDefinition(def));
		}

		return ret;
	}

	ServerSettingItem SettingUtils::deserializeDefinition(const json& aJson) {
		auto key = JsonUtil::getField<string>("key", aJson, false);
		auto title = JsonUtil::getField<string>("title", aJson, false);

		auto typeStr = JsonUtil::getField<string>("type", aJson, false);
		auto type = parseType(typeStr);
		if (type == ApiSettingItem::TYPE_LAST) {
			JsonUtil::throwError("type", JsonUtil::ERROR_INVALID, "Invalid type " + typeStr);
		}

		auto isOptional = JsonUtil::getOptionalFieldDefault<bool>("optional", aJson, false);
		if (isOptional && (type == ApiSettingItem::TYPE_BOOLEAN || type == ApiSettingItem::TYPE_NUMBER)) {
			JsonUtil::throwError("optional", JsonUtil::ERROR_INVALID, "Field of type " + typeStr + " can't be optional");
		}

		auto defaultValue = JsonUtil::getOptionalRawField("defaultValue", aJson, !isOptional);

		auto minValue = JsonUtil::getOptionalFieldDefault<int>("min", aJson, 0);
		auto maxValue = JsonUtil::getOptionalFieldDefault<int>("max", aJson, MAX_INT_VALUE);

		return ServerSettingItem(key, title, defaultValue, type, isOptional, { minValue, maxValue });
	}

	ApiSettingItem::Type SettingUtils::parseType(const string& aTypeStr) noexcept {
		if (aTypeStr == "string") {
			return ApiSettingItem::TYPE_STRING;
		} else if (aTypeStr == "boolean") {
			return ApiSettingItem::TYPE_BOOLEAN;
		} else if (aTypeStr == "number") {
			return ApiSettingItem::TYPE_NUMBER;
		} else if (aTypeStr == "text") {
			return ApiSettingItem::TYPE_TEXT;
		} else if (aTypeStr == "file_path") {
			return ApiSettingItem::TYPE_FILE_PATH;
		} else if (aTypeStr == "directory_path") {
			return ApiSettingItem::TYPE_DIRECTORY_PATH;
		}

		dcassert(0);
		return ApiSettingItem::TYPE_LAST;
	}
}
