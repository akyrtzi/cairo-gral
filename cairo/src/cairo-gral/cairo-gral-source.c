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

#define GRAL_COLOR_FROM_CAIRO_COLOR(gc,cc) \
  do {                                     \
    (gc).r = (float)(cc).red;              \
    (gc).g = (float)(cc).green;            \
    (gc).b = (float)(cc).blue;             \
    (gc).a = (float)(cc).alpha;            \
  } while (0)

static gral_argb_t
_cairo_gral_cairo_color_to_argb (const cairo_color_t *c) {
  gral_color_t gc;
  GRAL_COLOR_FROM_CAIRO_COLOR (gc, *c);
  return gral_color_to_argb (&gc);
}

static void
_cairo_gral_prepare_color_ramp_tex_state (cairo_gral_surface_t     *gsurface,
                                          cairo_gradient_pattern_t *pat,
                                          size_t unit);

static void
_cairo_gral_set_linear_source(cairo_gral_surface_t   *gsurface,
                              cairo_linear_pattern_t *pat)
{
  cairo_gral_vector2_t center;
  cairo_gral_vector2_t pos2;
  cairo_gral_vector2_t dir;
  double dir_len;

  gral_matrix_t mat;
  gral_matrix_t cairo_matrix;
  
  VECTOR2_FROM_POINT (center, pat->p1);
  VECTOR2_FROM_POINT (pos2, pat->p2);
  VECTOR2_SUB (dir, pos2, center);
  dir_len = _cairo_gral_vector2_normalize (&dir);
  VECTOR2_DIV_SCALAR (dir, dir, dir_len);

  mat.m[0][0] = dir.x,  mat.m[0][1] = dir.y, mat.m[0][2] = 0;
  mat.m[1][0] = -dir.y, mat.m[1][1] = dir.x, mat.m[1][2] = 0;
  mat.m[2][0] = 0,      mat.m[2][1] = 0,     mat.m[2][2] = 0;
  mat.m[3][0] = 0,      mat.m[3][1] = 0,     mat.m[3][2] = 0;

  gral_matrix_set_translate (&mat,
            mat.m[0][0]*(-center.x) + mat.m[0][1]*(-center.y),
            mat.m[1][0]*(-center.x) + mat.m[1][1]*(-center.y),
            mat.m[2][0]*(-center.x) + mat.m[2][1]*(-center.y) );
  mat.m[3][3] = 1;

  _cairo_gral_matrix_from_cairo_matrix (&cairo_matrix, &pat->base.base.matrix);
  gral_matrix_multiply (&mat, &mat, &cairo_matrix);

  /* get y,z coordinates to always be 0 */
  mat.m[1][0] = mat.m[1][1] = mat.m[1][2] = mat.m[1][3] = 0;
  mat.m[2][0] = mat.m[2][1] = mat.m[2][2] = mat.m[2][3] = 0;
  mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0, mat.m[3][3] = 1;

  gral_disable_texture_units_from (1);
  _cairo_gral_prepare_color_ramp_tex_state (gsurface, &pat->base, 0/*unit*/);
  gral_set_texture_matrix (0/*unit*/, &mat, 3);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_VERTEX);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_FRAGMENT);
}

static void
_cairo_gral_set_radial_source(cairo_gral_surface_t   *gsurface,
                              cairo_radial_pattern_t *pat)
{
  gral_cg_program_t *prog;
  cairo_gral_vector2_t center;
  cairo_gral_vector2_t circle2_pos;
  gral_matrix_t mat;

  if (gsurface->gpu->radial_shader == NULL) {
     gsurface->gpu->radial_shader =
          _cairo_gral_load_fragment_program ("fp_radial_gradient", "ps_2_0 arbfp1");
     assert(gsurface->gpu->radial_shader);
  }
  prog = gsurface->gpu->radial_shader;

  VECTOR2_FROM_POINT (center, pat->c1);
  VECTOR2_FROM_POINT (circle2_pos, pat->c2);
  VECTOR2_SUB (circle2_pos, circle2_pos, center);

  _cairo_gral_matrix_from_cairo_matrix (&mat, &pat->base.base.matrix);
  gral_matrix_set_translate (&mat, -center.x, -center.y, 0);

  {
    float param_rad1 = (float)_cairo_fixed_to_double(pat->r1);
    float param_rad2 = (float)_cairo_fixed_to_double(pat->r2);
    gral_cg_program_set_constant_matrix (prog, "matrix", &mat);
    gral_cg_program_set_constant_float (prog, "circle2_posx", (float)circle2_pos.x);
    gral_cg_program_set_constant_float (prog, "circle2_posy", (float)circle2_pos.y);
    gral_cg_program_set_constant_float (prog, "rad1", param_rad1);
    gral_cg_program_set_constant_float (prog, "rad2", param_rad2);
  }

  gral_disable_texture_units_from (1);
  _cairo_gral_prepare_color_ramp_tex_state (gsurface, &pat->base, 0/*unit*/);
  gral_cg_program_bind (prog);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_VERTEX);
}

static void
_cairo_gral_set_solid_source(cairo_gral_surface_t  *gsurface,
                             cairo_solid_pattern_t *source)
{
  gral_color_t col;
  GRAL_COLOR_FROM_CAIRO_COLOR (col, source->color);
  gral_set_lighting_enabled (TRUE);
  gral_set_surface_params (GRAL_COLOR_ZERO, &col, GRAL_COLOR_ZERO, &col,
                           0/*shininess*/, GRAL_TRACK_VERTEX_COLOR_TYPE_NONE);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_VERTEX);
  gral_unbind_gpu_program (GRAL_GPU_PROGRAM_TYPE_FRAGMENT);
}

cairo_int_status_t
_cairo_gral_set_source (cairo_gral_surface_t  *gsurface,
                        const cairo_pattern_t *source)
{
  switch (source->type) {
  case CAIRO_PATTERN_TYPE_LINEAR:
    _cairo_gral_set_linear_source (gsurface, (cairo_linear_pattern_t *)source);
    break;

  case CAIRO_PATTERN_TYPE_RADIAL:
    _cairo_gral_set_radial_source (gsurface, (cairo_radial_pattern_t *)source);
    break;

  case CAIRO_PATTERN_TYPE_SURFACE:
    fprintf(stderr, "CAIRO_PATTERN_TYPE_SURFACE not supported as source yet");
    return CAIRO_INT_STATUS_UNSUPPORTED;

  case CAIRO_PATTERN_TYPE_SOLID:
    _cairo_gral_set_solid_source(gsurface, (cairo_solid_pattern_t *)source);
    break;
  }

  return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_gral_prepare_color_ramp_texture(cairo_gral_surface_t     *gsurface,
                                       cairo_gradient_pattern_t *pat)
{
  gral_argb_t *dat;
  size_t i, first_pix, last_pix;

  if (gsurface->gpu->gral_tex == NULL) {
    gsurface->gpu->gral_tex = gral_texture_create (
          GRAL_TEX_TYPE_1D,
          CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH, /*width*/
          1, /*height*/
          1, /*depth*/
          0, /*num_mips*/
          GRAL_PIXEL_FORMAT_BYTE_BGRA,
          GRAL_TEXTURE_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE,
          FALSE, /*hw_gamma_correction*/
          0 /*fsaa*/);
    assert (gsurface->gpu->gral_tex);
  }

  dat = gral_texture_buffer_lock_full (gsurface->gpu->gral_tex,
                  0/*face*/,  0/*mipmap*/, GRAL_BUFFER_LOCK_OPTION_DISCARD);

  assert(pat->n_stops != 0);

  first_pix = (size_t)(pat->stops[0].offset * CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH);
  last_pix = (size_t)(pat->stops[pat->n_stops-1].offset * CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH);
  if (first_pix == CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH)
    --first_pix;
  if (last_pix == CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH)
    --last_pix;

  assert(first_pix >= 0 && first_pix < CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH);
  assert(last_pix >= 0 && last_pix < CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH);

  switch (pat->base.extend) {
    default: ASSERT_NOT_REACHED;

    case CAIRO_EXTEND_NONE:
      for (i=0; i < first_pix; ++i)
        dat[i] = 0;
      for (i=last_pix+1; i < CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH; ++i)
        dat[i] = 0;
      break;

    case CAIRO_EXTEND_PAD:
    case CAIRO_EXTEND_REFLECT: {
      gral_argb_t col = _cairo_gral_cairo_color_to_argb(&pat->stops[0].color);
      for (i=0; i < first_pix; ++i)
        dat[i] = col;
      col = _cairo_gral_cairo_color_to_argb(&pat->stops[pat->n_stops-1].color);
      for (i=last_pix+1; i < CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH; ++i)
        dat[i] = col;
      break;
    }

    case CAIRO_EXTEND_REPEAT: {
      size_t left_pad = first_pix;
      size_t right_pad = CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH - last_pix - 1;
      float total_pad = (float)(left_pad + right_pad);
      gral_color_t left_col, right_col;
      GRAL_COLOR_FROM_CAIRO_COLOR (left_col, pat->stops[0].color);      
      GRAL_COLOR_FROM_CAIRO_COLOR (right_col, pat->stops[pat->n_stops-1].color);
      for (i=0; i < first_pix; ++i) {
        float t = ((float)(first_pix - i))/total_pad;
        gral_color_t col;
        _cairo_gral_color_lerp (&col, &left_col, &right_col, t);
        dat[i] = gral_color_to_argb (&col);
      }
      for (i=last_pix+1; i < CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH; ++i) {
        float t = ((float)(i - last_pix))/total_pad;
        gral_color_t col;
        _cairo_gral_color_lerp (&col, &right_col, &left_col, t);
        dat[i] = gral_color_to_argb (&col);
      }
    }
  }

  {
    gral_color_t left_col, right_col;
    size_t left_pix, right_pix, stopi, pixi;

    GRAL_COLOR_FROM_CAIRO_COLOR (left_col, pat->stops[0].color);
    left_pix = first_pix;
    dat[left_pix] = gral_color_to_argb (&left_col);

    for (stopi=1; stopi < pat->n_stops; ++stopi) {
      GRAL_COLOR_FROM_CAIRO_COLOR (right_col, pat->stops[stopi].color);
      right_pix = (size_t)(pat->stops[stopi].offset * CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH);
      if (right_pix == CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH)
        --right_pix;
      for (pixi = left_pix+1; pixi <= right_pix; ++pixi) {
        float t = ((float)(pixi-left_pix)) / (right_pix-left_pix);
        gral_color_t col;
        _cairo_gral_color_lerp (&col, &left_col, &right_col, t);
        dat[pixi] = gral_color_to_argb (&col);
      }
      
      left_col = right_col;
      left_pix = right_pix;
    }
  }

  gral_texture_buffer_unlock (gsurface->gpu->gral_tex, 0/*face*/, 0/*mipmap*/);
}

static gral_texture_addressing_mode_t
_cairo_gral_get_texture_addressing (cairo_extend_t ext)
{
  switch (ext) {
    default: ASSERT_NOT_REACHED;
    case CAIRO_EXTEND_NONE:
      // FIXME: Works on OpenGL but on Direct3D it seems like the border color is
      // blended inside the drawing area.
      return GRAL_TEXTURE_ADDRESSING_MODE_BORDER;
    case CAIRO_EXTEND_PAD:
      return GRAL_TEXTURE_ADDRESSING_MODE_CLAMP;
    case CAIRO_EXTEND_REPEAT:
      return GRAL_TEXTURE_ADDRESSING_MODE_WRAP;
    case CAIRO_EXTEND_REFLECT:
      return GRAL_TEXTURE_ADDRESSING_MODE_MIRROR;
  }
}

static void
_cairo_gral_prepare_color_ramp_tex_state (cairo_gral_surface_t     *gsurface,
                                          cairo_gradient_pattern_t *pat,
                                          size_t unit)
{
  _cairo_gral_prepare_color_ramp_texture(gsurface, pat);
  assert(gsurface->gpu->gral_tex);
  gral_set_texture (unit, TRUE/*enabled*/, gsurface->gpu->gral_tex);
  gral_set_texture_coord_set (unit, 0);
  gral_set_texture_unit_filtering (unit, GRAL_FILTER_OPTION_LINEAR,
                                      GRAL_FILTER_OPTION_LINEAR, GRAL_FILTER_OPTION_POINT);
  gral_set_texture_layer_anisotropy (unit, 1);
  gral_set_texture_mipmap_bias (unit, 0);

  {
    gral_layer_blend_mode_t color_bm;
    gral_layer_blend_mode_t alpha_bm;
    color_bm.blend_type = GRAL_LAYER_BLEND_TYPE_COLOR;
    alpha_bm.blend_type = GRAL_LAYER_BLEND_TYPE_ALPHA;
    color_bm.operation = alpha_bm.operation = GRAL_LAYER_BLEND_OPERATION_MODULATE;
    color_bm.source1 = alpha_bm.source1 = GRAL_LAYER_BLEND_SOURCE_TEXTURE;
    color_bm.source2 = alpha_bm.source2 = GRAL_LAYER_BLEND_SOURCE_CURRENT;
    gral_set_texture_blend_mode (unit, &color_bm);
    gral_set_texture_blend_mode (unit, &alpha_bm);
  }
  
  {
    gral_uvw_addressing_mode_t uvw;
    uvw.u = uvw.v = uvw.w = _cairo_gral_get_texture_addressing(pat->base.extend);
    gral_set_texture_addressing_mode (unit, &uvw);

    if (uvw.u == GRAL_TEXTURE_ADDRESSING_MODE_BORDER) {
      // FIXME: doesn't seem to work as expected in D3D. The color applies inside the drawing area too.
      gral_set_texture_border_color (unit, GRAL_COLOR_BLACK);
    }
  }

  gral_set_texture_coord_calculation (unit, GRAL_TEX_COORD_CALC_METHOD_NONE);
}
