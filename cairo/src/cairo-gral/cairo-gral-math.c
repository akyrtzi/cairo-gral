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

#include "cairo-gral-math.h"

double
_cairo_gral_vector2_normalize (cairo_gral_vector2_t *v)
{
  double fLength = sqrt( v->x * v->x + v->y * v->y);

  // Will also work for zero-sized vectors, but will change nothing
  if ( fLength > 1e-08 )
  {
    double fInvLength = 1.0f / fLength;
    v->x *= fInvLength;
    v->y *= fInvLength;
  }

  return fLength;
}

void
_cairo_gral_matrix_from_cairo_matrix (gral_matrix_t *gm, cairo_matrix_t *cm)
{
  float xx = (float)cm->xx, xy = (float)cm->xy, x0 = (float)cm->x0;
  float yx = (float)cm->yx, yy = (float)cm->yy, y0 = (float)cm->y0;
  gm->m[0][0] = xx;  gm->m[0][1] = xy; gm->m[0][2] = 0; gm->m[0][3] = x0;
  gm->m[1][0] = yx;  gm->m[1][1] = yy; gm->m[1][2] = 0; gm->m[1][3] = y0;
  gm->m[2][0] = 0;  gm->m[2][1] = 0;   gm->m[2][2] = 1; gm->m[2][3] = 0;
  gm->m[3][0] = 0;  gm->m[3][1] = 0;   gm->m[3][2] = 0; gm->m[3][3] = 1;
}

void
_cairo_gral_subdivide_spline (double                     t,
                              const cairo_gral_spline_t *spline,
                              cairo_gral_spline_t       *left,
                              cairo_gral_spline_t       *right)
{
  const cairo_gral_vector2_t *cp       = spline->knots;
  cairo_gral_vector2_t       *left_cp  = left->knots;
  cairo_gral_vector2_t       *right_cp = right->knots;

  cairo_gral_vector2_t p01   = { cp[0].x * (1-t) + cp[1].x * t,
                                 cp[0].y * (1-t) + cp[1].y * t };
  cairo_gral_vector2_t p12   = { cp[1].x * (1-t) + cp[2].x * t,
                                 cp[1].y * (1-t) + cp[2].y * t };
  cairo_gral_vector2_t p23   = { cp[2].x * (1-t) + cp[3].x * t,
                                 cp[2].y * (1-t) + cp[3].y * t };
  cairo_gral_vector2_t p012  = { p01.x  * (1-t) + p12.x  * t,
                                 p01.y  * (1-t) + p12.y  * t };
  cairo_gral_vector2_t p123  = { p12.x  * (1-t) + p23.x  * t,
                                 p12.y  * (1-t) + p23.y  * t };
  cairo_gral_vector2_t p0123 = { p012.x * (1-t) + p123.x * t,
                                 p012.y * (1-t) + p123.y * t };

  left_cp[0] = cp[0];
  left_cp[1] = p01;
  left_cp[2] = p012;
  left_cp[3] = p0123;

  right_cp[0] = p0123;
  right_cp[1] = p123;
  right_cp[2] = p23;
  right_cp[3] = cp[3];
}

cairo_bool_t
_cairo_gral_quad_is_clockwise (cairo_gral_vector2_t p[4])
{
  double angle1, angle2;
  cairo_gral_vector2_t d01, d02, d03;

  VECTOR2_SUB (d01, p[1], p[0]);
  VECTOR2_SUB (d02, p[2], p[0]);
  VECTOR2_SUB (d03, p[3], p[0]);

  angle1 = VECTOR2_CROSS (d01, d02);
  angle2 = VECTOR2_CROSS (d02, d03);
  if (fabs(angle2) > fabs(angle1))
    angle1 = angle2;

  return (angle1 > 0 ? TRUE : FALSE);
}
