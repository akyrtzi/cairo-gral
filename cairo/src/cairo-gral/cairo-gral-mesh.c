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
#include <float.h>

void
_cairo_gral_mesh_init (cairo_gral_mesh_t          *mesh,
                       cairo_gral_vertex_pos_t    *vertices,
                       cairo_gral_tex_coord3_t    *tex_coords,
                       cairo_gral_vertex_index_t  *indices,
                       gral_vertex_buffer_t       *vertex_buf_pos,
                       gral_vertex_buffer_t       *vertex_buf_tex,
                       gral_index_buffer_t        *index_buf,
                       gral_vertex_data_t         *vertex_data,
                       gral_index_data_t          *index_data)
{
  mesh->op.operation_type = GRAL_RENDER_OPERATION_TYPE_TRIANGLE_LIST;
  mesh->op.vertex_data = vertex_data;
  mesh->op.index_data = index_data;
  mesh->op.use_indexes = TRUE;

  mesh->vertex_buf_pos = vertex_buf_pos;
  mesh->vertex_buf_tex = vertex_buf_tex;
  mesh->index_buf = index_buf;

  mesh->vertices = vertices;
  mesh->tex_coords = tex_coords;
  mesh->indices = indices;

  mesh->num_vertices = mesh->num_indices = 0;
  mesh->box.min_x = mesh->box.min_y = FLT_MAX;
  mesh->box.max_x = mesh->box.max_y = FLT_MIN;

  _cairo_gral_splines_buffer_init (&mesh->splines);
}

void
_cairo_gral_mesh_fini (cairo_gral_mesh_t *mesh)
{
  _cairo_gral_splines_buffer_fini (&mesh->splines);
}

cairo_gral_vertex_index_t
_cairo_gral_mesh_add_vertex_float (cairo_gral_mesh_t *mesh,
                                   float x, float y)
{
  if (x < mesh->box.min_x) mesh->box.min_x = x;
  if (y < mesh->box.min_y) mesh->box.min_y = y;
  if (x > mesh->box.max_x) mesh->box.max_x = x;
  if (y > mesh->box.max_y) mesh->box.max_y = y;

  assert(mesh->num_vertices < CAIRO_GRAL_MAX_VERTICES);
  mesh->vertices[mesh->num_vertices].x = x;
  mesh->vertices[mesh->num_vertices].y = y;
  mesh->vertices[mesh->num_vertices].z = CAIRO_GRAL_Z_VALUE;
  ++mesh->num_vertices;
  return mesh->num_vertices-1;
}

cairo_gral_vertex_index_t
_cairo_gral_mesh_add_vertex_pos_and_tex (cairo_gral_mesh_t       *mesh,
                                         float                    x,
                                         float                    y,
                                         cairo_gral_tex_coord3_t *tex_coord)
{
  cairo_gral_vertex_index_t index;

  assert (mesh->tex_coords);

  index = _cairo_gral_mesh_add_vertex_float (mesh, x, y);
  mesh->tex_coords[index] = *tex_coord;
  return index;
}

void
_cairo_gral_mesh_add_index (cairo_gral_mesh_t *mesh,
                            cairo_gral_vertex_index_t *pindex)
{
  cairo_gral_vertex_index_t index = *pindex;

  if (index >= mesh->num_vertices) {
    /* The contents of the vertices/indices buffers were rendered and now the
     * index refers to an invalid vertex. Copy the vertex that the index was
     * pointing (the vertices buffer doesn't get cleared after a rendering)
     * and set the index to point to the newly copied vertex.
     */
    if (mesh->tex_coords) {
      index = _cairo_gral_mesh_add_vertex_pos_and_tex (mesh,
                                                       mesh->vertices[index].x,
                                                       mesh->vertices[index].y,
                                                       &mesh->tex_coords[index]);
    } else {
      index = _cairo_gral_mesh_add_vertex_float (mesh,
                                                 mesh->vertices[index].x,
                                                 mesh->vertices[index].y);
    }

    *pindex = index;
  }

  assert(mesh->num_indices < CAIRO_GRAL_MAX_INDICES);
  mesh->indices[mesh->num_indices++] = index;

  if (mesh->num_indices < CAIRO_GRAL_MAX_INDICES)
    return;

  /* Index buffer is full */
  _cairo_gral_mesh_render (mesh);
}

void
_cairo_gral_mesh_render (cairo_gral_mesh_t *mesh)
{
  gral_vertex_buffer_t *vbuf;
  gral_index_buffer_t *ibuf;
  size_t length;
  void *dat;

  if (mesh->num_indices < 3)
    goto FINISHED_RENDER;

  assert(mesh->num_indices % 3 == 0);

  vbuf = mesh->vertex_buf_pos;
  length = sizeof(cairo_gral_vertex_pos_t) * mesh->num_vertices;
  assert(length <= gral_vertex_buffer_get_size (vbuf));
  dat = gral_vertex_buffer_lock (vbuf, 0, length, GRAL_BUFFER_LOCK_OPTION_DISCARD);
  memcpy(dat, mesh->vertices, length);
  gral_vertex_buffer_unlock (vbuf);

  if (mesh->tex_coords) {
    assert(mesh->vertex_buf_tex);

    vbuf = mesh->vertex_buf_tex;
    length = sizeof(cairo_gral_tex_coord3_t) * mesh->num_vertices;
    assert(length <= gral_vertex_buffer_get_size (vbuf));
    dat = gral_vertex_buffer_lock (vbuf, 0, length, GRAL_BUFFER_LOCK_OPTION_DISCARD);
    memcpy(dat, mesh->tex_coords, length);
    gral_vertex_buffer_unlock (vbuf);
  }

  ibuf = mesh->index_buf;
  length = sizeof(cairo_gral_vertex_index_t) * mesh->num_indices;
  assert(length <= gral_index_buffer_get_size (ibuf));
  dat = gral_index_buffer_lock (ibuf, 0, length, GRAL_BUFFER_LOCK_OPTION_DISCARD);
  memcpy(dat, mesh->indices, length);
  gral_index_buffer_unlock (ibuf);

  gral_vertex_data_set_count (mesh->op.vertex_data, mesh->num_vertices);
  gral_index_data_set_count (mesh->op.index_data, mesh->num_indices);

  gral_render (&mesh->op);

FINISHED_RENDER:
  mesh->num_vertices = mesh->num_indices = 0;
}
