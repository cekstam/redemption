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

#ifndef REDEMPTION_UTILS_SOCKET_TRANSPORT_UTILITY_HPP
#define REDEMPTION_UTILS_SOCKET_TRANSPORT_UTILITY_HPP

#include "socket_transport.hpp"
#include "wait_obj.hpp"

inline
void add_to_fd_set(wait_obj & w, SocketTransport * t, fd_set & rfds, unsigned & max, timeval & timeout)
{
    if (t && t->sck > 0){
        FD_SET(t->sck, &rfds);
        max = ((unsigned)t->sck > max)?t->sck:max;
    }
    if ((!t || t->sck <= 0 || w.object_and_time) && w.set_state) {
        struct timeval now;
        now = tvtime();
        timeval remain = how_long_to_wait(w.trigger_time, now);
        if (lessthantimeval(remain, timeout)){
            timeout = remain;
        }
    }
}

inline
bool is_set(wait_obj & w, SocketTransport * t, fd_set & rfds)
{
    w.waked_up_by_time = false;

    if (t && t->sck > 0) {
        bool res = FD_ISSET(t->sck, &rfds);

        if (res || !w.object_and_time) {
            return res;
        }
    }

    if (w.set_state) {
        if (tvtime() >= w.trigger_time) {
            w.waked_up_by_time = true;
            return true;
        }
    }

    return false;
}

#endif
