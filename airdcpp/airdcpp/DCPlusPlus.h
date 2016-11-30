/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_DCPLUSPLUS_H
#define DCPLUSPLUS_DCPP_DCPLUSPLUS_H

#include "compiler.h"
#include "typedefs.h"
#include "Exception.h"

namespace dcpp {

typedef function<void(const string&)> StepF;
typedef function<void(float)> ProgressF;
typedef function<void()> Callback;
typedef function<bool(const string& /*Message*/, bool /*isQuestion*/, bool /*isError*/)> MessageF;

// This should throw on fatal errors only (such as hash database initialization errors)
extern void startup(StepF stepF, MessageF messageF, Callback runWizard, ProgressF progressF, Callback moduleInitF = nullptr, Callback moduleLoadF = nullptr) throw(Exception);
extern void shutdown(StepF stepF, ProgressF progressF, Callback moduleDestroyF = nullptr);

} // namespace dcpp

#endif // !defined(DC_PLUS_PLUS_H)