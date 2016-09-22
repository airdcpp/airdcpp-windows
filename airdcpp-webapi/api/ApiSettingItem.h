/*
* Copyright (C) 2011-2016 AirDC++ Project
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

#ifndef DCPLUSPLUS_DCPP_APISETTINGITEM_H
#define DCPLUSPLUS_DCPP_APISETTINGITEM_H

#include <web-server/stdinc.h>

#include <airdcpp/GetSet.h>
#include <airdcpp/SettingItem.h>
#include <airdcpp/ResourceManager.h>

namespace webserver {

	class ApiSettingItem {
	public:
		enum Type {
			TYPE_GENERAL,
			TYPE_FILE_PATH,
			TYPE_DIRECTORY_PATH,
			TYPE_LONG_TEXT,
			TYPE_CONN_V4,
			TYPE_CONN_V6,
			TYPE_CONN_GEN,
			TYPE_LIMITS_DL,
			TYPE_LIMITS_UL,
			TYPE_LIMITS_MCN
		};

		struct Unit {
			const ResourceManager::Strings str;
			const bool isSpeed;
		};

		ApiSettingItem(const string& aName, Type aType = TYPE_GENERAL, Unit&& aUnit = { ResourceManager::Strings::LAST, false });

		virtual json infoToJson(bool aForceAutoValues = false) const noexcept;

		// Returns the value and bool indicating whether it's an auto detected value
		virtual pair<json, bool> valueToJson(bool aForceAutoValues = false) const noexcept = 0;
		virtual const string& getTitle() const noexcept = 0;

		virtual bool setCurValue(const json& aJson) = 0;
		virtual void unset() noexcept = 0;

		const string name;
		const Type type;

		Unit unit;
	};

	class CoreSettingItem : public ApiSettingItem, public SettingItem {
	public:
		CoreSettingItem(const string& aName, int aKey, ResourceManager::Strings aDesc, Type aType = TYPE_GENERAL, Unit&& aUnit = { ResourceManager::Strings::LAST, false });

		json infoToJson(bool aForceAutoValues = false) const noexcept override;

		// Returns the value and bool indicating whether it's an auto detected value
		pair<json, bool> valueToJson(bool aForceAutoValues = false) const noexcept override;
		json autoValueToJson(bool aForceAutoValues) const noexcept;

		// Throws on invalid JSON
		bool setCurValue(const json& aJson) override;
		void unset() noexcept override;

		const string& getTitle() const noexcept override {
			return SettingItem::getDescription();
		}
	};

	class ServerSettingItem : public ApiSettingItem {
	public:
		ServerSettingItem(const string& aKey, const string& aTitle, const json& aDefaultValue, Type aType = TYPE_GENERAL, Unit&& aUnit = { ResourceManager::Strings::LAST, false });

		json infoToJson(bool aForceAutoValues = false) const noexcept override;

		// Returns the value and bool indicating whether it's an auto detected value
		pair<json, bool> valueToJson(bool aForceAutoValues = false) const noexcept override;

		bool setCurValue(const json& aJson) override;

		const string desc;

		const string& getTitle() const noexcept override {
			return desc;
		}

		void unset() noexcept override;

		int num();
		uint64_t uint64();
		string str();

		bool isDefault() const noexcept;

		const json& getValue() const noexcept {
			return value;
		}
	private:
		json value;
		const json defaultValue;
	};
}

#endif