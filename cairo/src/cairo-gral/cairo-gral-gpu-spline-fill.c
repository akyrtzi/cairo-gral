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
#include "cairo-gral-math.h"

static void
_cairo_gral_fill_spline (cairo_gral_mesh_t     *mesh,
                         cairo_gral_vector2_t   cp[4])
{
  cairo_gral_vector3_t M[4];

  double a1, a2, a3;
  double d1, d2, d3;
  double ls,lt, ms,mt;
  cairo_bool_t reverse_orientation = FALSE;


  cairo_gral_vector3_t b0 = { cp[0].x, cp[0].y, 1 };
  cairo_gral_vector3_t b1 = { cp[1].x, cp[1].y, 1 };
  cairo_gral_vector3_t b2 = { cp[2].x, cp[2].y, 1 };
  cairo_gral_vector3_t b3 = { cp[3].x, cp[3].y, 1 };

  cairo_gral_vector3_t b3xb2;
  cairo_gral_vector3_t b0xb3;
  cairo_gral_vector3_t b1xb0;
  VECTOR3_CROSS (b3xb2, b3, b2);
  VECTOR3_CROSS (b0xb3, b0, b3);
  VECTOR3_CROSS (b1xb0, b1, b0);
  
  a1 = VECTOR3_DOT (b0, b3xb2);
  a2 = VECTOR3_DOT (b1, b0xb3);
  a3 = VECTOR3_DOT (b2, b1xb0);

  d1 = a1 - 2*a2 + 3*a3;
  d2 = -a2 + 3*a3;
  d3 = 3*a3;

  if (d1 == 0 && d2 == 0 && d3 == 0) {
    /* It's a line. Can't fill that.. */
    return;
  }

  if (d1 == 0 && d2 == 0) {
    /* Quadratic */

    VECTOR3_INIT (M[0], 0,       0,       0);
    VECTOR3_INIT (M[1], 1.0/3.0, 0,       1.0/3.0);
    VECTOR3_INIT (M[2], 2.0/3.0, 1.0/3.0, 2.0/3.0);
    VECTOR3_INIT (M[3], 1,       1,       1);

  } else  if (d1 == 0) {
    /* cusp at infinity */

    ls = d3;
    lt = 3*d2;

    VECTOR3_INIT (M[0], ls,              ls*ls*ls,                1);
    VECTOR3_INIT (M[1], ls-(1.0/3.0)*lt, ls*ls*(ls-lt),           1);
    VECTOR3_INIT (M[2], ls-(2.0/3.0)*lt, (ls-lt)*(ls-lt)*ls,      1);
    VECTOR3_INIT (M[3], ls-lt,           (ls-lt)*(ls-lt)*(ls-lt), 1);

  } else {

    double disI = 3*d2*d2-4*d1*d3;

    if (disI >= 0) {
      /* serpentine or cusp */

      disI = disI/3;
      ls = d2 - sqrt(disI);
      lt = 2*d1;
      ms = d2 + sqrt(disI);
      mt = 2*d1;

      VECTOR3_INIT (M[0], ls*ms,                                   ls*ls*ls,                 ms*ms*ms);
      VECTOR3_INIT (M[1], (1.0/3.0)*(3*ls*ms-ls*mt-lt*ms),         ls*ls*(ls-lt),            ms*ms*(ms-mt));
      VECTOR3_INIT (M[2], (1.0/3.0)*(lt*(mt-2*ms)+ls*(3*ms-2*mt)), (lt-ls)*(lt-ls)*ls,       (mt-ms)*(mt-ms)*ms);
      VECTOR3_INIT (M[3], (lt-ls)*(mt-ms),                         -(lt-ls)*(lt-ls)*(lt-ls), -(mt-ms)*(mt-ms)*(mt-ms));

      if (d1 < 0)
        reverse_orientation = TRUE;

    } else {
      /* loop */

      assert(disI < 0);
      disI = -disI;
      ls = d2 - sqrt(disI);
      lt = 2*d1;
      ms = d2 + sqrt(disI);
      mt = 2*d1;

      VECTOR3_INIT (M[0], ls*ms,                                   ls*ls*ms,                                 ls*ms*ms);
      VECTOR3_INIT (M[1], (1.0/3.0)*(3*ls*ms-ls*mt-lt*ms),         -(1.0/3.0)*ls*(ls*(mt-3*ms)+2*lt*ms),     -(1.0/3.0)*ms*(ls*(2*mt-3*ms)+lt*ms));
      VECTOR3_INIT (M[2], (1.0/3.0)*(lt*(mt-2*ms)+ls*(3*ms-2*mt)), (1.0/3.0)*(lt-ls)*(ls*(2*mt-3*ms)+lt*ms), (1.0/3.0)*(mt-ms)*(ls*(mt-3*ms)+2*lt*ms));
      VECTOR3_INIT (M[3], (lt-ls)*(mt-ms),                         -(lt-ls)*(lt-ls)*(mt-ms),                 -(lt-ls)*(mt-ms)*(mt-ms));

      if ((d1 < 0 && M[1].x > 0) || (d1 > 0 && M[1].x < 0))
        reverse_orientation = TRUE;
    }
  }

  if (_cairo_gral_quad_is_clockwise (cp))
    reverse_orientation = ! reverse_orientation;

  if (reverse_orientation) {
    VECTOR3_INIT (M[0], -M[0].x, -M[0].y, M[0].z);
    VECTOR3_INIT (M[1], -M[1].x, -M[1].y, M[1].z);
    VECTOR3_INIT (M[2], -M[2].x, -M[2].y, M[2].z);
    VECTOR3_INIT (M[3], -M[3].x, -M[3].y, M[3].z);
  }

  {
    cairo_gral_vertex_index_t index0, index1, index2, index3;
    cairo_gral_tex_coord3_t tex[4];
    int i;

    for (i = 0; i < 4; ++i) {
      tex[i].x = M[i].x;
      tex[i].y = M[i].y;
      tex[i].z = M[i].z;
    }

    index0 = _cairo_gral_mesh_add_vertex_pos_and_tex (mesh,
                                                      cp[0].x, cp[0].y,
                                                      &tex[0]);
    _cairo_gral_mesh_add_index (mesh, &index0);
    index1 = _cairo_gral_mesh_add_vertex_pos_and_tex (mesh,
                                                      cp[1].x, cp[1].y,
                                                      &tex[1]);
    _cairo_gral_mesh_add_index (mesh, &index1);
    index2 = _cairo_gral_mesh_add_vertex_pos_and_tex (mesh,
                                                      cp[2].x, cp[2].y,
                                                      &tex[2]);
    _cairo_gral_mesh_add_index (mesh, &index2);

    _cairo_gral_mesh_add_index (mesh, &index0);
    _cairo_gral_mesh_add_index (mesh, &index2);
    index3 = _cairo_gral_mesh_add_vertex_pos_and_tex (mesh,
                                                      cp[3].x, cp[3].y,
                                                      &tex[3]);
    _cairo_gral_mesh_add_index (mesh, &index3);
  }
}

void
_cairo_gral_mesh_gpu_spline_fill (cairo_gral_mesh_t          *mesh,
                                  cairo_gral_gpu_resources_t *gpu)
{
  cairo_gral_mesh_t spline_mesh;
  cairo_gral_tex_coord3_t tex_coords[CAIRO_GRAL_MAX_VERTICES];

  const cairo_gral_splines_buffer_t *splines_buffer;
  const cairo_gral_splines_buf_t    *buf;

  assert (mesh->num_vertices == 0 && mesh->num_indices == 0);
  assert (mesh->vertices && mesh->indices);

  _cairo_gral_mesh_init (&spline_mesh,
                         mesh->vertices,
                         tex_coords,
                         mesh->indices,
                         gpu->vertex_buf_pos,
                         gpu->vertex_buf_tex,
                         gpu->index_buf,
                         gpu->vertex_data_spline,
                         gpu->index_data);
  spline_mesh.box = mesh->box;

  if (gpu->spline_fill_shader == NULL) {

    gpu->spline_fill_shader = 
        _cairo_gral_load_fragment_program ("fp_cubic_bezier_fill", "ps_2_0 arbfp1");
    assert (gpu->spline_fill_shader && "Shader failed to load properly!");
  }

  gral_disable_texture_units_from (1);
  gral_set_texture_coord_set (0, 0);
  gral_cg_program_bind (gpu->spline_fill_shader);

  splines_buffer = &mesh->splines;
  for (buf = &splines_buffer->buf_head.base; buf; buf = buf->next)
  {
    int start, stop, i;

    start = 0;
    stop = buf->num_splines;

    for (i = start; i != stop; ++i)
      _cairo_gral_fill_spline (&spline_mesh, buf->splines[i].knots);
  }

  _cairo_gral_mesh_render (&spline_mesh);

  mesh->box = spline_mesh.box;

  _cairo_gral_mesh_fini (&spline_mesh);
}
