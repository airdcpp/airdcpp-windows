// MakeDefs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <airdcpp/SimpleXML.h>
#include <airdcpp/File.h>
#include <airdcpp/StringTokenizer.h>
#include <airdcpp/ResourceManager.h>
#include <airdcpp/version.h>

SettingsManager* Singleton<SettingsManager>::instance = 0;

string ResourceManager::strings[];
wstring ResourceManager::wstrings[];
ResourceManager* Singleton<ResourceManager>::instance;

int __cdecl main(int argc, char* argv[])
{
	if(argc < 3) {
		return 0;
	}
	
	try {
		//string tmp;
		File src(argv[1], File::READ, File::OPEN, File::BUFFER_SEQUENTIAL, false);
		File tgt(argv[2], File::WRITE, File::CREATE | File::TRUNCATE, File::BUFFER_AUTO, false);
		File example(argv[3], File::WRITE, File::CREATE | File::TRUNCATE, File::BUFFER_AUTO, false);
		string x = src.read();
		x = Text::acpToUtf8(x);
		string::size_type k;
		
		while((k = x.find('\r')) != string::npos) {
			x.erase(k, 1);
		}

		StringList l = StringTokenizer<string>(x, '\n').getTokens();

		StringIter i;
		string varStr;
		string varName;
		string start;

		SimpleXML ex;

		for(i = l.begin(); i != l.end(); ) {
			if( (k = i->find("// @Strings: ")) != string::npos) {
				varStr = i->substr(k + 13);
				i = l.erase(i);
			} else if( (k = i->find("// @Names: ")) != string::npos) {
				varName = i->substr(k + 11);
				i = l.erase(i);
			} else if(i->find("// @DontAdd") != string::npos) {
				i = l.erase(i);
			} else if( (k = i->find("// @Prolog: ")) != string::npos) {
				start += i->substr(k + 12) + "\r\n";
				i = l.erase(i);
			} else if(i->size() < 5) {
				i = l.erase(i);
			} else {
				++i;
			}
		}

		if(varStr.empty() || varName.empty()) {
			printf("No @Strings or @Names\n");
			return 0;
		}
		
		varStr += " = {\r\n";
		varName += " = {\r\n";

		ex.addTag("resources");
		ex.stepIn();
		string name;
		string def;
		string xmldef;
		string s;
		for(i = l.begin(); i != l.end(); i++) {

			name.clear();
			s = *i;

			bool u = true;
			for(k = s.find_first_not_of(" \t"); s[k] != ','; k++) {
				if(s[k] == '_') {
					u = true;
				} else if(u) {
					name+=s[k];
					u = false;
				} else {
					name+=(char)tolower(s[k]);
				}
			}

			k = s.find("// ");
			def = s.substr(k + 3);
			xmldef = def.substr(1, def.size() - 2);
			while( (k = xmldef.find("\\t")) != string::npos) {
				xmldef.replace(k, 2, "\t");
			}
			while( (k = xmldef.find("\\r")) != string::npos) {
				xmldef.replace(k, 2, "\r");
			}
			while( (k = xmldef.find("\\n")) != string::npos) {
				xmldef.replace(k, 2, "\n");
			}

			while( (k = xmldef.find("\\\\")) != string::npos) {
				xmldef.replace(k, 2, "\\");
			}
			while( (k = xmldef.find("\\\"")) != string::npos) {
				xmldef.replace(k, 2, "\"");
			}

			ex.addTag("string", xmldef);
			ex.addChildAttrib("name", name);

			varStr += def + ", \r\n";
			varName += '\"' + name + "\", \r\n";
		}

		varStr.erase(varStr.size()-2, 2);
		varName.erase(varName.size()-2, 2);

		varStr += "\r\n};\r\n";
		varName += "\r\n};\r\n";

		tgt.write(start);
		tgt.write(varStr);
		tgt.write(varName);

		example.write(SimpleXML::utf8Header);
		example.write(ex.toXML());
	} catch(Exception e) {
		printf("%s\n", e.getError().c_str());
	}

	return 0;
}
