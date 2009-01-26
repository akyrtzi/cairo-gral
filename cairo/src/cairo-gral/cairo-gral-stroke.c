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

struct _cairo_gral_stroke_path_mesh {
  cairo_gral_mesh_t           base;

  cairo_point_t               spline_forward_point;
  cairo_point_t               spline_backward_point;
  cairo_gral_vertex_index_t   spline_forward_index;
  cairo_gral_vertex_index_t   spline_backward_index;
};

cairo_status_t
_cairo_gral_path_stroke_triangle (cairo_gral_stroke_path_mesh_t *mesh,
                                  const cairo_point_t t[3])
{
  cairo_gral_vertex_index_t index;

  index = _cairo_gral_mesh_add_vertex_point (&mesh->base, &t[0]);
  _cairo_gral_mesh_add_index (&mesh->base, &index);
  index = _cairo_gral_mesh_add_vertex_point (&mesh->base, &t[1]);
  _cairo_gral_mesh_add_index (&mesh->base, &index);
  index = _cairo_gral_mesh_add_vertex_point (&mesh->base, &t[2]);
  _cairo_gral_mesh_add_index (&mesh->base, &index);

  return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_gral_path_stroke_convex_quad (cairo_gral_stroke_path_mesh_t *mesh,
                                     const cairo_point_t q[4])
{
  cairo_gral_vertex_index_t index0, index1, index2;

  index0 = _cairo_gral_mesh_add_vertex_point (&mesh->base, &q[0]);
  _cairo_gral_mesh_add_index (&mesh->base, &index0);
  index1 = _cairo_gral_mesh_add_vertex_point (&mesh->base, &q[1]);
  _cairo_gral_mesh_add_index (&mesh->base, &index1);
  index2 = _cairo_gral_mesh_add_vertex_point (&mesh->base, &q[2]);
  _cairo_gral_mesh_add_index (&mesh->base, &index2);

  _cairo_gral_mesh_add_index (&mesh->base, &index0);
  _cairo_gral_mesh_add_index (&mesh->base, &index2);
  index1 = _cairo_gral_mesh_add_vertex_point (&mesh->base, &q[3]);
  _cairo_gral_mesh_add_index (&mesh->base, &index1);

  return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_gral_path_stroke_spline_open (cairo_gral_stroke_path_mesh_t *mesh,
                                     const cairo_point_t *forward_point,
                                     const cairo_point_t *backward_point)
{
  mesh->spline_forward_point = *forward_point;
  mesh->spline_backward_point = *backward_point;
  mesh->spline_forward_index = _cairo_gral_mesh_add_vertex_point (&mesh->base, forward_point);
  mesh->spline_backward_index = _cairo_gral_mesh_add_vertex_point (&mesh->base, backward_point);

  return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_gral_path_stroke_spline_extend (cairo_gral_stroke_path_mesh_t *mesh,
                                       const cairo_point_t *forward_point,
                                       const cairo_point_t *backward_point)
{
  cairo_gral_vertex_index_t index;

  if (mesh->spline_forward_point.x != forward_point->x ||
      mesh->spline_forward_point.y != forward_point->y   ) {

    _cairo_gral_mesh_add_index (&mesh->base, &mesh->spline_forward_index);
    _cairo_gral_mesh_add_index (&mesh->base, &mesh->spline_backward_index);
    index = _cairo_gral_mesh_add_vertex_point (&mesh->base, forward_point);
    _cairo_gral_mesh_add_index (&mesh->base, &index);

    mesh->spline_forward_index = index;
  }

  if (mesh->spline_backward_point.x != backward_point->x ||
      mesh->spline_backward_point.y != backward_point->y   ) {

    _cairo_gral_mesh_add_index (&mesh->base, &mesh->spline_forward_index);
    _cairo_gral_mesh_add_index (&mesh->base, &mesh->spline_backward_index);
    index = _cairo_gral_mesh_add_vertex_point (&mesh->base, backward_point);
    _cairo_gral_mesh_add_index (&mesh->base, &index);

    mesh->spline_backward_index = index;
  }

  return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_gral_path_stroke_spline_close (cairo_gral_stroke_path_mesh_t *mesh)
{
  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_gral_render_stroke_path (cairo_gral_surface_t *gsurface,
                                cairo_path_fixed_t	*path,
                                cairo_stroke_style_t	*style,
                                cairo_matrix_t		*ctm,
                                cairo_matrix_t		*ctm_inverse,
                                double			tolerance,
                                cairo_gral_bound_box_t *box)
{
  cairo_gral_stroke_path_mesh_t mesh;
  cairo_gral_vertex_pos_t vertices[CAIRO_GRAL_MAX_VERTICES];
  cairo_gral_vertex_index_t indices[CAIRO_GRAL_MAX_INDICES];
  cairo_gral_gpu_resources_t *gpu = gsurface->gpu;
  cairo_status_t status;

  _cairo_gral_mesh_init (&mesh.base,
                         vertices,
                         NULL, /*tex_coords*/
                         indices,
                         gpu->vertex_buf_pos,
                         gpu->vertex_buf_tex,
                         gpu->index_buf,
                         gpu->vertex_data_stencil,
                         gpu->index_data);

  status = _cairo_gral_path_fixed_stroke_to_mesh (path,
                                                  style,
                                                  ctm,
                                                  ctm_inverse,
                                                  tolerance,
                                                  &mesh);
  if (unlikely (status))
    goto BAIL;

  _cairo_gral_mesh_render (&mesh.base);
  if (box)
    *box = mesh.base.box;

BAIL:
  _cairo_gral_mesh_fini (&mesh.base);
  return status;
}

cairo_status_t
_cairo_gral_prepare_stroke_stencil_mask (cairo_gral_surface_t   *gsurface,
                                         cairo_path_fixed_t     *path,
                                         cairo_stroke_style_t   *style,
                                         cairo_matrix_t	        *ctm,
                                         cairo_matrix_t	        *ctm_inverse,
                                         double                  tolerance,
                                         cairo_gral_bound_box_t *box)
{
  gral_set_stencil_check_enabled (TRUE);
  gral_set_color_buffer_write_enabled (FALSE, FALSE, FALSE, FALSE);

  /* Draw where stencil is zero, if stencil is already set don't draw or modify the stencil */
  gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_EQUAL,
                                  0, 0xffffffff,
                                  GRAL_STENCIL_OPERATION_KEEP,
                                  GRAL_STENCIL_OPERATION_KEEP,
                                  GRAL_STENCIL_OPERATION_INCREMENT,
                                  FALSE);

  return _cairo_gral_render_stroke_path (gsurface, path, style, ctm, ctm_inverse, tolerance, box);
}
