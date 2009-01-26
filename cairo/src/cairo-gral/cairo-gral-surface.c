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
#include "cairo-gral.h"

static cairo_status_t
_cairo_gral_surface_finish (void *asurface)
{
  cairo_gral_surface_t *gsurface = asurface;
  _cairo_gral_gpu_resources_release (gsurface->gpu);

  return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_gral_surface_intersect_clip_path	(void                *asurface,
                                         cairo_path_fixed_t  *path,
                                         cairo_fill_rule_t    fill_rule,
                                         double               tolerance,
                                         cairo_antialias_t    antialias)
{
  cairo_gral_surface_t *gsurface = asurface;
  float width, height;
  cairo_int_status_t status;

  if (path == NULL) {
    gsurface->has_clip = FALSE;
    return CAIRO_STATUS_SUCCESS;
  }

  if (!gsurface->has_clip) {
    gsurface->has_clip = TRUE;
    gral_clear_frame_buffer (GRAL_FRAME_BUFFER_TYPE_DEPTH | GRAL_FRAME_BUFFER_TYPE_STENCIL,
                             GRAL_COLOR_BLACK, 1.0f/*depth*/, 0/*stencil*/);
  }

  _cairo_gral_init_render_state (gsurface);

  gral_set_depth_buffer_write_enabled (FALSE);

  /* Tesselate into stencil */
  status = _cairo_gral_prepare_fill_stencil_mask (gsurface, path, fill_rule, tolerance, NULL);
  if (status)
    return status;

  gral_set_depth_buffer_write_enabled (TRUE);
  gral_set_color_buffer_write_enabled (FALSE, FALSE, FALSE, FALSE);

  /* Draw only outside of the fill path. */
  gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_EQUAL,
                                  0, 0xffffffff,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  FALSE);

  width = (float) gral_surface_get_width (gsurface->gral_surf);
  height = (float) gral_surface_get_height (gsurface->gral_surf);
  _cairo_gral_render_quad(gsurface, 0, 0, width, height);

  /* Reset state */
  gral_set_depth_buffer_write_enabled (FALSE);
  gral_set_color_buffer_write_enabled (TRUE, TRUE, TRUE, TRUE);
  gral_set_stencil_check_enabled (FALSE);

  return status;
}

static cairo_int_status_t
_cairo_gral_surface_get_extents	(void                  *asurface,
                                 cairo_rectangle_int_t *extents)
{
  cairo_gral_surface_t *gsurface = asurface;
  extents->x = extents->y = 0;
  extents->width = gral_surface_get_width(gsurface->gral_surf);
  extents->height = gral_surface_get_height(gsurface->gral_surf);
  return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_gral_surface_paint (void                   *asurface,
                           cairo_operator_t        op,
                           const cairo_pattern_t  *source,
                           cairo_rectangle_int_t  *extents)
{
  cairo_gral_surface_t *gsurface = asurface;
  float width, height;
  cairo_int_status_t status;

  if (op == CAIRO_OPERATOR_DEST)
    return CAIRO_STATUS_SUCCESS;

  _cairo_gral_init_render_state(gsurface);

  status = _cairo_gral_set_source(gsurface, source);
  if (status)
    return status;

  width = (float) gral_surface_get_width (gsurface->gral_surf);
  height = (float) gral_surface_get_height (gsurface->gral_surf);
  _cairo_gral_render_quad(gsurface, 0, 0, width, height);

  return status;
}

static cairo_int_status_t
_cairo_gral_surface_stroke (void                  *asurface,
                            cairo_operator_t       op,
                            const cairo_pattern_t *source,
                            cairo_path_fixed_t    *path,
                            cairo_stroke_style_t  *style,
                            cairo_matrix_t        *ctm,
                            cairo_matrix_t        *ctm_inverse,
                            double                 tolerance,
                            cairo_antialias_t      antialias,
                            cairo_rectangle_int_t *extents)
{
    cairo_gral_surface_t *gsurface = asurface;
    cairo_gral_bound_box_t box;
    cairo_int_status_t status;

    if (op == CAIRO_OPERATOR_DEST)
        return CAIRO_STATUS_SUCCESS;

    _cairo_gral_init_render_state(gsurface);

    /* Tesselate into stencil */
    status = _cairo_gral_prepare_stroke_stencil_mask (gsurface,
                                                      path,
                                                      style,
                                                      ctm,
                                                      ctm_inverse,
                                                      tolerance,
                                                      &box);
    if (status)
      return status;

    /* Draw paint where stencil not zero */
    gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_NOT_EQUAL,
                                    0, 0xffffffff,
                                    GRAL_STENCIL_OPERATION_ZERO,
                                    GRAL_STENCIL_OPERATION_ZERO,
                                    GRAL_STENCIL_OPERATION_ZERO,
                                    FALSE);
    gral_set_color_buffer_write_enabled (TRUE, TRUE, TRUE, TRUE);

    status = _cairo_gral_set_source(gsurface, source);
    if (status == CAIRO_STATUS_SUCCESS)
      _cairo_gral_render_quad(gsurface, box.min_x, box.min_y, box.max_x, box.max_y);

    /* Reset state */
    gral_set_stencil_check_enabled (FALSE);

    return status;
}

static cairo_int_status_t
_cairo_gral_surface_fill (void                  *asurface,
                          cairo_operator_t       op,
                          const cairo_pattern_t	*source,
                          cairo_path_fixed_t    *path,
                          cairo_fill_rule_t      fill_rule,
                          double                 tolerance,
                          cairo_antialias_t      antialias,
                          cairo_rectangle_int_t  *extents)
{
  cairo_gral_surface_t *gsurface = asurface;
  cairo_gral_bound_box_t box;
  cairo_int_status_t status;

  if (op == CAIRO_OPERATOR_DEST)
    return CAIRO_STATUS_SUCCESS;

  _cairo_gral_init_render_state(gsurface);

  /* Tesselate into stencil */
  status = _cairo_gral_prepare_fill_stencil_mask(gsurface, path, fill_rule, tolerance, &box);
  if (status)
    return status;

  /* Draw paint where stencil not zero */
  gral_set_stencil_buffer_params (GRAL_COMPARE_FUNC_NOT_EQUAL,
                                  0, 0xffffffff,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  GRAL_STENCIL_OPERATION_ZERO,
                                  FALSE /*two_sided_operation*/);
  gral_set_color_buffer_write_enabled (TRUE, TRUE, TRUE, TRUE);

  status = _cairo_gral_set_source(gsurface, source);
  if (status == CAIRO_STATUS_SUCCESS)
    _cairo_gral_render_quad(gsurface, box.min_x, box.min_y, box.max_x, box.max_y);

  /* Reset state */
  gral_set_stencil_check_enabled (FALSE);

  return status;
}

static const struct _cairo_surface_backend
_cairo_gral_surface_backend = {
    CAIRO_SURFACE_TYPE_GRAL,
    NULL, /* create_similar */
    _cairo_gral_surface_finish,
    NULL, /* acquire_source_image */
    NULL, /* release_source_image */
    NULL, /* acquire_dest_image */
    NULL, /* release_dest_image */
    NULL, /* clone_similar */
    NULL, /* composite */
    NULL, /* fill_rectangles */
    NULL, /* composite_trapezoids */
    NULL, /* create_span_renderer */
    NULL, /* check_span_renderer */
    NULL, /* copy_page */
    NULL, /* show_page */
    NULL, /* set_clip_region */
    _cairo_gral_surface_intersect_clip_path,
    _cairo_gral_surface_get_extents,
    NULL, /* old_show_glyphs */
    NULL, /* get_font_options */
    NULL, /* flush */
    NULL, /* mark_dirty_rectangle */
    NULL, /* scaled_font_fini */
    NULL, /* scaled_glyph_fini */

    _cairo_gral_surface_paint,
    NULL, /* mask */
    _cairo_gral_surface_stroke,
    _cairo_gral_surface_fill,
    NULL, /* show_glyphs */

    NULL, /* snapshot */
    NULL, /* is_similar */
    NULL, /* reset */
};

cairo_surface_t *
cairo_gral_surface_create (gral_surface_t *gral_surf)
{
  cairo_gral_surface_t *s;

  s = (cairo_gral_surface_t *) malloc(sizeof(cairo_gral_surface_t));
  memset(s, 0, sizeof(cairo_gral_surface_t));
  _cairo_surface_init(&s->base, &_cairo_gral_surface_backend, CAIRO_CONTENT_COLOR_ALPHA);

  s->gral_surf = gral_surf;
  s->gpu = _cairo_gral_gpu_resources_acquire ();

  return (cairo_surface_t *) s;
}

static cairo_bool_t
_cairo_surface_is_gral (const cairo_surface_t *surface)
{
  return surface->backend == &_cairo_gral_surface_backend;
}

int
cairo_gral_surface_get_width (cairo_surface_t *surface)
{
  cairo_gral_surface_t *gral_surface = (cairo_gral_surface_t *) surface;

  if (! _cairo_surface_is_gral (surface)) {
    _cairo_error_throw (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    return 0;
  }

  return gral_surface_get_width (gral_surface->gral_surf);
}

int
cairo_gral_surface_get_height (cairo_surface_t *surface)
{
  cairo_gral_surface_t *gral_surface = (cairo_gral_surface_t *) surface;

  if (! _cairo_surface_is_gral (surface)) {
    _cairo_error_throw (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    return 0;
  }

  return gral_surface_get_height (gral_surface->gral_surf);
}
