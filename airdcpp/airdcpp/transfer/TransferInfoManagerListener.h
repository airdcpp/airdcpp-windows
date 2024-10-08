/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_TRANSFERINFOMANAGER_LISTENER_H_
#define DCPLUSPLUS_DCPP_TRANSFERINFOMANAGER_LISTENER_H_

#include <airdcpp/forward.h>
#include <airdcpp/core/header/typedefs.h>

#include <airdcpp/transfer/TransferInfo.h>

namespace dcpp {

class TransferInfoManagerListener {
public:
	virtual ~TransferInfoManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Added;
	typedef X<1> Updated;
	typedef X<2> Removed;
	typedef X<3> Failed;
	typedef X<4> Starting;
	typedef X<5> Completed;
	typedef X<6> Tick;

	virtual void on(Added, const TransferInfoPtr&) noexcept { }
	virtual void on(Updated, const TransferInfoPtr&, int, bool) noexcept {}
	virtual void on(Removed, const TransferInfoPtr&) noexcept { }
	virtual void on(Failed, const TransferInfoPtr&) noexcept { }
	virtual void on(Starting, const TransferInfoPtr&) noexcept { }
	virtual void on(Completed, const TransferInfoPtr&) noexcept { }
	virtual void on(Tick, const TransferInfo::List&, int) noexcept { }
};
} // namespace dcpp

#endif /*UPLOADMANAGERLISTENER_H_*/
