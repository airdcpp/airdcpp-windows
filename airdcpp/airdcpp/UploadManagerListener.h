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

#ifndef DCPLUSPLUS_DCPP_UPLOADMANAGERLISTENER_H_
#define DCPLUSPLUS_DCPP_UPLOADMANAGERLISTENER_H_

#include "forward.h"
#include "typedefs.h"

namespace dcpp {

class UploadManagerListener {
	friend class UploadQueueItem;
public:
	virtual ~UploadManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> QueueAdd;
	typedef X<5> QueueRemove;
	typedef X<6> QueueItemRemove;
	typedef X<7> QueueUpdate;

	typedef X<8> Created;
	typedef X<9> Removed;

	typedef X<11> SlotsUpdated; //Added / removed reserved slot

	virtual void on(Starting, const Upload*) noexcept { }
	virtual void on(Tick, const UploadList&) noexcept { }
	virtual void on(Complete, const Upload*) noexcept { }
	virtual void on(Failed, const Upload*, const string&) noexcept { }

	virtual void on(QueueAdd, UploadQueueItem*) noexcept { }
	virtual void on(QueueRemove, const UserPtr&) noexcept { }
	virtual void on(QueueItemRemove, UploadQueueItem*) noexcept { }
	virtual void on(QueueUpdate) noexcept { }

	virtual void on(Created, Upload*) noexcept { }
	virtual void on(Removed, const Upload*) noexcept { }

	virtual void on(SlotsUpdated, const UserPtr&) noexcept { }
};

} // namespace dcpp

#endif /*UPLOADMANAGERLISTENER_H_*/
