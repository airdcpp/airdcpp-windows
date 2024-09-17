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

#ifndef DCPLUSPLUS_DISPATCHERS_H_
#define DCPLUSPLUS_DISPATCHERS_H_

#include <functional>

namespace wingui {

namespace Dispatchers {

	template<typename T>
	struct Base {
		using F = std::function<T>;

		explicit Base(F&& f_) : f(std::move(f_)) { }

	protected:
		F f;
	};

	template<LRESULT value = 0, bool handled = true>
	struct VoidVoid : public Base<void ()> {

		explicit VoidVoid(const F& f_) : Base<void ()>(f_) { }

		bool operator()(const MSG& msg, LRESULT& ret) const {
			this->f();
			ret = value;
			return handled;
		}
	};

}

using Dispatcher = Dispatchers::VoidVoid<>;

}

#endif