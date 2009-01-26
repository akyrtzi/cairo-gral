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

typedef struct _cairo_gral_fill_path_mesh {
  cairo_gral_mesh_t           base;

  cairo_gral_vector2_t        cur_point;
  cairo_bool_t                drawing_line;
  cairo_gral_vertex_index_t   cur_centric_vertex;
  cairo_gral_vertex_index_t   prev_vertex;

} cairo_gral_fill_path_mesh_t;

static cairo_status_t
_cairo_gral_fill_path_move_to (void                *closure,
                               const cairo_point_t *point)
{
  cairo_gral_fill_path_mesh_t *mesh = closure;

  if (mesh->drawing_line &&
      (mesh->prev_vertex - mesh->cur_centric_vertex) == 1) {
    /* The previous segment was just a single line. This can't be filled
     * so remove the 2 vertices of the line. */
    assert (mesh->base.num_vertices-1 == mesh->prev_vertex);
    mesh->base.num_vertices -= 2;
  }

  mesh->drawing_line = FALSE;
  VECTOR2_FROM_POINT (mesh->cur_point, *point);
  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_gral_fill_path_line_to_vector (cairo_gral_fill_path_mesh_t *mesh ,
                                      const cairo_gral_vector2_t  *point)
{
  if (mesh->cur_point.x == point->x && mesh->cur_point.y == point->y)
    return CAIRO_STATUS_SUCCESS;

  if (mesh->drawing_line) {
    cairo_gral_vertex_index_t next_vertex;

    _cairo_gral_mesh_add_index (&mesh->base, &mesh->cur_centric_vertex);
    _cairo_gral_mesh_add_index (&mesh->base, &mesh->prev_vertex);
    next_vertex = _cairo_gral_mesh_add_vertex_float (&mesh->base, point->x, point->y);
    _cairo_gral_mesh_add_index (&mesh->base, &next_vertex);
    mesh->prev_vertex = next_vertex;
    
  } else {
    mesh->drawing_line = TRUE;

    mesh->cur_centric_vertex = _cairo_gral_mesh_add_vertex_float (&mesh->base,
                                                                  mesh->cur_point.x, mesh->cur_point.y);
    mesh->prev_vertex = _cairo_gral_mesh_add_vertex_float (&mesh->base, point->x, point->y);
  }

  mesh->cur_point = *point;

  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_gral_fill_path_line_to (void                *closure,
                               const cairo_point_t *point)
{
  cairo_gral_vector2_t vec;
  VECTOR2_FROM_POINT (vec, *point);
  return _cairo_gral_fill_path_line_to_vector (closure, &vec);
}

static cairo_status_t
_cairo_gral_fill_path_add_spline (cairo_gral_fill_path_mesh_t *mesh,
                                  const cairo_gral_spline_t   *spline)
{
  cairo_status_t status;
  status = _cairo_gral_splines_buffer_add (&mesh->base.splines, spline);
  if (unlikely (status))
    return status;

  return _cairo_gral_fill_path_line_to_vector (mesh, &spline->knots[3]);
}

static cairo_status_t
_cairo_gral_fill_path_curve_to (void                *closure,
                                const cairo_point_t *p0,
                                const cairo_point_t *p1,
                                const cairo_point_t *p2)
{
  cairo_gral_fill_path_mesh_t *mesh = closure;
  cairo_status_t status;

  cairo_gral_spline_t spline;

  double a1, a2, a3;
  double d1, d2, d3;
  double ls,lt, ms,mt;
  double inflection1, inflection2;

  cairo_gral_vector3_t b0, b1, b2, b3;
  cairo_gral_vector3_t b3xb2;
  cairo_gral_vector3_t b0xb3;
  cairo_gral_vector3_t b1xb0;

  spline.knots[0] = mesh->cur_point;
  VECTOR2_FROM_POINT (spline.knots[1], *p0);
  VECTOR2_FROM_POINT (spline.knots[2], *p1);
  VECTOR2_FROM_POINT (spline.knots[3], *p2);

  VECTOR3_INIT (b0, spline.knots[0].x, spline.knots[0].y, 1);
  VECTOR3_INIT (b1, spline.knots[1].x, spline.knots[1].y, 1);
  VECTOR3_INIT (b2, spline.knots[2].x, spline.knots[2].y, 1);
  VECTOR3_INIT (b3, spline.knots[3].x, spline.knots[3].y, 1);

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
    /* It's a line. */
    return _cairo_gral_fill_path_line_to_vector (closure, &spline.knots[3]);
  }

  if (d1 == 0 && d2 == 0) {
    /* Quadratic */
    return _cairo_gral_fill_path_add_spline (mesh, &spline);
  }
  
  if (d1 == 0) {
    /* cusp at infinity */

    ls = d3;
    lt = 3*d2;

    assert(lt != 0);
    inflection1 = inflection2 = ls/lt;

  } else {

    double disI = 3*d2*d2-4*d1*d3;

    if (disI >= 0) {
      /* serpentine or cusp */
      disI = disI/3;
    } else {
      /* loop */
      disI = -disI;
    }

    ls = d2 - sqrt(disI);
    lt = 2*d1;
    ms = d2 + sqrt(disI);
    mt = 2*d1;

    assert(lt != 0 && mt != 0);
    inflection1 = ls/lt;
    inflection2 = ms/mt;
  }

  if (inflection1 > inflection2) {
    /* swap */
    double temp = inflection1;
    inflection1 = inflection2;
    inflection2 = temp;
  }

  if (inflection1 >=1 || inflection2 <=0 || (inflection1 <= 0 && inflection2 >=1))
    return _cairo_gral_fill_path_add_spline (mesh, &spline);

  {
    /* We will split the curve at the inflection points. */
    cairo_gral_spline_t left, right;

    if (inflection1 <= 0) {
      assert(inflection2 < 1);
      _cairo_gral_subdivide_spline (inflection2, &spline, &left, &right);

      status = _cairo_gral_fill_path_add_spline (mesh, &left);
      if (unlikely (status))
        return status;

      return _cairo_gral_fill_path_add_spline (mesh, &right);
    }

    assert(inflection1 > 0 && inflection1 < 1);
    _cairo_gral_subdivide_spline (inflection1, &spline, &left, &right);

    status = _cairo_gral_fill_path_add_spline (mesh, &left);
    if (unlikely (status))
      return status;

    if (inflection2 == inflection1 || inflection2 >= 1)
      return _cairo_gral_fill_path_add_spline (mesh, &right);

    assert(inflection2 > 0 && inflection2 < 1 && inflection2 > inflection1);
    _cairo_gral_subdivide_spline ((inflection2-inflection1)/(1-inflection1),
                                  &right, &left, &right);

    status = _cairo_gral_fill_path_add_spline (mesh, &left);
    if (unlikely (status))
      return status;

    return _cairo_gral_fill_path_add_spline (mesh, &right);
  }
}

static cairo_status_t
_cairo_path_to_verts_close_path (void *closure)
{
  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_gral_render_fill_path (cairo_gral_surface_t   *gsurface,
                              cairo_path_fixed_t     *path,
                              double                  tolerance,
                              cairo_gral_bound_box_t *box)
{
  cairo_gral_fill_path_mesh_t mesh;
  cairo_gral_vertex_pos_t vertices[CAIRO_GRAL_MAX_VERTICES];
  cairo_gral_vertex_index_t indices[CAIRO_GRAL_MAX_INDICES];
  cairo_bool_t use_shader = _cairo_gral_has_capability (gsurface, GRAL_CAP_FRAGMENT_PROGRAM);
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
  mesh.drawing_line = FALSE;

#if CAIRO_GRAL_DISABLE_GPU_SPLINE_RENDERING
  use_shader = FALSE;
#endif

  if (use_shader) {
    status = _cairo_path_fixed_interpret (path,
                                          CAIRO_DIRECTION_FORWARD,
                                          _cairo_gral_fill_path_move_to,
                                          _cairo_gral_fill_path_line_to,
                                          _cairo_gral_fill_path_curve_to,
                                          _cairo_path_to_verts_close_path,
                                          &mesh);
  } else {
    status = _cairo_path_fixed_interpret_flat (path,
                                               CAIRO_DIRECTION_FORWARD,
                                               _cairo_gral_fill_path_move_to,
                                               _cairo_gral_fill_path_line_to,
                                               _cairo_path_to_verts_close_path,
                                               &mesh,
                                               tolerance);
  }
  if (unlikely (status))
    goto BAIL;

  _cairo_gral_mesh_render (&mesh.base);

  if (! _cairo_gral_splines_buffer_is_empty (&mesh.base.splines))
    _cairo_gral_mesh_gpu_spline_fill (&mesh.base, gsurface->gpu);

  if (box)
    *box = mesh.base.box;

BAIL:
  _cairo_gral_mesh_fini (&mesh.base);
  return status;
}

cairo_status_t
_cairo_gral_prepare_fill_stencil_mask(cairo_gral_surface_t   *gsurface,
                                      cairo_path_fixed_t     *path,
                                      cairo_fill_rule_t       fill_rule,
                                      double                  tolerance,
                                      cairo_gral_bound_box_t *box)
{
  gral_set_stencil_check_enabled (TRUE);
  gral_set_color_buffer_write_enabled (FALSE, FALSE, FALSE, FALSE);

  switch (fill_rule) {
    default: ASSERT_NOT_REACHED;
    case CAIRO_FILL_RULE_WINDING:
      gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_ALWAYS_PASS,
                                      0, 0xffffffff,
                                      GRAL_STENCIL_OPERATION_INCREMENT_WRAP,
                                      GRAL_STENCIL_OPERATION_INCREMENT_WRAP,
                                      GRAL_STENCIL_OPERATION_INCREMENT_WRAP,
                                      TRUE /*twoSidedOperation*/);
      break;
    case CAIRO_FILL_RULE_EVEN_ODD:
      gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_ALWAYS_PASS,
                                      0, 0xffffffff,
                                      GRAL_STENCIL_OPERATION_INVERT,
                                      GRAL_STENCIL_OPERATION_INVERT,
                                      GRAL_STENCIL_OPERATION_INVERT,
                                      FALSE /*two_sided_operation*/);
      break;
  }

  return _cairo_gral_render_fill_path (gsurface, path, tolerance, box);
}
