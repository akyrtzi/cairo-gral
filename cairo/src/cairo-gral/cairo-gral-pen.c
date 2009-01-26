/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2008 Chris Wilson
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
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "cairoint.h"
#include "cairo-gral-private.h"

static int
_cairo_pen_vertices_needed (double tolerance, double radius, cairo_matrix_t *matrix);

static void
_cairo_pen_compute_slopes (cairo_pen_t *pen);

/*
The circular pen in user space is transformed into an ellipse in
device space.

We construct the pen by computing points along the circumference
using equally spaced angles.

We show that this approximation to the ellipse has maximum error at the
major axis of the ellipse.

Set

	    M = major axis length
	    m = minor axis length

Align 'M' along the X axis and 'm' along the Y axis and draw
an ellipse parameterized by angle 't':

	    x = M cos t			y = m sin t

Perturb t by ± d and compute two new points (x+,y+), (x-,y-).
The distance from the average of these two points to (x,y) represents
the maximum error in approximating the ellipse with a polygon formed
from vertices 2∆ radians apart.

	    x+ = M cos (t+∆)		y+ = m sin (t+∆)
	    x- = M cos (t-∆)		y- = m sin (t-∆)

Now compute the approximation error, E:

	Ex = (x - (x+ + x-) / 2)
	Ex = (M cos(t) - (Mcos(t+∆) + Mcos(t-∆))/2)
	   = M (cos(t) - (cos(t)cos(∆) + sin(t)sin(∆) +
			  cos(t)cos(∆) - sin(t)sin(∆))/2)
	   = M(cos(t) - cos(t)cos(∆))
	   = M cos(t) (1 - cos(∆))

	Ey = y - (y+ - y-) / 2
	   = m sin (t) - (m sin(t+∆) + m sin(t-∆)) / 2
	   = m (sin(t) - (sin(t)cos(∆) + cos(t)sin(∆) +
			  sin(t)cos(∆) - cos(t)sin(∆))/2)
	   = m (sin(t) - sin(t)cos(∆))
	   = m sin(t) (1 - cos(∆))

	E² = Ex² + Ey²
	   = (M cos(t) (1 - cos (∆)))² + (m sin(t) (1-cos(∆)))²
	   = (1 - cos(∆))² (M² cos²(t) + m² sin²(t))
	   = (1 - cos(∆))² ((m² + M² - m²) cos² (t) + m² sin²(t))
	   = (1 - cos(∆))² (M² - m²) cos² (t) + (1 - cos(∆))² m²

Find the extremum by differentiation wrt t and setting that to zero

∂(E²)/∂(t) = (1-cos(∆))² (M² - m²) (-2 cos(t) sin(t))

         0 = 2 cos (t) sin (t)
	 0 = sin (2t)
	 t = nπ

Which is to say that the maximum and minimum errors occur on the
axes of the ellipse at 0 and π radians:

	E²(0) = (1-cos(∆))² (M² - m²) + (1-cos(∆))² m²
	      = (1-cos(∆))² M²
	E²(π) = (1-cos(∆))² m²

maximum error = M (1-cos(∆))
minimum error = m (1-cos(∆))

We must make maximum error ≤ tolerance, so compute the ∆ needed:

	    tolerance = M (1-cos(∆))
	tolerance / M = 1 - cos (∆)
	       cos(∆) = 1 - tolerance/M
                    ∆ = acos (1 - tolerance / M);

Remembering that ∆ is half of our angle between vertices,
the number of vertices is then

             vertices = ceil(2π/2∆).
                      = ceil(π/∆).

Note that this also equation works for M == m (a circle) as it
doesn't matter where on the circle the error is computed.
*/

static int
_cairo_pen_vertices_needed (double	    tolerance,
			    double	    radius,
			    cairo_matrix_t  *matrix)
{
    /*
     * the pen is a circle that gets transformed to an ellipse by matrix.
     * compute major axis length for a pen with the specified radius.
     * we don't need the minor axis length.
     */

    double  major_axis = _cairo_matrix_transformed_circle_major_axis(matrix, radius);

    /*
     * compute number of vertices needed
     */
    int	    num_vertices;

    /* Where tolerance / M is > 1, we use 4 points */
    if (tolerance >= major_axis) {
	num_vertices = 4;
    } else {
	double delta = acos (1 - tolerance / major_axis);
	num_vertices = ceil (M_PI / delta);

	/* number of vertices must be even */
	if (num_vertices % 2)
	    num_vertices++;

	/* And we must always have at least 4 vertices. */
	if (num_vertices < 4)
	    num_vertices = 4;
    }

    return num_vertices;
}

static void
_cairo_pen_compute_slopes (cairo_pen_t *pen)
{
    int i, i_prev;
    cairo_pen_vertex_t *prev, *v, *next;

    for (i=0, i_prev = pen->num_vertices - 1;
	 i < pen->num_vertices;
	 i_prev = i++) {
	prev = &pen->vertices[i_prev];
	v = &pen->vertices[i];
	next = &pen->vertices[(i + 1) % pen->num_vertices];

	_cairo_slope_init (&v->slope_cw, &prev->point, &v->point);
	_cairo_slope_init (&v->slope_ccw, &v->point, &next->point);
    }
}

static cairo_status_t
_cairo_pen_stroke_spline_add_convolved_points (cairo_gral_pen_stroke_spline_t	*stroker,
					      const cairo_slope_t *slope)
{
    cairo_slope_t forward_slope = *slope;
    cairo_slope_t backward_slope = { -slope->dx, -slope->dy };
    cairo_bool_t extend_forward = TRUE;
    cairo_bool_t extend_backward = TRUE;
  
    do {
        cairo_status_t status;

        if (extend_forward) {
	    stroker->forward_hull_point.x =
                stroker->last_point.x + stroker->pen.vertices[stroker->forward_vertex].point.x;
	    stroker->forward_hull_point.y =
                stroker->last_point.y + stroker->pen.vertices[stroker->forward_vertex].point.y;
        }
        if (extend_backward) {
	    stroker->backward_hull_point.x =
                stroker->last_point.x + stroker->pen.vertices[stroker->backward_vertex].point.x;
	    stroker->backward_hull_point.y =
                stroker->last_point.y + stroker->pen.vertices[stroker->backward_vertex].point.y;
        }

	status = _cairo_gral_path_stroke_spline_extend (stroker->mesh,
	                &stroker->forward_hull_point, &stroker->backward_hull_point);
        if (unlikely (status))
            return status;

	/* The strict inequalities here ensure that if a spline slope
	 * compares identically with either of the slopes of the
	 * active vertex, then it remains the active vertex. This is
	 * very important since otherwise we can trigger an infinite
	 * loop in the case of a degenerate pen, (a line), where
	 * neither vertex considers itself active for the slope---one
	 * will consider it as equal and reject, and the other will
	 * consider it unequal and reject. This is due to the inherent
	 * ambiguity when comparing slopes that differ by exactly
	 * pi. */
        if (extend_forward) {
	    if (_cairo_slope_compare (&forward_slope,
				      &stroker->pen.vertices[stroker->forward_vertex].slope_ccw) > 0)
	    {
	        if (++stroker->forward_vertex == stroker->pen.num_vertices)
		    stroker->forward_vertex = 0;
	    }
	    else if (_cairo_slope_compare (&forward_slope,
				           &stroker->pen.vertices[stroker->forward_vertex].slope_cw) < 0)
	    {
	        if (--stroker->forward_vertex == -1)
		    stroker->forward_vertex = stroker->pen.num_vertices - 1;
	    }
	    else
	    {
                extend_forward = FALSE;
	    }
        }

        if (extend_backward) {
	    if (_cairo_slope_compare (&backward_slope,
				      &stroker->pen.vertices[stroker->backward_vertex].slope_ccw) > 0)
	    {
	        if (++stroker->backward_vertex == stroker->pen.num_vertices)
		    stroker->backward_vertex = 0;
	    }
	    else if (_cairo_slope_compare (&backward_slope,
				           &stroker->pen.vertices[stroker->backward_vertex].slope_cw) < 0)
	    {
	        if (--stroker->backward_vertex == -1)
		    stroker->backward_vertex = stroker->pen.num_vertices - 1;
	    }
	    else
	    {
                extend_backward = FALSE;
	    }
        }
    } while (extend_forward || extend_backward);

    return CAIRO_STATUS_SUCCESS;
}


/* Compute outline of a given spline using the pen.
 */
cairo_status_t
_cairo_gral_pen_stroke_spline (cairo_gral_pen_stroke_spline_t	*stroker,
			       double tolerance)
{
    cairo_status_t status;
    cairo_slope_t slope;

    /* If the line width is so small that the pen is reduced to a
       single point, then we have nothing to do. */
    if (stroker->pen.num_vertices <= 1)
	return CAIRO_STATUS_SUCCESS;

    /* open the polygon */
    slope = stroker->spline.initial_slope;
    stroker->forward_vertex =
	_cairo_pen_find_active_cw_vertex_index (&stroker->pen, &slope);
    stroker->forward_hull_point.x = stroker->last_point.x +
	stroker->pen.vertices[stroker->forward_vertex].point.x;
    stroker->forward_hull_point.y = stroker->last_point.y +
	stroker->pen.vertices[stroker->forward_vertex].point.y;

    slope.dx = -slope.dx;
    slope.dy = -slope.dy;
    stroker->backward_vertex =
	_cairo_pen_find_active_cw_vertex_index (&stroker->pen, &slope);
    stroker->backward_hull_point.x = stroker->last_point.x +
	stroker->pen.vertices[stroker->backward_vertex].point.x;
    stroker->backward_hull_point.y = stroker->last_point.y +
	stroker->pen.vertices[stroker->backward_vertex].point.y;

    status = _cairo_gral_path_stroke_spline_open (stroker->mesh,
			                          &stroker->forward_hull_point,
			                          &stroker->backward_hull_point);
    if (unlikely (status))
	return status;

    status = _cairo_spline_decompose (&stroker->spline, tolerance);
    if (unlikely (status))
	return status;

    /* close the polygon */
    slope = stroker->spline.final_slope;
    status = _cairo_pen_stroke_spline_add_convolved_points (stroker, &slope);
    if (unlikely (status))
        return status;

    status = _cairo_gral_path_stroke_spline_close (stroker->mesh);
    return status;
}

static cairo_status_t
_cairo_pen_stroke_spline_add_point (void		    *closure,
				    const cairo_point_t	    *point)
{
    cairo_gral_pen_stroke_spline_t	*stroker = closure;
    cairo_slope_t slope;
    cairo_status_t status;

    _cairo_slope_init (&slope, &stroker->last_point, point);
    status = _cairo_pen_stroke_spline_add_convolved_points (stroker, &slope);

    stroker->last_point = *point;

    return status;
}

cairo_int_status_t
_cairo_gral_pen_stroke_spline_init (cairo_gral_pen_stroke_spline_t *stroker,
			       const cairo_pen_t *pen,
			       const cairo_point_t *a,
			       const cairo_point_t *b,
			       const cairo_point_t *c,
			       const cairo_point_t *d,
                               cairo_gral_stroke_path_mesh_t *mesh)
{
    cairo_int_status_t status;

    if (! _cairo_spline_init (&stroker->spline,
			      _cairo_pen_stroke_spline_add_point,
			      stroker,
			      a, b, c, d))
    {
	return CAIRO_INT_STATUS_DEGENERATE;
    }

    status = _cairo_pen_init_copy (&stroker->pen, pen);
    if (unlikely (status))
	return status;

    stroker->mesh = mesh;

    stroker->last_point = *a;

    return CAIRO_STATUS_SUCCESS;
}

void
_cairo_gral_pen_stroke_spline_fini (cairo_gral_pen_stroke_spline_t *stroker)
{
    _cairo_pen_fini (&stroker->pen);
}
