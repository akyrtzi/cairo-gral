/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* Cairo - a vector graphics library with display and print output
 *
 * Copyright © 2009 Argiris Kirtzidis
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Argiris Kirtzidis.
 *
 * Contributor(s):
 *      Argiris Kirtzidis <akyrtzi@gmail.com>
 */

#include "cairo-gral-private.h"

static void
_cairo_gral_splines_fixed_add_buf (cairo_gral_splines_buffer_t *splines,
                                   cairo_gral_splines_buf_t    *buf)
{
  buf->next = NULL;
  buf->prev = splines->buf_tail;

  splines->buf_tail->next = buf;
  splines->buf_tail = buf;
}

static cairo_gral_splines_buf_t *
_cairo_gral_splines_buf_create (int buf_size)
{
  cairo_gral_splines_buf_t *buf;

  buf = _cairo_malloc_ab_plus_c (buf_size,
                                 sizeof (cairo_gral_spline_t),
                                 sizeof (cairo_gral_splines_buf_t));
  if (buf) {
    buf->next = NULL;
    buf->prev = NULL;
    buf->num_splines = 0;
    buf->buf_size = buf_size;

    buf->splines = (cairo_gral_spline_t *) (buf + 1);
  }

  return buf;
}

static void
_cairo_gral_splines_buf_destroy (cairo_gral_splines_buf_t *buf)
{
  free (buf);
}

void
_cairo_gral_splines_buffer_init (cairo_gral_splines_buffer_t *splines)
{
  splines->buf_head.base.next = NULL;
  splines->buf_head.base.prev = NULL;
  splines->buf_tail = &splines->buf_head.base;

  splines->buf_head.base.num_splines = 0;
  splines->buf_head.base.buf_size = CAIRO_GRAL_SPLINE_BUF_SIZE;
  splines->buf_head.base.splines = splines->buf_head.splines;
}

cairo_status_t
_cairo_gral_splines_buffer_add (cairo_gral_splines_buffer_t *splines_buf,
                                const cairo_gral_spline_t   *spline)
{
  cairo_gral_splines_buf_t *buf = splines_buf->buf_tail;

  if (buf->num_splines + 1 > buf->buf_size)
  {
    buf = _cairo_gral_splines_buf_create (buf->buf_size * 2);
    if (unlikely (buf == NULL))
      return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_gral_splines_fixed_add_buf (splines_buf, buf);
  }

  buf->splines[buf->num_splines++] = *spline;

  return CAIRO_STATUS_SUCCESS;
}

cairo_bool_t
_cairo_gral_splines_buffer_is_empty (cairo_gral_splines_buffer_t *splines_buf)
{
  return splines_buf->buf_head.base.num_splines == 0;
}

void
_cairo_gral_splines_buffer_fini (cairo_gral_splines_buffer_t *splines)
{
  cairo_gral_splines_buf_t *buf;

  buf = splines->buf_head.base.next;
  while (buf) {
    cairo_gral_splines_buf_t *this = buf;
    buf = buf->next;
    _cairo_gral_splines_buf_destroy (this);
  }
  splines->buf_head.base.next = NULL;
  splines->buf_head.base.prev = NULL;
  splines->buf_tail = &splines->buf_head.base;
  splines->buf_head.base.num_splines = 0;
}
