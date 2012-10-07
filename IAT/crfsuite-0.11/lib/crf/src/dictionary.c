/*
 *      Implementation of dictionary.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id: dictionary.c 176 2010-07-14 09:31:04Z naoaki $ */

#include <os.h>

#include <stdlib.h>
#include <string.h>

#include <crfsuite.h>
#include "quark.h"

static int dictionary_addref(crf_dictionary_t* dic)
{
    return crf_interlocked_increment(&dic->nref);
}

static int dictionary_release(crf_dictionary_t* dic)
{
    int count = crf_interlocked_decrement(&dic->nref);
    if (count == 0) {
        quark_t *qrk = (quark_t*)dic->internal;
        quark_delete(qrk);
        free(dic);
    }
    return count;
}

static int dictionary_get(crf_dictionary_t* dic, const char *str)
{
    quark_t *qrk = (quark_t*)dic->internal;
    return quark_get(qrk, str);
}

static int dictionary_to_id(crf_dictionary_t* dic, const char *str)
{
    quark_t *qrk = (quark_t*)dic->internal;
    return quark_to_id(qrk, str);    
}

static int dictionary_to_string(crf_dictionary_t* dic, int id, char const **pstr)
{
    quark_t *qrk = (quark_t*)dic->internal;
    const char *str = quark_to_string(qrk, id);
    if (str != NULL) {
        char *dst = (char*)malloc(strlen(str)+1);
        if (dst) {
            strcpy(dst, str);
            *pstr = dst;
            return 0;
        }
    }
    return 1;
}

static int dictionary_num(crf_dictionary_t* dic)
{
    quark_t *qrk = (quark_t*)dic->internal;
    return quark_num(qrk);
}

static void dictionary_free(crf_dictionary_t* dic, const char *str)
{
    free((char*)str);
}

int crf_dictionary_create_instance(const char *interface, void **ptr)
{
    if (strcmp(interface, "dictionary") == 0) {
        crf_dictionary_t* dic = (crf_dictionary_t*)calloc(1, sizeof(crf_dictionary_t));

        if (dic != NULL) {
            dic->internal = quark_new();
            dic->nref = 1;
            dic->addref = dictionary_addref;
            dic->release = dictionary_release;
            dic->get = dictionary_get;
            dic->to_id = dictionary_to_id;
            dic->to_string = dictionary_to_string;
            dic->num = dictionary_num;
            dic->free = dictionary_free;
            *ptr = dic;
            return 0;
        } else {
            return -1;
        }
    } else {
        return 1;
    }
}
