/*
	free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef NUTILSCMAXMINDDB_H
#define NUTILSCMAXMINDDB_H
#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream> 
#include <maxminddb.h>

#include "Singleton.h"

using namespace std;

using std::string;

namespace dcpp {

		class cMaxMindDB
		{
			public:
				cMaxMindDB();
				~cMaxMindDB();
				void ReloadAll();
				bool GetCC(const string &host, string &cc);
				bool GetCN(const string &host, string &cn);
			private:
				::MMDB_s *mDBCO;
				::MMDB_s *TryCountryDB(unsigned int flags);
				bool FileExists(const char *name);
				unsigned long Ip2Num(const string &ip)
				{
					int i;
					char c;
					std::istringstream is(ip);
					unsigned long mask = 0;
					is >> i >> c; mask += i & 0xFF; mask <<= 8;
					is >> i >> c; mask += i & 0xFF; mask <<= 8;
					is >> i >> c; mask += i & 0xFF; mask <<= 8;
					is >> i; mask += i & 0xFF;
					return mask;
				}

				void Num2Ip(unsigned long mask, string &ip)
				{
					std::ostringstream os;
					unsigned char *i = (unsigned char *)&mask;
					os << int(i[3]) << ".";
					os << int(i[2]) << ".";
					os << int(i[1]) << ".";
					os << int(i[0]);
					ip = os.str();
				}
		};
};
#endif
