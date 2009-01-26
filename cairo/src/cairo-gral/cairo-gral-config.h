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

#ifndef CAIRO_GRAL_CONFIG_H
#define CAIRO_GRAL_CONFIG_H

#define CAIRO_GRAL_MAX_TRIGS     2048
#define CAIRO_GRAL_MAX_VERTICES  CAIRO_GRAL_MAX_TRIGS*3
#define CAIRO_GRAL_MAX_INDICES   CAIRO_GRAL_MAX_TRIGS*3

#define CAIRO_GRAL_COLOR_RAMP_TEX_WIDTH 1024

#define CAIRO_GRAL_Z_VALUE 0

/* #define CAIRO_GRAL_DISABLE_FRAGMENT_SHADERS 1 */
#define CAIRO_GRAL_DISABLE_GPU_SPLINE_RENDERING 1

/* If CAIRO_GRAL_EMBED_SHADER_SOURCE is set, the shaders will be loaded by
 * the shader source that is embedded into the executable, otherwise
 * the shader file will be loaded from the current directory.
 */
#define CAIRO_GRAL_EMBED_SHADER_SOURCE 1

#endif /* CAIRO_GRAL_CONFIG_H */
