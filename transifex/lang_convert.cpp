/*
* Copyright (C) 2013 AirDC++ Project
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

#include "../client/stdinc.h"

#include <iostream>
#include <string>

#include "../client/typedefs.h"
#include "../client/File.h"
#include "../client/SimpleXML.h"
#include "../client/Util.h"

using namespace std;

struct Strings {
	string toFind;
	string replaceWith;
};

Strings arr[] {
	{ "<?xml version=\'1.0\' encoding=\'UTF-8\'?>", "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>" }, 
	{ "<resources>",		"<Language Name=\"Lang\" Author=\"FlylinkDC++ Development Team\" Revision=\"1\">\n	<Strings>" }, 
	{ "  <string name",		"<String Name" }, 
	{ "</string>",			"</String>" },
	{ "</resources>",		"	</Strings>\n</Language>" },
	{ "\\t",				"\t" },
	{ "\\r",				"" },
	{ "\\n",				"\n" },
	{ "\\\'",				"\'" },
	{ "\\\"",				"\"" },
	{ "\\\\",				"\\" }

};

int getVersion(dcpp::SimpleXML& xml) {
	if (xml.findChild("Language")) {
		//xml.stepIn();
		auto version = xml.getIntChildAttrib("Version");
		return version;
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
  //cout << endl << "Use: lang_convert.exe ru-RU.xml" << endl;
	if (argc != 3) {
		cout << endl << "Usage: lang_convert <source path> <target path>" << endl;
		return 0;
	}

    string sourcePath = argv[1];
	string tempPath = sourcePath + "converted" + PATH_SEPARATOR;
	string targetPath = argv[2];

	dcpp::File::ensureDirectory(tempPath);
	dcpp::File::ensureDirectory(targetPath);

	auto files = dcpp::File::findFiles(sourcePath, "*.xml");
	for (const auto& p : files) {
		//cout << p + " found" << endl;
		auto fileName = dcpp::Util::getFileName(p);
		auto cur = dcpp::File(p, dcpp::File::READ, dcpp::File::OPEN).read();

		try {
			auto old = dcpp::File(tempPath + fileName, dcpp::File::READ, dcpp::File::OPEN).read();
			if (cur == old) {
				cout << p + " is the same" << endl;
				continue;
			}
		} catch (dcpp::FileException&) {
			cout << "Old comparison file doesn't exist, creating new" << endl;
		}

		cout << p + " was updated" << endl;
		auto target = targetPath + fileName;

		try {
			dcpp::File::copyFile(p, tempPath + fileName);
		} catch (dcpp::FileException&) {
			cout << "Failed to copy the comparison file" << endl;
		}

		for (const auto& r : arr) {
			// replace
			int i = cur.find(r.toFind);
			while (i != string::npos) {
				cur.replace(i, r.toFind.length(), r.replaceWith);
				//cout << "||||||||||||||| " << j->first << "  ---------->  " << j->second << endl;
				i = cur.find(r.toFind);
			}
		}

		int version = 0;
		// pick current version from the target location
		try {
			dcpp::SimpleXML targetXml;
			targetXml.fromXML(dcpp::File(target, dcpp::File::READ, dcpp::File::OPEN).read());
			version = getVersion(targetXml);
		} catch (dcpp::FileException&) {
			// not relevant...
			cout << "The target doesn't exist, creating new" << endl;
		}

		// append the version in out file
		dcpp::SimpleXML xml;
		xml.fromXML(cur);

		getVersion(xml); // move to the current location
		xml.replaceChildAttrib("Version", dcpp::Util::toString(version + 1));

		try {
			dcpp::File f(target, dcpp::File::WRITE, dcpp::File::CREATE | dcpp::File::TRUNCATE);
			f.write(dcpp::SimpleXML::utf8Header);
			f.write(xml.toXML());
			f.close();
		} catch (dcpp::FileException&) {
			cout << "Failed to write into target location" << endl;
		}
	}

	return 0;
}
