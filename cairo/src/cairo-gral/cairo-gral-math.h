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

#ifndef CAIRO_GRAL_MATH_H
#define CAIRO_GRAL_MATH_H

#include "gral.h"
#include "cairoint.h"
#include "cairo-gral-private.h"

CAIRO_BEGIN_DECLS

typedef struct _cairo_gral_vector3 {
  float x, y, z;
} cairo_gral_vector3_t;

#define VECTOR2_FROM_POINT(v,p)             \
  do {                                      \
    (v).x = _cairo_fixed_to_double((p).x);  \
    (v).y = _cairo_fixed_to_double((p).y);  \
  } while (0)

#define VECTOR2_ADD(res,v1,v2)  \
  do {                          \
    (res).x = (v1).x + (v2).x;  \
    (res).y = (v1).y + (v2).y;  \
  } while (0)

#define VECTOR2_SUB(res,v1,v2)  \
  do {                          \
    (res).x = (v1).x - (v2).x;  \
    (res).y = (v1).y - (v2).y;  \
  } while (0)

#define VECTOR2_DIV_SCALAR(res,v,s)   \
  do {                                \
    (res).x = (v).x / (s);            \
    (res).y = (v).y / (s);            \
  } while (0)

#define VECTOR2_CROSS(v1,v2) ((v1).x * (v2).y - (v1).y * (v2).x)

#define VECTOR3_INIT(v,vx,vy,vz)        \
  do {                                  \
    (v).x = vx, (v).y = vy, (v).z = vz; \
  } while (0)

#define VECTOR3_CROSS(res,v1,v2)                 \
  do {                                            \
    (res).x = (v1).y * (v2).z - (v1).z * (v2).y;  \
    (res).y = (v1).z * (v2).x - (v1).x * (v2).z;  \
    (res).z = (v1).x * (v2).y - (v1).y * (v2).x;  \
  } while (0)

#define VECTOR3_DOT(v1,v2) ((v1).x * (v2).x + (v1).y * (v2).y + (v1).z * (v2).z)

#define _cairo_gral_lerp(a,b,t) ( (a)+(t)*((b)-(a)) )
#define _cairo_gral_color_lerp(res, c1, c2, t)        \
  do {                                                \
  (res)->r = _cairo_gral_lerp((c1)->r, (c2)->r, t); \
  (res)->g = _cairo_gral_lerp((c1)->g, (c2)->g, t); \
  (res)->b = _cairo_gral_lerp((c1)->b, (c2)->b, t); \
  (res)->a = _cairo_gral_lerp((c1)->a, (c2)->a, t); \
  } while (0)

cairo_private double
_cairo_gral_vector2_normalize (cairo_gral_vector2_t *v);

cairo_private void
_cairo_gral_matrix_from_cairo_matrix (gral_matrix_t *gm, cairo_matrix_t *cm);

cairo_private void
_cairo_gral_subdivide_spline (double                     t,
                              const cairo_gral_spline_t *spline,
                              cairo_gral_spline_t       *left,
                              cairo_gral_spline_t       *right);

cairo_private cairo_bool_t
_cairo_gral_quad_is_clockwise (cairo_gral_vector2_t p[3]);

CAIRO_END_DECLS

#endif /* CAIRO_GRAL_MATH_H */