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

#ifndef DCPLUSPLUS_HTTPLINKS_H_
#define DCPLUSPLUS_HTTPLINKS_H_


#include <airdcpp/typedefs.h>

namespace dcpp {
	class SimpleXML;
}

namespace wingui {


class HttpLinks {
public:
	static tstring homepage;
	static tstring github;
	static tstring downloads;
	static tstring guides;
	static tstring discuss;
	static tstring bugs;

	static tstring extensionHomepageBase;
	static tstring extensionCatalog;
	static tstring extensionPackageBase;
	static tstring webServerHelp;
	static tstring extensionsDevHelp;

	static tstring ipSearchBase;
	static tstring speedTest;
	static tstring appCrash;
	static tstring userCommandsHelp;
	static tstring timeVariablesHelp;

	static void updateLinks(SimpleXML& xml);
};

}

#endif // DCPLUSPLUS_HTTPLINKS_H_