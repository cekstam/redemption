/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#include <zlib.h>

#include "transport.hpp"

#ifndef REDEMPTION_TRANSPORT_GZIP_COMPRESSION_TRANSPORT_HPP
#define REDEMPTION_TRANSPORT_GZIP_COMPRESSION_TRANSPORT_HPP

static const size_t GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH = 1024 * 64;

/*****************************
* GZipCompressionInTransport
*/

class GZipCompressionInTransport : public Transport {
    Transport & source_transport;

    z_stream compression_stream;

    uint8_t * uncompressed_data;
    size_t    uncompressed_data_length;
    uint8_t   uncompressed_data_buffer[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];

    bool inflate_pending;

    uint32_t verbose;

public:
    GZipCompressionInTransport(Transport & st, uint32_t verbose = 0)
    : Transport()
    , source_transport(st)
    , compression_stream()
    , uncompressed_data(NULL)
    , uncompressed_data_length(0)
    , uncompressed_data_buffer()
    , inflate_pending(false)
    , verbose(verbose) {
        int ret = ::inflateInit(&this->compression_stream);
(void)ret;
    }

    virtual ~GZipCompressionInTransport() {
        ::inflateEnd(&this->compression_stream);
    }

private:
    virtual void do_recv(char ** pbuffer, size_t len) {
        uint8_t * temp_data        = reinterpret_cast<uint8_t *>(*pbuffer);
        size_t    temp_data_length = len;

        while (temp_data_length) {
            if (this->uncompressed_data_length) {
                REDASSERT(this->uncompressed_data);

                const size_t data_length = std::min<size_t>(temp_data_length, this->uncompressed_data_length);

                ::memcpy(temp_data, this->uncompressed_data, data_length);

                this->uncompressed_data        += data_length;
                this->uncompressed_data_length -= data_length;

                temp_data        += data_length;
                temp_data_length -= data_length;
            }
            else {
                BStream data_stream(GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH);

                if (!this->inflate_pending) {
                    this->source_transport.recv(
                          &data_stream.end
                        , 3                 // reset_decompressor((1) + compressed_data_length(2)
                        );

                    if (data_stream.in_uint8() == 1) {
                        REDASSERT(this->inflate_pending == false);

                        if (this->verbose) {
                            LOG(LOG_INFO, "GZipCompressionInTransport::do_recv: Decompressor reset");
                        }
                        ::inflateEnd(&this->compression_stream);

                        ::memset(&this->compression_stream, 0, sizeof(this->compression_stream));

                        int ret = ::inflateInit(&this->compression_stream);
(void)ret;
                    }

                    const uint16_t compressed_data_length = data_stream.in_uint16_le();
                    if (this->verbose) {
                        LOG(LOG_INFO, "GZipCompressionInTransport::do_recv: compressed_data_length=%u", compressed_data_length);
                    }

                    data_stream.reset();

                    this->source_transport.recv(&data_stream.end, compressed_data_length);

                    this->compression_stream.avail_in = compressed_data_length;
                    this->compression_stream.next_in  = data_stream.get_data();
                }

                this->uncompressed_data = this->uncompressed_data_buffer;

                const size_t uncompressed_data_capacity = sizeof(this->uncompressed_data_buffer);

                this->compression_stream.avail_out = uncompressed_data_capacity;
                this->compression_stream.next_out  = this->uncompressed_data;

                int ret = ::inflate(&this->compression_stream, Z_NO_FLUSH);
                if (this->verbose & 0x2) {
                    LOG(LOG_INFO, "GZipCompressionInTransport::do_recv: inflate return %d", ret);
                }
(void)ret;

                if (this->verbose & 0x2) {
                    LOG( LOG_INFO, "GZipCompressionInTransport::do_recv: uncompressed_data_capacity=%u avail_out=%u"
                       , uncompressed_data_capacity, this->compression_stream.avail_out);
                }
                this->uncompressed_data_length = uncompressed_data_capacity - this->compression_stream.avail_out;
                if (this->verbose) {
                    LOG( LOG_INFO, "GZipCompressionInTransport::do_recv: uncompressed_data_length=%u"
                       , this->uncompressed_data_length);
                }

                this->inflate_pending = ((ret == 0) && (this->compression_stream.avail_out == 0));
            }
        }

        (*pbuffer) = (*pbuffer) + len;
    }
};  // class GZipCompressionInTransport


class GZipCompressionOutTransport : public Transport {
    Transport & target_transport;

    z_stream compression_stream;

    bool reset_compressor;

    static const size_t MAX_UNCOMPRESSED_DATA_LENGTH = 1000 * 64; // ((GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH - 6) * 16384 - 5 * 16383) / (5 + 16384) = 65505

    uint8_t uncompressed_data[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];
    size_t  uncompressed_data_length;

    uint32_t verbose;

public:
    GZipCompressionOutTransport(Transport & tt, uint32_t verbose = 0)
    : Transport()
    , target_transport(tt)
    , compression_stream()
    , reset_compressor(false)
    , uncompressed_data()
    , uncompressed_data_length(0)
    , verbose(verbose) {
        REDASSERT(MAX_UNCOMPRESSED_DATA_LENGTH <=
                  ((GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH - 6) * 16384 - 5 * 16383) / (5 + 16384));
        REDASSERT(MAX_UNCOMPRESSED_DATA_LENGTH <= 0xFFFF); // 0xFFFF (for uint16_t)

        int ret = ::deflateInit(&this->compression_stream, Z_DEFAULT_COMPRESSION);
(void)ret;
    }

    virtual ~GZipCompressionOutTransport() {
        if (this->uncompressed_data_length) {
            if (this->verbose & 0x4) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::~GZipCompressionOutTransport: Compress");
            }
            this->compress(this->uncompressed_data, this->uncompressed_data_length, true);

            this->uncompressed_data_length = 0;
        }

        ::deflateEnd(&this->compression_stream);
    }

private:
    void compress(const uint8_t * const data, size_t data_length, bool end) {
        if (this->verbose) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::compress: uncompressed_data_length=%u", data_length);
        }
        if (this->verbose & 0x4) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::compress: end=%s", (end ? "true" : "false"));
        }

        const int flush = (end ? Z_FINISH : Z_SYNC_FLUSH);

        this->compression_stream.avail_in = data_length;
        this->compression_stream.next_in  = const_cast<uint8_t *>(data);

        uint8_t compressed_data[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];

        do {
            this->compression_stream.avail_out = sizeof(compressed_data);
            this->compression_stream.next_out  = reinterpret_cast<unsigned char *>(compressed_data);

            int ret = ::deflate(&this->compression_stream, flush);
(void)ret;
            if (this->verbose & 0x2) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::compress: deflate return %d", ret);
            }
            REDASSERT(ret != Z_STREAM_ERROR);

            if (this->verbose & 0x2) {
                LOG( LOG_INFO, "GZipCompressionOutTransport::compress: compressed_data_capacity=%u avail_out=%u"
                   , sizeof(compressed_data), this->compression_stream.avail_out);
            }
            const size_t compressed_data_length = sizeof(compressed_data) - this->compression_stream.avail_out;

            BStream buffer_stream(128);

            buffer_stream.out_uint8(this->reset_compressor ? 1 : 0);
            this->reset_compressor = false;

            buffer_stream.out_uint16_le(compressed_data_length);
            if (this->verbose) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::compress: compressed_data_length=%u", compressed_data_length);
            }

            buffer_stream.mark_end();
            this->target_transport.send(buffer_stream);

            this->target_transport.send(compressed_data, compressed_data_length);
        }
        while (this->compression_stream.avail_out == 0);
        REDASSERT(this->compression_stream.avail_in == 0);
    }

    virtual void do_send(const char * const buffer, size_t len) {
        if (this->verbose & 0x4) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::do_send: len=%u", len);
        }

        const uint8_t * temp_data        = reinterpret_cast<const uint8_t *>(buffer);
        size_t          temp_data_length = len;

        while (temp_data_length) {
            if (this->uncompressed_data_length) {
                const size_t data_length = std::min<size_t>(
                      temp_data_length
                    , GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH - this->uncompressed_data_length
                    );

                ::memcpy(this->uncompressed_data + this->uncompressed_data_length, temp_data, data_length);

                this->uncompressed_data_length += data_length;

                temp_data += data_length;
                temp_data_length -= data_length;

                if (this->uncompressed_data_length == GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH) {
                    this->compress(this->uncompressed_data, this->uncompressed_data_length, false);

                    this->uncompressed_data_length = 0;
                }
            }
            else {
                if (temp_data_length >= GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH) {
                    this->compress(temp_data, GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH, false);

                    temp_data += GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH;
                    temp_data_length -= GZipCompressionOutTransport::MAX_UNCOMPRESSED_DATA_LENGTH;
                }
                else {
                    ::memcpy(this->uncompressed_data, temp_data, temp_data_length);

                    this->uncompressed_data_length = temp_data_length;

                    temp_data_length = 0;
                }
            }
        }

        if (this->verbose & 0x4) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::do_send: uncompressed_data_length=%u", this->uncompressed_data_length);
        }
    }

public:
    virtual bool next() {
        if (this->uncompressed_data_length) {
            if (this->verbose & 0x4) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::next: Compress");
            }
            this->compress(this->uncompressed_data, this->uncompressed_data_length, true);

            this->uncompressed_data_length = 0;
        }

        if (this->verbose) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::next: Compressor reset");
        }

        ::deflateEnd(&this->compression_stream);

        ::memset(&this->compression_stream, 0, sizeof(this->compression_stream));

        int ret = ::deflateInit(&this->compression_stream, Z_DEFAULT_COMPRESSION);
(void)ret;

        this->reset_compressor = true;

        return this->target_transport.next();
    }

    virtual void timestamp(timeval now) {
        this->target_transport.timestamp(now);
    }
};  // class GZipCompressionOutTransport

#endif  // #ifndef REDEMPTION_TRANSPORT_GZIP_COMPRESSION_TRANSPORT_HPP