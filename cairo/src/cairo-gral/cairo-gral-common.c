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

cairo_gral_gpu_resources_t shared_gpu_resources = {0,0,0,0,0,0,0,0};

static void
_cairo_gral_gpu_resources_init (cairo_gral_gpu_resources_t *gpu) {

  CAIRO_REFERENCE_COUNT_INIT (&gpu->ref_count, 1);

  gpu->caps = gral_get_capabilities ();
#if CAIRO_GRAL_DISABLE_FRAGMENT_SHADERS
  gpu->caps &= ~GRAL_CAP_FRAGMENT_PROGRAM;
#endif

  gpu->vertex_buf_pos = gral_vertex_buffer_create (sizeof(cairo_gral_vertex_pos_t),
                                          CAIRO_GRAL_MAX_VERTICES,
                                          GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE);
  gpu->vertex_buf_tex = gral_vertex_buffer_create (sizeof(cairo_gral_tex_coord3_t),
                                          CAIRO_GRAL_MAX_VERTICES,
                                          GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE);
  gpu->index_buf = gral_index_buffer_create (
#if CAIRO_GRAL_USE_SHORT_INDICES
        GRAL_INDEX_BUFFER_TYPE_16BIT,
#else
        GRAL_INDEX_BUFFER_TYPE_32BIT,
#endif
        CAIRO_GRAL_MAX_INDICES,
        GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE);

  {
    gral_vertex_data_t *vd = gral_vertex_data_create ();
    gral_vertex_data_set_start (vd, 0);
    gral_vertex_data_add_element (vd, 0/*source*/, 0/*offset*/,
                                  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
                                  GRAL_VERTEX_ELEMENT_SEMANTIC_POSITION, 0/*index*/);
    gral_vertex_data_add_element (vd, 1/*source*/, 0/*offset*/,
                                  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
                                  GRAL_VERTEX_ELEMENT_SEMANTIC_TEXTURE_COORDINATES, 0/*index*/);
    gral_vertex_data_bind_buffer (vd, 0/*source*/, gpu->vertex_buf_pos);
    gral_vertex_data_bind_buffer (vd, 1/*source*/, gpu->vertex_buf_pos);
    gpu->vertex_data_source = vd;
    assert (gral_vertex_data_get_vertex_size (vd, 0) == sizeof(cairo_gral_vertex_pos_t));
    assert (gral_vertex_data_get_vertex_size (vd, 1) == sizeof(cairo_gral_vertex_pos_t));
  }

  {
    gral_vertex_data_t *vd = gral_vertex_data_create ();
    gral_vertex_data_set_start (vd, 0);
    gral_vertex_data_add_element (vd, 0/*source*/, 0/*offset*/,
                                  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
                                  GRAL_VERTEX_ELEMENT_SEMANTIC_POSITION, 0/*index*/);
    gral_vertex_data_bind_buffer (vd, 0/*source*/, gpu->vertex_buf_pos);
    gpu->vertex_data_stencil = vd;
    assert (gral_vertex_data_get_vertex_size (vd, 0) == sizeof(cairo_gral_vertex_pos_t));
  }

  {
    gral_vertex_data_t *vd = gral_vertex_data_create ();
    gral_vertex_data_set_start (vd, 0);
    gral_vertex_data_add_element (vd, 0/*source*/, 0/*offset*/,
                                  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
                                  GRAL_VERTEX_ELEMENT_SEMANTIC_POSITION, 0/*index*/);
    gral_vertex_data_add_element (vd, 1/*source*/, 0/*offset*/,
                                  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
                                  GRAL_VERTEX_ELEMENT_SEMANTIC_TEXTURE_COORDINATES, 0/*index*/);
    gral_vertex_data_bind_buffer (vd, 0/*source*/, gpu->vertex_buf_pos);
    gral_vertex_data_bind_buffer (vd, 1/*source*/, gpu->vertex_buf_tex);
    gpu->vertex_data_spline = vd;
    assert (gral_vertex_data_get_vertex_size (vd, 0) == sizeof(cairo_gral_vertex_pos_t));
    assert (gral_vertex_data_get_vertex_size (vd, 1) == sizeof(cairo_gral_tex_coord3_t));
  }

  {
    gral_index_data_t *id = gral_index_data_create ();
    gral_index_data_set_start (id, 0);
    gral_index_data_set_buffer (id, gpu->index_buf);
    gpu->index_data = id;
  }
}

cairo_gral_gpu_resources_t *
_cairo_gral_gpu_resources_acquire (void)
{
  if (CAIRO_REFERENCE_COUNT_GET_VALUE (&shared_gpu_resources.ref_count) == 0)
    _cairo_gral_gpu_resources_init (&shared_gpu_resources);
  else
    _cairo_reference_count_inc (&shared_gpu_resources.ref_count);

  assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&shared_gpu_resources.ref_count));
  return &shared_gpu_resources;
}

void
_cairo_gral_gpu_resources_release (cairo_gral_gpu_resources_t *gpu)
{
  assert (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&gpu->ref_count));

  if (! _cairo_reference_count_dec_and_test (&gpu->ref_count))
    return;

  gral_vertex_buffer_destroy (gpu->vertex_buf_pos);
  gral_vertex_buffer_destroy (gpu->vertex_buf_tex);
  gral_index_buffer_destroy (gpu->index_buf);
  gral_vertex_data_destroy (gpu->vertex_data_source);
  gral_vertex_data_destroy (gpu->vertex_data_stencil);
  gral_vertex_data_destroy (gpu->vertex_data_spline);
  gral_index_data_destroy (gpu->index_data);

  if (gpu->gral_tex)
    gral_texture_destroy (gpu->gral_tex);
  if (gpu->radial_shader)
    gral_cg_program_destroy (gpu->radial_shader);
  if (gpu->spline_fill_shader)
    gral_cg_program_destroy (gpu->spline_fill_shader);

  memset (gpu, 0, sizeof(cairo_gral_gpu_resources_t));
}

gral_cg_program_t *
_cairo_gral_load_fragment_program (const char *entry,
                                   const char *profiles)
{
#if CAIRO_GRAL_EMBED_SHADER_SOURCE
  /* This symbol is embedded into the executable using 'as'. */
  extern char _cairo_gral_shaders_source_cg[];

  return gral_cg_program_create_from_source (
                      GRAL_GPU_PROGRAM_TYPE_FRAGMENT,
                      _cairo_gral_shaders_source_cg, entry, profiles);
#else
  return gral_cg_program_create_from_file (
                      GRAL_GPU_PROGRAM_TYPE_FRAGMENT,
                      "shaders.cg", entry, profiles);
#endif
}

void
_cairo_gral_render_quad (cairo_gral_surface_t *gsurface,
                         float left, float top, float right, float bottom)
{
  cairo_gral_vertex_pos_t verts[] = {
    {right, top,    CAIRO_GRAL_Z_VALUE},
    {left,  top,    CAIRO_GRAL_Z_VALUE},
    {right, bottom, CAIRO_GRAL_Z_VALUE},
    {left,  bottom, CAIRO_GRAL_Z_VALUE},
  };

  gral_vertex_buffer_t *vbuf = gsurface->gpu->vertex_buf_pos;
  void *dat;
  size_t length = sizeof(verts);
  assert(length <= gral_vertex_buffer_get_size (vbuf));
  dat = gral_vertex_buffer_lock (vbuf, 0, length, GRAL_BUFFER_LOCK_OPTION_DISCARD);
  memcpy(dat, verts, length);
  gral_vertex_buffer_unlock (vbuf);

  gral_vertex_data_set_count (gsurface->gpu->vertex_data_source, 4);

  {
    gral_render_operation_t op;
    op.operation_type = GRAL_RENDER_OPERATION_TYPE_TRIANGLE_STRIP;
    op.vertex_data = gsurface->gpu->vertex_data_source;
    op.use_indexes = FALSE;
    gral_render (&op);
  }
}

void _cairo_gral_init_render_state(cairo_gral_surface_t *gsurface)
{
  /* set-up matrices */

  gral_matrix_t mat;

  int width = gral_surface_get_width(gsurface->gral_surf);
  int height = gral_surface_get_height(gsurface->gral_surf);

  float scale_x, scale_y, scale_z;
  float trans_x, trans_y, trans_z;
  scale_x = 1.0f  / (0.5f * width);
  scale_y = -1.0f / (0.5f * height);
  scale_z = 1;
  trans_x = -1 + gral_get_horizontal_texel_offset() * scale_x;
  trans_y = 1 - gral_get_vertical_texel_offset() * scale_y;
  trans_z = 0;

  gral_matrix_init_identity (&mat);
  gral_matrix_set_translate (&mat, trans_x, trans_y, trans_z);
  gral_matrix_set_scale (&mat, scale_x, scale_y, scale_z);

  gral_set_render_surface(gsurface->gral_surf);

  gral_set_world_matrix(&mat);
  gral_set_view_matrix (GRAL_MATRIX_IDENTITY);
  gral_set_projection_matrix (GRAL_MATRIX_IDENTITY);

  gral_set_lighting_enabled (FALSE);
  gral_set_culling_mode (GRAL_CULL_NONE);
  gral_set_color_buffer_write_enabled (TRUE, TRUE, TRUE, TRUE);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_VERTEX);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_FRAGMENT);
  gral_set_shading_type (GRAL_SHADE_TYPE_FLAT);

  if (gsurface->has_clip)
    gral_set_depth_buffer_params (TRUE, FALSE, GRAL_COMPARE_FUNC_LESS);
  else
    gral_set_depth_buffer_params (FALSE, FALSE, GRAL_COMPARE_FUNC_LESS_EQUAL);

  /* initialise texture settings */
  gral_disable_texture_units_from (0);

  gral_set_scene_blending (GRAL_SCENE_BLEND_FACTOR_SOURCE_ALPHA,
                           GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_SOURCE_ALPHA);
}
