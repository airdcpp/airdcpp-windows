/*
* Copyright (C) 2011-2024 AirDC++ Project
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

#ifndef DCPP_PROPERTY_H
#define DCPP_PROPERTY_H

#include <airdcpp/util/text/StringMatch.h>

namespace webserver {

	enum SerializationMethod {
		SERIALIZE_TEXT,
		SERIALIZE_NUMERIC,
		SERIALIZE_BOOL,
		SERIALIZE_CUSTOM
	};

	enum FilterPropertyType {
		TYPE_TEXT,
		TYPE_SIZE,
		TYPE_TIME,
		TYPE_SPEED,
		TYPE_NUMERIC_OTHER,
		TYPE_IMAGE,
		TYPE_LIST_NUMERIC,
		TYPE_LIST_TEXT,
	};

	enum SortMethod {
		SORT_TEXT,
		SORT_NUMERIC,
		SORT_CUSTOM,
		SORT_NONE
	};

	struct Property {
		const int id;
		const std::string name;
		const FilterPropertyType filterType;
		const SerializationMethod serializationMethod;
		const SortMethod sortMethod;
	};

	using PropertyList = vector<Property>;

	using PropertyIdSet = std::set<int>;

	// Creates a list of numeric IDs of all properties
	static inline PropertyIdSet toPropertyIdSet(const PropertyList& aProperties) {
		PropertyIdSet ret;
		for (const auto& p : aProperties)
			ret.insert(p.id);

		return ret;
	}

	static inline int findPropertyByName(const string& aPropertyName, const PropertyList& aProperties) {
		auto p = ranges::find_if(aProperties, [&](const Property& aProperty) { return aProperty.name == aPropertyName; });
		if (p == aProperties.end()) {
			return -1;
		}

		return (*p).id;
	}

	template <class T>
	struct PropertyItemHandler {
		using ItemList = vector<T>;
		using CustomPropertySerializer = std::function<json (const T &, int)>;
		using CustomFilterFunction = std::function<bool (const T &, int, const StringMatch &, double)>;

		using SorterFunction = std::function<int (const T &, const T &, int)>;
		using StringFunction = std::function<string (const T &, int)>;
		using NumberFunction = std::function<double (const T &, int)>;
		using ItemListFunction = std::function<ItemList ()>;

		PropertyItemHandler(const PropertyList& aProperties,
			StringFunction aStringF, NumberFunction aNumberF, 
			SorterFunction aSorterF, CustomPropertySerializer aJsonF,
			CustomFilterFunction aFilterF = nullptr) :

			properties(aProperties),
			stringF(aStringF), numberF(aNumberF), 
			customSorterF(aSorterF), jsonF(aJsonF),
			customFilterF(aFilterF) { }

		// Information about each property
		const PropertyList& properties;

		// Return std::string value of the property
		const StringFunction stringF;

		// Return double value of the property
		const NumberFunction numberF;

		// Compares two items
		const SorterFunction customSorterF;

		// Returns JSON for special properties
		const CustomPropertySerializer jsonF;

		// Returns true if the item matches filter
		const CustomFilterFunction customFilterF;
	};
}

#endif