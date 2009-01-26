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

#ifndef CAIRO_GRAL_PRIVATE_H
#define CAIRO_GRAL_PRIVATE_H

#include "gral.h"
#include "cairo-gral-config.h"
#include "cairoint.h"
#include "cairo-path-fixed-private.h"

CAIRO_BEGIN_DECLS

#define CAIRO_SURFACE_TYPE_GRAL 200

#if CAIRO_GRAL_MAX_VERTICES <= 0x10000
  #define CAIRO_GRAL_USE_SHORT_INDICES 1
  typedef uint16_t cairo_gral_vertex_index_t;
#else
  #define CAIRO_GRAL_USE_SHORT_INDICES 0
  typedef uint32_t cairo_gral_vertex_index_t;
#endif

typedef struct _cairo_gral_gpu_resources {
  cairo_reference_count_t ref_count;

  gral_capabilities_t     caps;

  gral_vertex_buffer_t   *vertex_buf_pos;
  gral_vertex_buffer_t   *vertex_buf_tex;
  gral_index_buffer_t    *index_buf;
  gral_vertex_data_t     *vertex_data_source;
  gral_vertex_data_t     *vertex_data_stencil;
  gral_vertex_data_t     *vertex_data_spline;
  gral_index_data_t      *index_data;

  gral_texture_t         *gral_tex;
  gral_cg_program_t      *radial_shader;
  gral_cg_program_t      *spline_fill_shader;

} cairo_gral_gpu_resources_t;

cairo_private cairo_gral_gpu_resources_t *
_cairo_gral_gpu_resources_acquire (void);

cairo_private void
_cairo_gral_gpu_resources_release (cairo_gral_gpu_resources_t *gpu);

typedef struct _cairo_gral_surface {
  cairo_surface_t             base;

  gral_surface_t             *gral_surf;
  cairo_gral_gpu_resources_t *gpu;

  cairo_bool_t                has_clip;

} cairo_gral_surface_t;

typedef struct _cairo_gral_vertex_pos {
  float x,y,z;
} cairo_gral_vertex_pos_t;

typedef struct _cairo_gral_tex_coord3 {
  float x,y,z;
} cairo_gral_tex_coord3_t;

typedef struct _cairo_gral_bound_box {
  float min_x, min_y;
  float max_x, max_y;
} cairo_gral_bound_box_t;

typedef struct _cairo_gral_vector2 {
  float x, y;
} cairo_gral_vector2_t;

typedef struct _cairo_gral_spline {
  cairo_gral_vector2_t knots[4];
} cairo_gral_spline_t;

/* make cairo_gral_splines_buffer_t fit 4K bytes. 63 items */
#define CAIRO_GRAL_SPLINE_BUF_SIZE ((4*1024 - sizeof (void*) - sizeof (cairo_gral_splines_buf_t)) \
                                   / sizeof (cairo_gral_spline_t))

typedef struct _cairo_gral_splines_buf {
  struct _cairo_gral_splines_buf *next, *prev;
  unsigned int buf_size;
  unsigned int num_splines;

  cairo_gral_spline_t *splines;
} cairo_gral_splines_buf_t;
typedef struct _cairo_gral_splines_buf_fixed {
  cairo_gral_splines_buf_t base;

  cairo_gral_spline_t splines[CAIRO_GRAL_SPLINE_BUF_SIZE];
} cairo_gral_splines_buf_fixed_t;

typedef struct _cairo_gral_splines_buffer {
  cairo_gral_splines_buf_t       *buf_tail;
  cairo_gral_splines_buf_fixed_t  buf_head;
} cairo_gral_splines_buffer_t;

cairo_private void
_cairo_gral_splines_buffer_init (cairo_gral_splines_buffer_t *splines);

cairo_private cairo_status_t
_cairo_gral_splines_buffer_add (cairo_gral_splines_buffer_t *splines_buf,
                                const cairo_gral_spline_t   *spline);

cairo_private cairo_bool_t
_cairo_gral_splines_buffer_is_empty (cairo_gral_splines_buffer_t *splines_buf);

cairo_private void
_cairo_gral_splines_buffer_fini (cairo_gral_splines_buffer_t *splines);

typedef void
(cairo_gral_mesh_on_full_t) (void *closure);

typedef struct _cairo_gral_mesh {
  gral_render_operation_t     op;
  gral_vertex_buffer_t       *vertex_buf_pos;
  gral_vertex_buffer_t       *vertex_buf_tex;
  gral_index_buffer_t        *index_buf;

  cairo_gral_vertex_pos_t    *vertices;
  cairo_gral_tex_coord3_t    *tex_coords;
  cairo_gral_vertex_index_t  *indices;
  size_t                      num_vertices;
  size_t                      num_indices;

  cairo_gral_splines_buffer_t splines;

  cairo_gral_bound_box_t      box;

} cairo_gral_mesh_t;

/* Mesh functions. */

cairo_private void
_cairo_gral_mesh_init (cairo_gral_mesh_t          *mesh,
                       cairo_gral_vertex_pos_t    *vertices,
                       cairo_gral_tex_coord3_t    *tex_coords,
                       cairo_gral_vertex_index_t  *indices,
                       gral_vertex_buffer_t       *vertex_buf_pos,
                       gral_vertex_buffer_t       *vertex_buf_tex,
                       gral_index_buffer_t        *index_buf,
                       gral_vertex_data_t         *vertex_data,
                       gral_index_data_t          *index_data);

cairo_private void
_cairo_gral_mesh_fini (cairo_gral_mesh_t *mesh);

cairo_private cairo_gral_vertex_index_t
_cairo_gral_mesh_add_vertex_float (cairo_gral_mesh_t *mesh, float x, float y);

cairo_private cairo_gral_vertex_index_t
_cairo_gral_mesh_add_vertex_pos_and_tex (cairo_gral_mesh_t       *mesh,
                                         float                    x,
                                         float                    y,
                                         cairo_gral_tex_coord3_t *tex_coord);

#define _cairo_gral_mesh_add_vertex_point(mesh,p)                           \
  _cairo_gral_mesh_add_vertex_float (mesh,                                  \
                                     (float)_cairo_fixed_to_double((p)->x), \
                                     (float)_cairo_fixed_to_double((p)->y))

cairo_private void
_cairo_gral_mesh_add_index (cairo_gral_mesh_t *mesh,
                            cairo_gral_vertex_index_t *index);

cairo_private void
_cairo_gral_mesh_render (cairo_gral_mesh_t *mesh);

cairo_private void
_cairo_gral_mesh_gpu_spline_fill (cairo_gral_mesh_t          *mesh,
                                  cairo_gral_gpu_resources_t *gpu);

/* Stroke functions. */

typedef struct _cairo_gral_stroke_path_mesh cairo_gral_stroke_path_mesh_t;

cairo_private cairo_status_t
_cairo_gral_path_stroke_triangle (cairo_gral_stroke_path_mesh_t *mesh,
                                   const cairo_point_t t[3]);

cairo_private cairo_status_t
_cairo_gral_path_stroke_convex_quad (cairo_gral_stroke_path_mesh_t *mesh,
                                      const cairo_point_t q[4]);

cairo_private cairo_status_t
_cairo_gral_path_stroke_spline_open (cairo_gral_stroke_path_mesh_t *mesh,
                                     const cairo_point_t *forward_point,
                                     const cairo_point_t *backward_point);

cairo_private cairo_status_t
_cairo_gral_path_stroke_spline_extend (cairo_gral_stroke_path_mesh_t *mesh,
                                       const cairo_point_t *forward_point,
                                       const cairo_point_t *backward_point);

cairo_private cairo_status_t
_cairo_gral_path_stroke_spline_close (cairo_gral_stroke_path_mesh_t *mesh);

typedef struct {
  cairo_pen_t pen;
  cairo_spline_t spline;
  cairo_gral_stroke_path_mesh_t *mesh;
  cairo_point_t last_point;
  cairo_point_t forward_hull_point;
  cairo_point_t backward_hull_point;
  int forward_vertex;
  int backward_vertex;
} cairo_gral_pen_stroke_spline_t;

cairo_private cairo_int_status_t
_cairo_gral_pen_stroke_spline_init (cairo_gral_pen_stroke_spline_t *stroker,
                                    const cairo_pen_t *pen,
                                    const cairo_point_t *a,
                                    const cairo_point_t *b,
                                    const cairo_point_t *c,
                                    const cairo_point_t *d,
                                    cairo_gral_stroke_path_mesh_t *mesh);

cairo_private void
_cairo_gral_pen_stroke_spline_fini (cairo_gral_pen_stroke_spline_t *stroker);

cairo_private cairo_status_t
_cairo_gral_pen_stroke_spline (cairo_gral_pen_stroke_spline_t	*stroker,
                               double tolerance);

cairo_private cairo_status_t
_cairo_gral_path_fixed_stroke_to_mesh (cairo_path_fixed_t	*path,
				   cairo_stroke_style_t	*stroke_style,
				   cairo_matrix_t	*ctm,
				   cairo_matrix_t	*ctm_inverse,
				   double		 tolerance,
				   cairo_gral_stroke_path_mesh_t *mesh);

/* Utility functions. */

cairo_private void
_cairo_gral_init_render_state (cairo_gral_surface_t *gsurface);

cairo_private cairo_int_status_t
_cairo_gral_set_source (cairo_gral_surface_t *gsurface,
                        const cairo_pattern_t	*source);

cairo_private void
_cairo_gral_render_quad (cairo_gral_surface_t *gsurface,
                         float left, float top, float right, float bottom);

cairo_private cairo_status_t
_cairo_gral_prepare_fill_stencil_mask (cairo_gral_surface_t *gsurface,
                                       cairo_path_fixed_t	*path,
                                       cairo_fill_rule_t	 fill_rule,
                                       double			 tolerance,
                                       cairo_gral_bound_box_t *box);

cairo_private cairo_status_t
_cairo_gral_prepare_stroke_stencil_mask (cairo_gral_surface_t   *gsurface,
                                         cairo_path_fixed_t     *path,
                                         cairo_stroke_style_t   *style,
                                         cairo_matrix_t	        *ctm,
                                         cairo_matrix_t	        *ctm_inverse,
                                         double                  tolerance,
                                         cairo_gral_bound_box_t *box);

cairo_private gral_cg_program_t *
_cairo_gral_load_fragment_program (const char *entry,
                                   const char *profiles);

#define _cairo_gral_has_capability(gsurface, cap) (gsurface->gpu->caps & cap)

CAIRO_END_DECLS

#endif /* CAIRO_GRAL_PRIVATE_H */
