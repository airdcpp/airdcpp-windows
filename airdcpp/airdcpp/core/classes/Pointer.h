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

#ifndef DCPLUSPLUS_DCPP_POINTER_H
#define DCPLUSPLUS_DCPP_POINTER_H

#include <boost/intrusive_ptr.hpp>

namespace dcpp {

template<typename T>
class intrusive_ptr_base
{
public:
	void inc() noexcept {
		intrusive_ptr_add_ref(this);
	}

	void dec() noexcept {
		intrusive_ptr_release(this);
	}

	bool unique(int val = 1) const noexcept {
		return (ref <= val);
	}

	int getRefs() const noexcept {
		return ref;
	}
	
protected:
	intrusive_ptr_base() noexcept : ref(0) { }
	virtual ~intrusive_ptr_base() { }
	
private:
	friend void intrusive_ptr_add_ref(intrusive_ptr_base* p) { ++p->ref; }

#if defined (__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
// warning: �long unsigned int __atomic_sub_fetch_8(volatile void*, long unsigned int, int)� writing 8 bytes into a region of size 0 overflows the destination
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107694
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
	friend void intrusive_ptr_release(intrusive_ptr_base* p) { if(--p->ref == 0) { delete static_cast<T*>(p); } }
#if defined (__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif
	atomic<long> ref;
};


struct DeleteFunction {
	template<typename T>
	void operator()(const T& p) const { delete p; }
};

} // namespace dcpp

#endif // !defined(POINTER_H)