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
	json SettingUtils::serializeDefinition(const ApiSettingItem& aItem) noexcept {
		json ret = {
			{ "key", aItem.name },
			{ "title", aItem.getTitle() },
			{ "type", typeToStr(aItem.type) },
			{ "defaultValue", aItem.getDefaultValue() },
		};

		if (!aItem.getHelpStr().empty()) {
			ret["help"] = aItem.getHelpStr();
		}

		if (aItem.isOptional()) {
			ret["optional"] = true;
		}

		{
			for (const auto& opt : aItem.getEnumOptions()) {
				ret["values"].push_back({
					{ "id", opt.id },
					{ "name", opt.text },
				});
			}
		}

		if (aItem.type == ApiSettingItem::TYPE_NUMBER) {
			const auto& minMax = aItem.getMinMax();

			if (minMax.min != 0) {
				ret["min"] = minMax.min;
			}

			if (minMax.max != MAX_INT_VALUE) {
				ret["max"] = minMax.max;
			}
		}

		if (aItem.type == ApiSettingItem::TYPE_LIST_OBJECT) {
			dcassert(!aItem.getValueTypes().empty());
			for (const auto& valueType: aItem.getValueTypes()) {
				ret["value_definitions"].push_back(serializeDefinition(*valueType));
			}
		}

		return ret;
	}

	string SettingUtils::typeToStr(ApiSettingItem::Type aType) noexcept {
		switch (aType) {
			case ApiSettingItem::TYPE_BOOLEAN: return "boolean";
			case ApiSettingItem::TYPE_NUMBER: return "number";
			case ApiSettingItem::TYPE_STRING: return "string";
			case ApiSettingItem::TYPE_FILE_PATH: return "file_path";
			case ApiSettingItem::TYPE_DIRECTORY_PATH: return "directory_path";
			case ApiSettingItem::TYPE_TEXT: return "text";
			case ApiSettingItem::TYPE_LIST_STRING: return "list_string";
			case ApiSettingItem::TYPE_LIST_NUMBER: return "list_number";
			case ApiSettingItem::TYPE_LIST_OBJECT: return "list_object";
			case ApiSettingItem::TYPE_LAST: dcassert(0);
		}

		dcassert(0);
		return Util::emptyString;
	}

	json SettingUtils::validateObjectListValue(const ApiSettingItem::PtrList& aPropertyDefinitions, const json& aValue) {
		// Unknown properties will be ignored...
		auto ret = json::object();
		for (const auto& def: aPropertyDefinitions) {
			auto i = aValue.find(def->name);
			if (i == aValue.end()) {
				ret[def->name] = validateValue(def->getDefaultValue(), *def);
			} else {
				ret[def->name] = validateValue(i.value(), *def);
			}
		}

		return ret;
	}

	json SettingUtils::validateValue(const json& aValue, const ApiSettingItem& aItem) {
		return validateValue(aValue, aItem.name, aItem.type, aItem.isOptional(), aItem.getMinMax(), aItem.getValueTypes(), aItem.getEnumOptions());
	}

	json SettingUtils::validateValue(const json& aValue, const string& aKey, ApiSettingItem::Type aType, bool aOptional, const ApiSettingItem::MinMax& aMinMax,
		const ApiSettingItem::PtrList& aObjectValues, const ApiSettingItem::EnumOption::List& aEnumOptions) {
		{
			// Validate the current value for enum fields
			if (!aEnumOptions.empty()) {
				if (aType == ApiSettingItem::TYPE_NUMBER || aType == ApiSettingItem::TYPE_STRING) {
					// Single value
					auto i = boost::find_if(aEnumOptions, [&](const ApiSettingItem::EnumOption& opt) { return opt.id == aValue; });
					if (i == aEnumOptions.end()) {
						JsonUtil::throwError(aKey, JsonUtil::ERROR_INVALID, "Value is not one of the enum options");
					}
				} else if (aType == ApiSettingItem::TYPE_LIST_NUMBER || aType == ApiSettingItem::TYPE_LIST_STRING) {
					// Array, validate all values
					for (const auto& itemId: aValue) {
						auto i = boost::find_if(aEnumOptions, [&](const ApiSettingItem::EnumOption& opt) { return opt.id == itemId; });
						if (i == aEnumOptions.end()) {
							JsonUtil::throwError(aKey, JsonUtil::ERROR_INVALID, "All values can't be found from enum options");
						}
					}
				}
			}
		}

		if (aType == ApiSettingItem::TYPE_NUMBER) {
			auto num = JsonUtil::parseValue<int>(aKey, aValue, aOptional);

			// Validate range
			JsonUtil::validateRange(aKey, num, aMinMax.min, aMinMax.max);

			return num;
		} else if (ApiSettingItem::isString(aType)) {
			auto value = JsonUtil::parseValue<string>(aKey, aValue, aOptional);

			// Validate paths
			if (aType == ApiSettingItem::TYPE_DIRECTORY_PATH) {
				value = Util::validatePath(value, true);
			} else if (aType == ApiSettingItem::TYPE_FILE_PATH) {
				value = Util::validateFileName(value);
			}

			return value;
		} else if (aType == ApiSettingItem::TYPE_BOOLEAN) {
			return JsonUtil::parseValue<bool>(aKey, aValue, aOptional);
		} else if (aType == ApiSettingItem::TYPE_LIST_STRING) {
			return JsonUtil::parseValue<vector<string>>(aKey, aValue, aOptional);
		} else if (aType == ApiSettingItem::TYPE_LIST_NUMBER) {
			return JsonUtil::parseValue<vector<int>>(aKey, aValue, aOptional);
		} else if (aType == ApiSettingItem::TYPE_LIST_OBJECT) {
			auto ret = json::array();
			for (const auto& listValueObj: JsonUtil::parseValue<json::array_t>(aKey, aValue, aOptional)) {
				ret.push_back(validateObjectListValue(aObjectValues, JsonUtil::parseValue<json::object_t>(aKey, listValueObj, false)));
			}

			return ret;
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

	json SettingUtils::parseEnumId(const json& aJson, ApiSettingItem::Type aType) {
		if (aType == ApiSettingItem::TYPE_NUMBER) {
			return JsonUtil::getField<int>("id", aJson, false);
		}

		return JsonUtil::getField<string>("id", aJson, false);
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

		auto help = JsonUtil::getOptionalFieldDefault<string>("help", aJson, Util::emptyString);

		ApiSettingItem::MinMax minMax = {
			JsonUtil::getOptionalFieldDefault<int>("min", aJson, 0),
			JsonUtil::getOptionalFieldDefault<int>("max", aJson, MAX_INT_VALUE)
		};

		ServerSettingItem::List objectValues;
		if (type == ApiSettingItem::TYPE_LIST_OBJECT) {
			for (const auto& valueJ: JsonUtil::getRawField("value_definitions", aJson)) {
				objectValues.push_back(deserializeDefinition(valueJ));
			}
		}

		ApiSettingItem::EnumOption::List enumOptions;

		if (type == ApiSettingItem::TYPE_STRING || type == ApiSettingItem::TYPE_NUMBER || type == ApiSettingItem::TYPE_LIST_NUMBER || type == ApiSettingItem::TYPE_LIST_STRING) {
			auto optionsJson = JsonUtil::getOptionalRawField("values", aJson, false);
			if (!optionsJson.is_null()) {
				for (const auto& opt: optionsJson) {
					enumOptions.push_back({
						parseEnumId(opt, type),
						JsonUtil::getField<string>("name", opt, false)
					});
				}
			}
		}

		auto defaultValue = validateValue(
			JsonUtil::getOptionalRawField("defaultValue", aJson, !isOptional), 
			key, type, true, minMax, ApiSettingItem::valueTypesToPtrList(objectValues), enumOptions
		);

		return ServerSettingItem(key, title, defaultValue, type, isOptional, minMax, objectValues, help, enumOptions);
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
		} else if (aTypeStr == "list_string") {
			return ApiSettingItem::TYPE_LIST_STRING;
		} else if (aTypeStr == "list_number") {
			return ApiSettingItem::TYPE_LIST_NUMBER;
		} else if (aTypeStr == "list_object") {
			return ApiSettingItem::TYPE_LIST_OBJECT;
		}

		dcassert(0);
		return ApiSettingItem::TYPE_LAST;
	}
}
