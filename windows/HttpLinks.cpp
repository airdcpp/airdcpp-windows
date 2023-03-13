/*
 * Copyright (C) 2012-2023 AirDC++ Project
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

#include "stdafx.h"

#include "HttpLinks.h"

#include <airdcpp/SimpleXML.h>

namespace dcpp {

tstring HttpLinks::homepage = _T("https://www.airdcpp.net/");
tstring HttpLinks::github = _T("https://github.com/airdcpp/airdcpp-windows/");
tstring HttpLinks::downloads = HttpLinks::homepage + _T("download/");
tstring HttpLinks::guides = HttpLinks::github + _T("wiki/");
tstring HttpLinks::discuss = HttpLinks::github + _T("discussions/");
tstring HttpLinks::bugs = HttpLinks::github + _T("issues/");

tstring HttpLinks::extensionHomepageBase = _T("https://www.npmjs.com/package/");
tstring HttpLinks::extensionCatalog = _T("https://registry.npmjs.org/-/v1/search?text=keywords:airdcpp-extensions-public&size=100");
tstring HttpLinks::extensionPackageBase = _T("https://registry.npmjs.org/");

tstring HttpLinks::ipSearchBase = _T("https://apps.db.ripe.net/db-web-ui/query?form_type=simple&full_query_string=&searchtext=");
tstring HttpLinks::speedTest = _T("https://www.speedtest.net");
tstring HttpLinks::appCrash = _T("http://crash.airdcpp.net");

tstring HttpLinks::webServerHelp = HttpLinks::github + _T("wiki/Web-server/");
tstring HttpLinks::extensionsDevHelp = _T("https://airdcpp-web.github.io/docs/advanced/scripting.html");
tstring HttpLinks::userCommandsHelp = _T("https://dcplusplus.sourceforge.net/webhelp/dialog_user_command.html");
tstring HttpLinks::timeVariablesHelp = _T("https://cplusplus.com/reference/ctime/strftime");


#define LINK(name, name2) \
	if(xml.findChild(#name2)) { \
		HttpLinks::##name = Text::toT(xml.getChildData()); \
	} \
	xml.resetCurrentChild();

void HttpLinks::updateLinks(SimpleXML& xml) {
	xml.resetCurrentChild();
	if (xml.findChild("Links")) {
		xml.stepIn();

		// General
		LINK(homepage, Homepage);
		LINK(downloads, Downloads);
		LINK(discuss, Forum);
		LINK(bugs, Bugs);
		LINK(guides, Guides);

		// Extensions/web server
		LINK(extensionHomepageBase, ExtensionHomepage);
		LINK(extensionCatalog, ExtensionCatalog);
		LINK(extensionPackageBase, ExtensionPackage);
		LINK(webServerHelp, WebServerHelp);
		LINK(extensionsDevHelp, ExtensionsDevHelp);

		// Help/misc
		LINK(ipSearchBase, IPSearchBase);
		LINK(speedTest, Speedtest);
		LINK(appCrash, AppCrash);
		LINK(userCommandsHelp, UserCommandsHelp);
		LINK(timeVariablesHelp, TimeVariablesHelp);

		xml.stepOut();
	}
}

}