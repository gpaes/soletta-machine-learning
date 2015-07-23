/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sml_matrix.h"
#include "sml_log.h"
#include "sml_string.h"
#include <string.h>
#define max(a, b) ( a > b ? a : b )
#define BUF_LEN (50)

inline static struct sol_vector *
_get_line(struct sol_vector *v, uint16_t pos, uint16_t elem_size)
{
    uint16_t i;

    for (i = v->len; i <= pos; i++) {
        struct sol_vector *line = sol_vector_append(v);
        sol_vector_init(line, elem_size);
    }
    return sol_vector_get(v, pos);
}

inline static void *
_get_column(struct sol_vector *line, uint16_t pos, uint16_t elem_size)
{
    uint16_t i;

    for (i = line->len; i <= pos; i++) {
        void *c = sol_vector_append(line);
        if (!c)
            return NULL;
        memset(c, 0, elem_size);
    }
    return sol_vector_get(line, pos);
}

void
sml_matrix_init(struct sml_matrix *m, uint16_t elem_size)
{
    sol_vector_init(&m->data, sizeof(struct sol_vector));
    m->elem_size = elem_size;
}

void
sml_matrix_clear(struct sml_matrix *m)
{
    uint16_t i;
    struct sol_vector *line;

    SOL_VECTOR_FOREACH_IDX (&m->data, line, i)
        sol_vector_clear(line);

    sol_vector_clear(&m->data);
}

void *
sml_matrix_insert(struct sml_matrix *m, uint16_t i, uint16_t j)
{
    struct sol_vector *line = _get_line(&m->data, i, m->elem_size);

    if (!line)
        return NULL;
    return _get_column(line, j, m->elem_size);
}

void *
sml_matrix_get(struct sml_matrix *m, uint16_t i, uint16_t j)
{
    struct sol_vector *line = sol_vector_get(&m->data, i);

    if (!line)
        return NULL;
    return sol_vector_get(line, j);
}

bool
sml_matrix_equals(struct sml_matrix *m1, struct sml_matrix *m2, struct
    sol_vector *changed,
    sml_cmp_cb cmp_cb)
{
    uint16_t i, j, len_i, len_j, *idx;
    struct sol_vector *vec1, *vec2;
    bool r = false;

    len_i = max(m1->data.len, m2->data.len);
    for (i = 0; i < len_i; i++) {
        vec1 = sol_vector_get(&m1->data, i);
        vec2 = sol_vector_get(&m2->data, i);
        len_j = max((vec1 ? vec1->len : 0), (vec2 ? vec2->len : 0));
        for (j = 0; j < len_j; j++) {
            if (!cmp_cb(sol_vector_get(vec1, j),
                sol_vector_get(vec2, j))) {
                if (changed) {
                    idx = sol_vector_append(changed);
                    if (!idx) {
                        sml_critical("Could not append the index:%d to the" \
                            " changed vector", i);
                        return false;
                    }
                    *idx = i;
                }
                r = true;
                break;
            }
        }
    }

    return r;
}

uint16_t
sml_matrix_lines(struct sml_matrix *m)
{
    return m->data.len;
}

uint16_t
sml_matrix_cols(struct sml_matrix *m, uint16_t i)
{
    struct sol_vector *line = sol_vector_get(&m->data, i);

    if (!line)
        return 0;

    return line->len;
}

void
sml_matrix_remove_line(struct sml_matrix *m, uint16_t line_num)
{
    struct sol_vector *line;

    line = sol_vector_get(&m->data, line_num);
    if (!line)
        return;

    sol_vector_clear(line);
    sol_vector_del(&m->data, line_num);
}

void
sml_matrix_remove_col(struct sml_matrix *m, uint16_t line_num, uint16_t col_num)
{
    struct sol_vector *line;

    line = sol_vector_get(&m->data, line_num);
    if (!line)
        return;

    sol_vector_del(line, col_num);
}

void
sml_matrix_debug(struct sml_matrix *m, sml_matrix_convert_cb convert)
{
    struct sml_string *str = sml_string_new("\t");
    uint16_t i, len_i, j, len_j;
    struct sol_vector *vec;
    char buf[BUF_LEN];

    sml_string_append(str, "{");
    len_i = m->data.len;
    for (i = 0; i < len_i; i++) {
        vec = sol_vector_get(&m->data, i);
        if (i > 0)
            sml_string_append(str, ", ");

        sml_string_append(str, "{");
        len_j = vec ? vec->len : 0;
        for (j = 0; j < len_j; j++) {
            if (j > 0)
                sml_string_append(str, ", ");

            convert(buf, BUF_LEN, sml_matrix_get(m, i, j));
            sml_string_append_printf(str, "%s", buf);
        }
        sml_string_append(str, "}");
    }
    sml_string_append(str, "}");
    sml_debug("%s", sml_string_get_string(str));

    sml_string_free(str);
}
