/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARIO *ICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean

   Template for new SQ_One sequence class

*/

#ifndef _REDEMPTION_LIBS_SQ_OUTFILENAME_H_
#define _REDEMPTION_LIBS_SQ_OUTFILENAME_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rio.h"


extern "C" {
    struct SQOutfilename {
        int fd;
        RIO * trans;
        timeval start_tv;
        timeval stop_tv;
        RIO * tracker;
        SQ_FORMAT format;
        char prefix[512];
        char extension[12];
        unsigned pid;
        unsigned count;
    };

    static inline RIO_ERROR sq_m_SQOutfilename_constructor(SQOutfilename * self, SQ_FORMAT format, const char * prefix, const char * extension)
    {
        self->trans = NULL;
        self->count = 0;
        self->format = format;
        self->pid = getpid();
        if (strlen(prefix) > sizeof(self->prefix) - 1){
            return RIO_ERROR_STRING_PREFIX_TOO_LONG;
        }
        strcpy(self->prefix, prefix);
        if (strlen(extension) > sizeof(self->extension) - 1){
            return RIO_ERROR_STRING_EXTENSION_TOO_LONG;
        }
        strcpy(self->extension, extension);
        return RIO_ERROR_OK;
    }

    // internal utility method, used to get name of files used for target transports
    // it is called internally, but actual goal is to enable tests to check and remove the created files afterward.
    // not a part of external sequence API
    static inline size_t sq_im_SQOutfilename_get_name(const SQOutfilename * self, char * buffer, size_t size, int count)
    {
        size_t res = 0;
        switch (self->format){
        default:
        case SQF_PREFIX_PID_COUNT_EXTENSION:
            res = snprintf(buffer, size, "%s-%06u-%06u%s", self->prefix, self->pid, count, self->extension);
        break;
        case SQF_PREFIX_COUNT_EXTENSION:
            res = snprintf(buffer, size, "%s-%06u%s", self->prefix, count, self->extension);
        break;
        case SQF_PREFIX_PID_EXTENSION:
            res = snprintf(buffer, size, "%s-%06u%s", self->prefix, self->pid, self->extension);
        break;
        case SQF_PREFIX_EXTENSION:
            res = snprintf(buffer, size, "%s%s", self->prefix, self->extension);
        break;
        }
        return res;
    }

    static RIO_ERROR sq_m_SQOutfilename_timestamp(SQOutfilename * self, timeval * tv)
    {
        return RIO_ERROR_OK;
    }


    static inline RIO_ERROR sq_m_SQOutfilename_destructor(SQOutfilename * self)
    {
        if (self->trans){
            TODO("check if close returns some error");
            rio_delete(self->trans);
            close(self->fd);
            self->trans = NULL;
        }
        return RIO_ERROR_OK;
    }

    static inline RIO * sq_m_SQOutfilename_get_trans(SQOutfilename * self, RIO_ERROR * status)
    {
        if (status && (*status != RIO_ERROR_OK)) { return self->trans; }
        if (!self->trans){
            char tmpname[1024];
            sq_im_SQOutfilename_get_name(self, tmpname, sizeof(tmpname), self->count);
            self->fd = ::open(tmpname, O_WRONLY|O_CREAT, S_IRUSR|S_IRUSR);
            if (self->fd < 0){
                if (status) { *status = RIO_ERROR_CREAT; }
                return self->trans;
            }
            self->trans = rio_new_outfile(status, self->fd);
        }
        return self->trans;
    }

    static inline RIO_ERROR sq_m_SQOutfilename_next(SQOutfilename * self)
    {
        sq_m_SQOutfilename_destructor(self);
        self->count += 1;
        return RIO_ERROR_OK;
    }

    static inline RIO_ERROR sq_m_SQOutfilename_get_chunk_info(SQOutfilename * self, unsigned * num_chunk, char * path, size_t path_len, timeval * begin, timeval * end)
    {
        return RIO_ERROR_OK;
    }
};

#endif
