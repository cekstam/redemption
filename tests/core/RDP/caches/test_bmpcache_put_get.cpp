/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan
 */

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestBmpCachePutGet
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
//#define LOGPRINT

#include "RDP/caches/bmpcache.hpp"

BOOST_AUTO_TEST_CASE(BmpcachePutAndGet)
{
    BmpCache bmpcache(BmpCache::Mod_rdp, 24, 1, false, BmpCache::CacheOption(256, 1024, true));
    {
        Bitmap bmp("no_image");
        BOOST_CHECK(bmp.is_valid());
        bmpcache.put(0, RDPBmpCache::BITMAPCACHE_WAITING_LIST_INDEX, bmp, ~0, ~0);
    }
    {
        const Bitmap & bmp = bmpcache.get(0, RDPBmpCache::BITMAPCACHE_WAITING_LIST_INDEX);
        BOOST_CHECK(bmp.is_valid());
    }
}

