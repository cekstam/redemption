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
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan
 */

#ifndef REDEMPTION_TRANSPORT_META_SEQUENCE_TRANSPORT_HPP
#define REDEMPTION_TRANSPORT_META_SEQUENCE_TRANSPORT_HPP

#include "detail/filename_sequence_policy.hpp"
#include "detail/meta_writer.hpp"
#include "buffer_transport.hpp"
#include "buffer/input_output_buf.hpp"
#include "buffer/file_buf.hpp"
#include "sequence_generator.hpp"

#include <limits>
#include <cstdio>
#include <cerrno>
#include <ctime>

namespace detail
{
    template<class Buf>
    struct out_meta_nexter
    : public Buf
    {
        out_meta_nexter(time_t sec) /*noexcept*/
        : start_sec(sec)
        , stop_sec(sec)
        {}

        template<class TransportBuf>
        int next(TransportBuf & buf) /*noexcept*/
        {
            if (buf.is_open()) {
                buf.close();
                const char * filename = this->current_path(buf);
                ssize_t len = strlen(filename);
                ssize_t res = this->write(filename, len);
                if (res >= 0 && res == len) {
                    char mes[(std::numeric_limits<unsigned>::digits10 + 1) * 2 + 4];
                    len = std::sprintf(mes, " %u %u\n", (unsigned)this->start_sec, (unsigned)this->stop_sec+1);
                    res = this->write(mes, len);
                    if (res == len) {
                        res = 0;
                        this->start_sec = this->stop_sec;
                    }
                }
                return res;
            }
            return 1;
        }

        template<class TransportBuf>
        bool next_end(TransportBuf & buf) /*noexcept*/
        {
            return this->next(buf);
        }

        template<class TransportBuf>
        /*constexpr*/ const char * current_path(TransportBuf & buf) const /*noexcept*/
        {
            return buf.policy().seqgen().get(buf.policy().seqnum());
        }

        void update_sec(time_t sec) /*noexcept*/
        { this->stop_sec = sec; }

    private:
        time_t start_sec;
        time_t stop_sec;
    };
}


#include "detail/out_meta_sequence_transport_base.hpp"

struct OutMetaSequenceTransport
: detail::OutMetaSequenceTransportBase<
    transbuf::output_buf<io::posix::fdbuf, detail::FilenameSequencePolicy>,
    detail::out_meta_nexter<transbuf::ofile_base>
>
{
    OutMetaSequenceTransport(
        const char * path,
        const char * basename,
        timeval now,
        uint16_t width,
        uint16_t height,
        const int groupid,
        auth_api * authentifier = NULL,
        unsigned verbose = 0,
        FilenameFormat format = FilenameGenerator::PATH_FILE_PID_COUNT_EXTENSION)
    : OutMetaSequenceTransport::OutMetaSequenceBase(
        detail::FilenameSequencePolicyParams(format, path, basename, ".wrm", groupid),
        now.tv_sec,
        path, basename, format, width, height, authentifier, verbose)
    {}
};

#endif