/* Copyright (c) 2009, Argiris Kirtzidis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ARGIRIS KIRTZIDIS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ARGIRIS KIRTZIDIS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GRAL_H_
#define _GRAL_H_

#include <stddef.h>

#ifdef  __cplusplus
# define GRAL_BEGIN_DECLS  extern "C" {
# define GRAL_END_DECLS    }
#else
# define GRAL_BEGIN_DECLS
# define GRAL_END_DECLS
#endif

#ifndef gral_public
# if defined (_MSC_VER) && ! defined (GRAL_WIN32_STATIC_BUILD)
#  define gral_public __declspec(dllimport)
# else
#  define gral_public
# endif
#endif

/*
 * Standard integers
 */
#if defined (_SVR4) || defined (SVR4) || defined (__OpenBSD__) || defined (_sgi) || defined (__sun) || defined (sun)
#  include <inttypes.h>
#elif defined (_MSC_VER)
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#elif defined (_AIX)
#  include <sys/inttypes.h>
#else
#  include <stdint.h>
#endif

GRAL_BEGIN_DECLS

typedef int gral_bool_t;

typedef uint32_t gral_argb_t;
typedef uint32_t gral_abgr_t;

typedef struct _gral_surface gral_surface_t;
typedef struct _gral_texture gral_texture_t;
typedef struct _gral_vertex_data gral_vertex_data_t;
typedef struct _gral_index_data gral_index_data_t;
typedef struct _gral_vertex_buffer gral_vertex_buffer_t;
typedef struct _gral_index_buffer gral_index_buffer_t;
typedef struct _gral_cg_program gral_cg_program_t;

typedef struct _gral_matrix {
  /// The matrix entries, indexed by [row][col].
  union {
    float m[4][4];
    float _m[16];
  };
} gral_matrix_t;

typedef struct _gral_color {
  float r, g, b, a;
} gral_color_t;

typedef enum {
  GRAL_STOCK_WHITE,
  GRAL_STOCK_BLACK,
  GRAL_STOCK_ZERO
} gral_stock_t;

gral_public const gral_color_t *
gral_stock_color (gral_stock_t stock);

#define GRAL_COLOR_WHITE       gral_stock_color (GRAL_STOCK_WHITE)
#define GRAL_COLOR_BLACK       gral_stock_color (GRAL_STOCK_BLACK)
#define GRAL_COLOR_ZERO        gral_stock_color (GRAL_STOCK_ZERO)

#define gral_color_init(c,red,green,blue,alpha)                   \
  do {                                                            \
    (c)->r = red, (c)->g = green, (c)->b = blue, (c)->a = alpha;  \
  } while (0)

gral_public gral_argb_t
gral_color_to_argb (gral_color_t *col);

gral_public gral_abgr_t
gral_color_to_abgr (gral_color_t *col);

gral_public const gral_matrix_t *
gral_matrix_identity (void);

#define GRAL_MATRIX_IDENTITY gral_matrix_identity ()

#define gral_matrix_set_translate(matrix, x, y, z) \
  do {                                             \
    (matrix)->m[0][3] = x;                         \
    (matrix)->m[1][3] = y;                         \
    (matrix)->m[2][3] = z;                         \
  } while (0)

#define gral_matrix_set_scale(matrix, x, y, z) \
  do {                                         \
    (matrix)->m[0][0] = x;                     \
    (matrix)->m[1][1] = y;                     \
    (matrix)->m[2][2] = z;                     \
  } while (0)

gral_public void
gral_matrix_init (gral_matrix_t *matrix,
                  float m00, float m01, float m02, float m03,
                  float m10, float m11, float m12, float m13,
                  float m20, float m21, float m22, float m23,
                  float m30, float m31, float m32, float m33);

gral_public void
gral_matrix_init_identity (gral_matrix_t *matrix);

gral_public void
gral_matrix_translate (gral_matrix_t *matrix, float x, float y, float z);

gral_public void
gral_matrix_scale (gral_matrix_t *matrix, float x, float y, float z);

gral_public void
gral_matrix_init_translate (gral_matrix_t *matrix, float x, float y, float z);

gral_public void
gral_matrix_init_scale (gral_matrix_t *matrix, float x, float y, float z);

gral_public void
gral_matrix_multiply (gral_matrix_t *result,
                      const gral_matrix_t *a,
                      const gral_matrix_t *b);

gral_public int
gral_surface_get_width (gral_surface_t *surf);

gral_public int
gral_surface_get_height (gral_surface_t *surf);

gral_public void
gral_set_render_surface (gral_surface_t *surf);

gral_public void
gral_set_view_matrix (const gral_matrix_t *m);

gral_public void
gral_set_projection_matrix (const gral_matrix_t *m);

gral_public void
gral_set_world_matrix (const gral_matrix_t *m);

gral_public float
gral_get_horizontal_texel_offset (void);

gral_public float
gral_get_vertical_texel_offset (void);

#define GRAL_CAPS_VALUE(val) (1 << val)
typedef enum {
  GRAL_CAP_FRAGMENT_PROGRAM = GRAL_CAPS_VALUE(1)
} gral_capabilities_t;

gral_public gral_capabilities_t
gral_get_capabilities (void);

gral_public void
gral_set_lighting_enabled (gral_bool_t enabled);

typedef enum
{
  /// Hardware never culls triangles and renders everything it receives.
  GRAL_CULL_NONE,
  /// Hardware culls triangles whose vertices are listed clockwise in the view (default).
  GRAL_CULL_CLOCKWISE,
  /// Hardware culls triangles whose vertices are listed anticlockwise in the view.
  GRAL_CULL_ANTICLOCKWISE
} gral_culling_mode_t;

gral_public void
gral_set_culling_mode (gral_culling_mode_t mode);

typedef enum
{
  GRAL_GPU_PROGRAM_TYPE_VERTEX,
  GRAL_GPU_PROGRAM_TYPE_FRAGMENT,
  GRAL_GPU_PROGRAM_TYPE_GEOMETRY
} gral_gpu_program_type_t;

gral_public void
gral_unbind_gpu_program (gral_gpu_program_type_t gptype);

typedef enum
{
  GRAL_SHADE_TYPE_FLAT,
  GRAL_SHADE_TYPE_GOURAUD,
  GRAL_SHADE_TYPE_PHONG
} gral_shade_type_t;

gral_public void
gral_set_shading_type (gral_shade_type_t so);

/** An enumeration describing which material properties should track the vertex colours */
typedef enum TrackVertexColourEnum {
  GRAL_TRACK_VERTEX_COLOR_TYPE_NONE        = 0x0,
  GRAL_TRACK_VERTEX_COLOR_TYPE_AMBIENT     = 0x1,        
  GRAL_TRACK_VERTEX_COLOR_TYPE_DIFFUSE     = 0x2,
  GRAL_TRACK_VERTEX_COLOR_TYPE_SPECULAR    = 0x4,
  GRAL_TRACK_VERTEX_COLOR_TYPE_EMISSIVE    = 0x8
} gral_track_vertex_color_type_t;

gral_public void
gral_set_surface_params (const gral_color_t *ambient,
                         const gral_color_t *diffuse, const gral_color_t *specular,
                         const gral_color_t *emissive, float shininess,
                         gral_track_vertex_color_type_t tracking);

/** Comparison functions used for the depth/stencil buffer operations and 
  others. */
typedef enum {
  GRAL_COMPARE_FUNC_ALWAYS_FAIL,
  GRAL_COMPARE_FUNC_ALWAYS_PASS,
  GRAL_COMPARE_FUNC_LESS,
  GRAL_COMPARE_FUNC_LESS_EQUAL,
  GRAL_COMPARE_FUNC_EQUAL,
  GRAL_COMPARE_FUNC_NOT_EQUAL,
  GRAL_COMPARE_FUNC_GREATER_EQUAL,
  GRAL_COMPARE_FUNC_GREATER
} gral_compare_func_t;

gral_public void
gral_set_depth_buffer_params (gral_bool_t depthTest, gral_bool_t depthWrite,
                              gral_compare_func_t depthFunction);

gral_public void
gral_set_depth_buffer_write_enabled (gral_bool_t enabled);

gral_public void
gral_set_color_buffer_write_enabled (gral_bool_t red,
                                     gral_bool_t green,
                                     gral_bool_t blue,
                                     gral_bool_t alpha);

gral_public void
gral_set_stencil_check_enabled (gral_bool_t enabled);

/* Enum describing the various actions which can be taken onthe stencil buffer */
typedef enum {
  /// Leave the stencil buffer unchanged
  GRAL_STENCIL_OPERATION_KEEP,
  /// Set the stencil value to zero
  GRAL_STENCIL_OPERATION_ZERO,
  /// Set the stencil value to the reference value
  GRAL_STENCIL_OPERATION_REPLACE,
  /// Increase the stencil value by 1, clamping at the maximum value
  GRAL_STENCIL_OPERATION_INCREMENT,
  /// Decrease the stencil value by 1, clamping at 0
  GRAL_STENCIL_OPERATION_DECREMENT,
  /// Increase the stencil value by 1, wrapping back to 0 when incrementing the maximum value
  GRAL_STENCIL_OPERATION_INCREMENT_WRAP,
  /// Decrease the stencil value by 1, wrapping when decrementing 0
  GRAL_STENCIL_OPERATION_DECREMENT_WRAP,
  /// Invert the bits of the stencil buffer
  GRAL_STENCIL_OPERATION_INVERT
} gral_stencil_operation_t;

gral_public void
gral_set_stencil_buffer_params (gral_compare_func_t func, 
                                uint32_t refValue, uint32_t mask, 
                                gral_stencil_operation_t stencilFailOp, 
                                gral_stencil_operation_t depthFailOp,
                                gral_stencil_operation_t passOp, 
                                gral_bool_t twoSidedOperation);

typedef enum {
  GRAL_FRAME_BUFFER_TYPE_COLOUR  = 0x1,
  GRAL_FRAME_BUFFER_TYPE_DEPTH   = 0x2,
  GRAL_FRAME_BUFFER_TYPE_STENCIL = 0x4
} gral_frame_buffer_type_t;

gral_public void
gral_clear_frame_buffer (unsigned int buffers, 
                      const gral_color_t *color, float depth, unsigned short stencil);

gral_public void
gral_disable_texture_units_from (size_t tex_unit);

/** Blending factors for manually blending objects with the scene. If there isn't a predefined
SceneBlendType that you like, then you can specify the blending factors directly to affect the
combination of object and the existing scene. See Material::setSceneBlending for more details.
*/
typedef enum
{
  GRAL_SCENE_BLEND_FACTOR_SBF_ONE,
  GRAL_SCENE_BLEND_FACTOR_ZERO,
  GRAL_SCENE_BLEND_FACTOR_DEST_COLOUR,
  GRAL_SCENE_BLEND_FACTOR_SOURCE_COLOUR,
  GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_DEST_COLOUR,
  GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_SOURCE_COLOUR,
  GRAL_SCENE_BLEND_FACTOR_DEST_ALPHA,
  GRAL_SCENE_BLEND_FACTOR_SOURCE_ALPHA,
  GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_DEST_ALPHA,
  GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_SOURCE_ALPHA
} gral_scene_blend_factor_t;

gral_public void
gral_set_scene_blending (gral_scene_blend_factor_t sourceFactor, gral_scene_blend_factor_t destFactor);

/// The rendering operation type to perform
typedef enum {
  /// A list of points, 1 vertex per point
  GRAL_RENDER_OPERATION_TYPE_POINT_LIST,
  /// A list of lines, 2 vertices per line
  GRAL_RENDER_OPERATION_TYPE_LINE_LIST,
  /// A strip of connected lines, 1 vertex per line plus 1 start vertex
  GRAL_RENDER_OPERATION_TYPE_LINE_STRIP,
  /// A list of triangles, 3 vertices per triangle
  GRAL_RENDER_OPERATION_TYPE_TRIANGLE_LIST,
  /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that 
  GRAL_RENDER_OPERATION_TYPE_TRIANGLE_STRIP,
  /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
  GRAL_RENDER_OPERATION_TYPE_TRIANGLE_FAN
} gral_render_operation_type_t;

typedef struct _gral_render_operation {
  /// Vertex source data
  gral_vertex_data_t *vertex_data;

  /// The type of operation to perform
  gral_render_operation_type_t operation_type;

  /** Specifies whether to use indexes to determine the vertices to use as input. If false, the vertices are
  simply read in sequence to define the primitives. If true, indexes are used instead to identify vertices
  anywhere in the buffer, and allowing vertices to be used more than once.
  If true, then the indexBuffer, indexStart and numIndexes properties must be valid. */
  gral_bool_t use_indexes;

  /// Index data - only valid if useIndexes is true
  gral_index_data_t *index_data;

} gral_render_operation_t;

gral_public void
gral_render (gral_render_operation_t *op);

typedef enum
{
  /** Static buffer which the application rarely modifies once created. Modifying 
  the contents of this buffer will involve a performance hit.
  */
  GRAL_BUFFER_USAGE_STATIC,
  /** Indicates the application would like to modify this buffer with the CPU
  fairly often. 
  Buffers created with this flag will typically end up in AGP memory rather 
  than video memory.
  */
  GRAL_BUFFER_USAGE_DYNAMIC,
  /** Indicates the application will never read the contents of the buffer back, 
  it will only ever write data. Locking a buffer with this flag will ALWAYS 
  return a pointer to new, blank memory rather than the memory associated 
  with the contents of the buffer; this avoids DMA stalls because you can 
  write to a new memory area while the previous one is being used. 
  */
  GRAL_BUFFER_USAGE_WRITE_ONLY,
  /** Indicates that the application will be refilling the contents
  of the buffer regularly (not just updating, but generating the
  contents from scratch), and therefore does not mind if the contents 
  of the buffer are lost somehow and need to be recreated. This
  allows and additional level of optimisation on the buffer.
  This option only really makes sense when combined with 
  GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY.
  */
  GRAL_BUFFER_USAGE_DISCARDABLE,
  /// Combination of GRAL_BUFFER_USAGE_STATIC and GRAL_BUFFER_USAGE_WRITE_ONLY
  GRAL_BUFFER_USAGE_STATIC_WRITE_ONLY, 
  /** Combination of GRAL_BUFFER_USAGE_DYNAMIC and GRAL_BUFFER_USAGE_WRITE_ONLY. If you use 
  this, strongly consider using GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE
  instead if you update the entire contents of the buffer very 
  regularly. 
  */
  GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY,
  /// Combination of GRAL_BUFFER_USAGE_DYNAMIC, GRAL_BUFFER_USAGE_STATIC_WRITE_ONLY and GRAL_BUFFER_USAGE_DISCARDABLE
  GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE
} gral_buffer_usage_t;

gral_public gral_vertex_buffer_t *
gral_vertex_buffer_create (size_t vertexSize, size_t numVerts, gral_buffer_usage_t usage);

gral_public void
gral_vertex_buffer_destroy (gral_vertex_buffer_t *vb);

gral_public size_t
gral_vertex_buffer_get_size (gral_vertex_buffer_t *vb);

/// Locking options
typedef enum {
  /** Normal mode, ie allows read/write and contents are preserved. */
  GRAL_BUFFER_LOCK_OPTION_NORMAL,
  /** Discards the <em>entire</em> buffer while locking; this allows optimisation to be 
  performed because synchronisation issues are relaxed. Only allowed on buffers 
  created with the GRAL_BUFFER_USAGE_DYNAMIC flag. 
  */
  GRAL_BUFFER_LOCK_OPTION_DISCARD,
  /** Lock the buffer for reading only. Not allowed in buffers which are created with GRAL_BUFFER_USAGE_WRITE_ONLY. 
  Mandatory on static buffers, i.e. those created without the GRAL_BUFFER_USAGE_DYNAMIC flag. 
  */ 
  GRAL_BUFFER_LOCK_OPTION_READ_ONLY,
  /** As GRAL_BUFFER_LOCK_OPTION_NORMAL, except the application guarantees not to overwrite any 
  region of the buffer which has already been used in this frame, can allow
  some optimisation on some APIs. */
  GRAL_BUFFER_LOCK_OPTION_NO_OVERWRITE
} gral_buffer_lock_option_t;

gral_public void *
gral_vertex_buffer_lock (gral_vertex_buffer_t *vb, size_t offset, size_t length,
                         gral_buffer_lock_option_t opt);

gral_public void
gral_vertex_buffer_unlock (gral_vertex_buffer_t *vb);

typedef enum {
  GRAL_INDEX_BUFFER_TYPE_16BIT,
  GRAL_INDEX_BUFFER_TYPE_32BIT
} gral_index_buffer_type_t;

gral_public gral_index_buffer_t *
gral_index_buffer_create (gral_index_buffer_type_t itype, size_t numIndexes, 
                          gral_buffer_usage_t usage);

gral_public void
gral_index_buffer_destroy (gral_index_buffer_t *ib);

gral_public size_t
gral_index_buffer_get_size (gral_index_buffer_t *ib);

gral_public void *
gral_index_buffer_lock (gral_index_buffer_t *ib, size_t offset, size_t length,
                         gral_buffer_lock_option_t opt);

gral_public void
gral_index_buffer_unlock (gral_index_buffer_t *ib);

gral_public gral_vertex_data_t *
gral_vertex_data_create (void);

gral_public void
gral_vertex_data_destroy (gral_vertex_data_t *vd);

gral_public void
gral_vertex_data_set_start (gral_vertex_data_t *vd, size_t start);

gral_public void
gral_vertex_data_set_count (gral_vertex_data_t *vd, size_t count);

/// Vertex element type, used to identify the base types of the vertex contents
typedef enum
{
  GRAL_VERTEX_ELEMENT_TYPE_FLOAT1,
  GRAL_VERTEX_ELEMENT_TYPE_FLOAT2,
  GRAL_VERTEX_ELEMENT_TYPE_FLOAT3,
  GRAL_VERTEX_ELEMENT_TYPE_FLOAT4,
  GRAL_VERTEX_ELEMENT_TYPE_COLOR,
  GRAL_VERTEX_ELEMENT_TYPE_SHORT1,
  GRAL_VERTEX_ELEMENT_TYPE_SHORT2,
  GRAL_VERTEX_ELEMENT_TYPE_SHORT3,
  GRAL_VERTEX_ELEMENT_TYPE_SHORT4,
  GRAL_VERTEX_ELEMENT_TYPE_UBYTE4
} gral_vertex_element_type_t;

/// Vertex element semantics, used to identify the meaning of vertex buffer contents
typedef enum {
  /// Position, 3 reals per vertex
  GRAL_VERTEX_ELEMENT_SEMANTIC_POSITION,
  /// Blending weights
  GRAL_VERTEX_ELEMENT_SEMANTIC_BLEND_WEIGHTS,
  /// Blending indices
  GRAL_VERTEX_ELEMENT_SEMANTIC_BLEND_INDICES,
  /// Normal, 3 reals per vertex
  GRAL_VERTEX_ELEMENT_SEMANTIC_NORMAL,
  /// Diffuse colours
  GRAL_VERTEX_ELEMENT_SEMANTIC_DIFFUSE,
  /// Specular colours
  GRAL_VERTEX_ELEMENT_SEMANTIC_SPECULAR,
  /// Texture coordinates
  GRAL_VERTEX_ELEMENT_SEMANTIC_TEXTURE_COORDINATES,
  /// Binormal (Y axis if normal is Z)
  GRAL_VERTEX_ELEMENT_SEMANTIC_BINORMAL,
  /// Tangent (X axis if normal is Z)
  GRAL_VERTEX_ELEMENT_SEMANTIC_TANGENT
} gral_vertex_element_semantic_t;

gral_public void
gral_vertex_data_add_element (gral_vertex_data_t *vertex_data,
                              unsigned short source, size_t offset,
                              gral_vertex_element_type_t theType,
                              gral_vertex_element_semantic_t semantic,
                              unsigned short index);

gral_public size_t
gral_vertex_data_get_vertex_size (gral_vertex_data_t *vertex_data,
                                  unsigned short source);

gral_public void
gral_vertex_data_bind_buffer (gral_vertex_data_t *vd,
                              unsigned short source,
                              gral_vertex_buffer_t *buffer);

gral_public gral_index_data_t *
gral_index_data_create (void);

gral_public void
gral_index_data_destroy (gral_index_data_t *id);

gral_public void
gral_index_data_set_start (gral_index_data_t *id, size_t start);

gral_public void
gral_index_data_set_count (gral_index_data_t *id, size_t count);

gral_public void
gral_index_data_set_buffer (gral_index_data_t *id, gral_index_buffer_t *buffer);

gral_public void
gral_set_texture (size_t unit, gral_bool_t enabled, gral_texture_t *tex);

gral_public void
gral_set_texture_matrix (size_t unit, const gral_matrix_t *xform, size_t numTexCoords);

gral_public void
gral_set_texture_coord_set (size_t unit, size_t index);

typedef enum {
  /// No filtering, used for FILT_MIP to turn off mipmapping
  GRAL_FILTER_OPTION_NONE,
  /// Use the closest pixel
  GRAL_FILTER_OPTION_POINT,
  /// Average of a 2x2 pixel area, denotes bilinear for MIN and MAG, trilinear for MIP
  GRAL_FILTER_OPTION_LINEAR,
  /// Similar to FO_LINEAR, but compensates for the angle of the texture plane
  GRAL_FILTER_OPTION_ANISOTROPIC
} gral_filter_option_t;

gral_public void
gral_set_texture_unit_filtering (size_t unit, gral_filter_option_t minFilter,
                              gral_filter_option_t magFilter, gral_filter_option_t mipFilter);

gral_public void
gral_set_texture_layer_anisotropy (size_t unit, unsigned int maxAnisotropy);

gral_public void
gral_set_texture_mipmap_bias (size_t unit, float bias);

/** Type of texture blend mode.
*/
typedef enum {
  GRAL_LAYER_BLEND_TYPE_COLOR,
  GRAL_LAYER_BLEND_TYPE_ALPHA
} gral_layer_blend_type_t;

typedef enum {
  /// use source1 without modification
  GRAL_LAYER_BLEND_OPERATION_SOURCE1,
  /// use source2 without modification
  GRAL_LAYER_BLEND_OPERATION_SOURCE2,
  /// multiply source1 and source2 together
  GRAL_LAYER_BLEND_OPERATION_MODULATE,
  /// as LBX_MODULATE but brighten afterwards (x2)
  GRAL_LAYER_BLEND_OPERATION_MODULATE_X2,
  /// as LBX_MODULATE but brighten more afterwards (x4)
  GRAL_LAYER_BLEND_OPERATION_MODULATE_X4,
  /// add source1 and source2 together
  GRAL_LAYER_BLEND_OPERATION_ADD,
  /// as LBX_ADD, but subtract 0.5 from the result
  GRAL_LAYER_BLEND_OPERATION_ADD_SIGNED,
  /// as LBX_ADD, but subtract product from the sum
  GRAL_LAYER_BLEND_OPERATION_ADD_SMOOTH,
  /// subtract source2 from source1
  GRAL_LAYER_BLEND_OPERATION_SUBTRACT,
  /// use interpolated alpha value from vertices to scale source1, then add source2 scaled by (1-alpha)
  GRAL_LAYER_BLEND_OPERATION_BLEND_DIFFUSE_ALPHA,
  /// as LBX_BLEND_DIFFUSE_ALPHA, but use alpha from texture
  GRAL_LAYER_BLEND_OPERATION_BLEND_TEXTURE_ALPHA,
  /// as LBX_BLEND_DIFFUSE_ALPHA, but use current alpha from previous stages
  GRAL_LAYER_BLEND_OPERATION_BLEND_CURRENT_ALPHA,
  /// as LBX_BLEND_DIFFUSE_ALPHA but use a constant manual blend value (0.0-1.0)
  GRAL_LAYER_BLEND_OPERATION_BLEND_MANUAL,
  /// dot product of color1 and color2 
  GRAL_LAYER_BLEND_OPERATION_DOTPRODUCT,
  /// use interpolated color values from vertices to scale source1, then add source2 scaled by (1-color)
  GRAL_LAYER_BLEND_OPERATION_BLEND_DIFFUSE_COLOUR
} gral_layer_blend_operation_t;

typedef enum {
  /// the colour as built up from previous stages
  GRAL_LAYER_BLEND_SOURCE_CURRENT,
  /// the colour derived from the texture assigned to this layer
  GRAL_LAYER_BLEND_SOURCE_TEXTURE,
  /// the interpolated diffuse colour from the vertices
  GRAL_LAYER_BLEND_SOURCE_DIFFUSE,
  /// the interpolated specular colour from the vertices
  GRAL_LAYER_BLEND_SOURCE_SPECULAR,
  /// a colour supplied manually as a separate argument
  GRAL_LAYER_BLEND_SOURCE_MANUAL
} gral_layer_blend_source_t;

typedef struct _gral_layer_blend_mode {
  /// The type of blending (colour or alpha)
  gral_layer_blend_type_t blend_type;
  /// The operation to be applied
  gral_layer_blend_operation_t operation;
  /// The first source of colour/alpha
  gral_layer_blend_source_t source1;
  /// The second source of colour/alpha
  gral_layer_blend_source_t source2;

  /// Manual colour value for manual source1
  gral_color_t color_arg1;
  /// Manual colour value for manual source2
  gral_color_t color_arg2;
  /// Manual alpha value for manual source1
  float alpha_arg1;
  /// Manual alpha value for manual source2
  float alpha_arg2;
  /// Manual blending factor
  float factor;
} gral_layer_blend_mode_t;

gral_public void
gral_set_texture_blend_mode (size_t unit, const gral_layer_blend_mode_t *bm);

typedef enum {
  /// Texture wraps at values over 1.0
  GRAL_TEXTURE_ADDRESSING_MODE_WRAP,
  /// Texture mirrors (flips) at joins over 1.0
  GRAL_TEXTURE_ADDRESSING_MODE_MIRROR,
  /// Texture clamps at 1.0
  GRAL_TEXTURE_ADDRESSING_MODE_CLAMP,
  /// Texture coordinates outside the range [0.0, 1.0] are set to the border colour
  GRAL_TEXTURE_ADDRESSING_MODE_BORDER
} gral_texture_addressing_mode_t;

typedef struct _gral_uvw_addressing_mode {
  gral_texture_addressing_mode_t u, v, w;
} gral_uvw_addressing_mode_t;

gral_public void
gral_set_texture_addressing_mode (size_t unit, const gral_uvw_addressing_mode_t *uvw);

gral_public void
gral_set_texture_border_color (size_t unit, const gral_color_t *color);

typedef enum {
  /// No calculated texture coordinates
  GRAL_TEX_COORD_CALC_METHOD_NONE,
  /// Environment map based on vertex normals
  GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP,
  /// Environment map based on vertex positions
  GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_PLANAR,
  GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_REFLECTION,
  GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_NORMAL,
  /// Projective texture
  GRAL_TEX_COORD_CALC_METHOD_PROJECTIVE_TEXTURE
} gral_tex_coord_calc_method_t;

gral_public void
gral_set_texture_coord_calculation (size_t unit, gral_tex_coord_calc_method_t m);

typedef enum {
  /// 1D texture, used in combination with 1D texture coordinates
  GRAL_TEX_TYPE_1D,
  /// 2D texture, used in combination with 2D texture coordinates (default)
  GRAL_TEX_TYPE_2D,
  /// 3D volume texture, used in combination with 3D texture coordinates
  GRAL_TEX_TYPE_3D,
  /// 3D cube map, used in combination with 3D texture coordinates
  GRAL_TEX_TYPE_CUBE_MAP
} gral_texture_type_t;

typedef enum {
  /// 3 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue
  GRAL_PIXEL_FORMAT_BYTE_RGB,
  /// 3 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red
  GRAL_PIXEL_FORMAT_BYTE_BGR,
  /// 4 byte pixel format, 1 byte for blue, 1 byte for green, 1 byte for red and one byte for alpha
  GRAL_PIXEL_FORMAT_BYTE_BGRA,
  /// 4 byte pixel format, 1 byte for red, 1 byte for green, 1 byte for blue, and one byte for alpha
  GRAL_PIXEL_FORMAT_BYTE_RGBA
} gral_pixel_format_t;

typedef enum {
  GRAL_TEXTURE_USAGE_STATIC,
  GRAL_TEXTURE_USAGE_DYNAMIC,
  GRAL_TEXTURE_USAGE_WRITE_ONLY,
  GRAL_TEXTURE_USAGE_STATIC_WRITE_ONLY, 
  GRAL_TEXTURE_USAGE_DYNAMIC_WRITE_ONLY,
  GRAL_TEXTURE_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE,
  /// mipmaps will be automatically generated for this texture
  GRAL_TEXTURE_USAGE_AUTOMIPMAP,
  /// this texture will be a render target, i.e. used as a target for render to texture
  /// setting this flag will ignore all other texture usages except TU_AUTOMIPMAP
  GRAL_TEXTURE_USAGE_RENDERTARGET
} gral_texture_usage_t;

gral_public gral_texture_t *
gral_texture_create (gral_texture_type_t tex_type,
                     unsigned int width, unsigned int height, unsigned int depth,
                     int num_mips,
                     gral_pixel_format_t format, gral_texture_usage_t usage,
                     gral_bool_t hw_gamma_correction, unsigned int fsaa);

gral_public void
gral_texture_destroy (gral_texture_t *tus);

gral_public void *
gral_texture_buffer_lock_full (gral_texture_t *tex, size_t face, size_t mipmap,
                               gral_buffer_lock_option_t options);

gral_public void
gral_texture_buffer_unlock (gral_texture_t *tex, size_t face, size_t mipmap);

gral_public gral_cg_program_t *
gral_cg_program_create_from_file (gral_gpu_program_type_t gptype,
                                  const char *filename,
                                  const char *entry_point,
                                  const char *profiles);

gral_public gral_cg_program_t *
gral_cg_program_create_from_source (gral_gpu_program_type_t gptype,
                                    const char *source_string,
                                    const char *entry_point,
                                    const char *profiles);

gral_public void
gral_cg_program_destroy (gral_cg_program_t *prog);

gral_public void
gral_cg_program_set_constant_matrix (gral_cg_program_t *prog, 
                                     const char *name, const gral_matrix_t *m);

gral_public void
gral_cg_program_set_constant_float (gral_cg_program_t *prog, 
                                    const char *name, float val);

gral_public void
gral_cg_program_bind (gral_cg_program_t *prog);

GRAL_END_DECLS

#endif /* _GRAL_H_ */