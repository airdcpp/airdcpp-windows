/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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

#ifndef DCPLUSPLUS_DISPATCHERS_H_
#define DCPLUSPLUS_DISPATCHERS_H_

#include <functional>

namespace dcpp {

namespace Dispatchers {

	template<typename T>
	struct Base {
		typedef std::function<T> F;

		Base(const F& f_) : f(f_) { }

	protected:
		F f;
	};

	template<LRESULT value = 0, bool handled = true>
	struct VoidVoid : public Base<void ()> {

		VoidVoid(const F& f_) : Base<void ()>(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			this->f();
			ret = value;
			return handled;
		}
	};

}

typedef Dispatchers::VoidVoid<> Dispatcher;

}

#endif