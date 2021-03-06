/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2014
*   Author(s): Jonathan Poelen
*/

#ifndef REDEMPTION_UTILS_FINALLY_HPP
#define REDEMPTION_UTILS_FINALLY_HPP

template<class F>
struct basic_finally {
    F f;
    ~basic_finally() { f(); }
};

template<class F>
basic_finally<F> finally(F f) {
    return {static_cast<F&&>(f)};
}

template<class F, class Finally>
auto try_finally(F f, Finally finally) -> decltype(f()) {
    basic_finally<Finally&> d{finally};
    return f();
}

template<class F, class Finally>
auto except_try_finally(F f, Finally finally) -> decltype(f()) {
    decltype(f()) ret;
    try {
        ret = f();
    }
    catch (...) {
        finally();
        throw;
    }
    finally();
    return ret;
}

#endif
