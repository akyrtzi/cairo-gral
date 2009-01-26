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

#include <Ogre.h>
#include "gral-internal.h"
#include "gral.h"
#include "gral-ogre.h"

using namespace Ogre;

#define TO_COLOURVALUE(c) ColourValue((c).r, (c).g, (c).b, (c).a)
#define TO_MATRIX4(mat) Matrix4((mat).m[0][0], (mat).m[0][1], (mat).m[0][2], (mat).m[0][3], \
                                (mat).m[1][0], (mat).m[1][1], (mat).m[1][2], (mat).m[1][3], \
                                (mat).m[2][0], (mat).m[2][1], (mat).m[2][2], (mat).m[2][3], \
                                (mat).m[3][0], (mat).m[3][1], (mat).m[3][2], (mat).m[3][3] )

struct _gral_texture {
  TexturePtr ogre_tex;
};

struct _gral_vertex_buffer {
  HardwareVertexBufferSharedPtr ogre_buf;
};

struct _gral_index_buffer {
  HardwareIndexBufferSharedPtr ogre_buf;
};

struct _gral_cg_program {
  HighLevelGpuProgramPtr ogre_prog;
  GpuProgramParametersSharedPtr params;
};

static CullingMode convertEnum(gral_culling_mode_t mode);
static RenderOperation::OperationType convertEnum(gral_render_operation_type_t op);
static HardwareBuffer::Usage convertEnum(gral_buffer_usage_t usage);
static HardwareIndexBuffer::IndexType convertEnum(gral_index_buffer_type_t itype);
static VertexElementType convertEnum(gral_vertex_element_type_t type);
static VertexElementSemantic convertEnum(gral_vertex_element_semantic_t sem);
static GpuProgramType convertEnum(gral_gpu_program_type_t gtype);
static ShadeOptions convertEnum(gral_shade_type_t so);
static CompareFunction convertEnum (gral_compare_func_t func);
static StencilOperation convertEnum (gral_stencil_operation_t op);
static SceneBlendFactor convertEnum(gral_scene_blend_factor_t factor);
static TextureType convertEnum(gral_texture_type_t type);
static PixelFormat convertEnum(gral_pixel_format_t format);
static TextureUsage convertEnum(gral_texture_usage_t usage);
static HardwareBuffer::LockOptions convertEnum(gral_buffer_lock_option_t opt);
static TextureUnitState::TextureAddressingMode convertEnum(gral_texture_addressing_mode_t tam);
static FilterOptions convertEnum(gral_filter_option_t opt);
static LayerBlendType convertEnum(gral_layer_blend_type_t bt);
static LayerBlendOperationEx convertEnum(gral_layer_blend_operation_t bt);
static LayerBlendSource convertEnum(gral_layer_blend_source_t bt);
static TexCoordCalcMethod convertEnum(gral_tex_coord_calc_method_t m);

gral_argb_t
gral_color_to_argb (gral_color_t *col)
{
  return TO_COLOURVALUE(*col).getAsARGB();  
}

gral_abgr_t
gral_color_to_abgr (gral_color_t *col)
{
  return TO_COLOURVALUE(*col).getAsABGR();
}

gral_surface_t *
gral_ogre_surface_from_viewport(Viewport *vp)
{
  return reinterpret_cast<gral_surface_t *>(vp);
}

int
gral_surface_get_width (gral_surface_t *surf)
{
  Viewport *vp = reinterpret_cast<Viewport *>(surf);  
  return vp->getActualWidth();
}

int
gral_surface_get_height (gral_surface_t *surf)
{
  Viewport *vp = reinterpret_cast<Viewport *>(surf);
  return vp->getActualHeight();
}

void
gral_set_render_surface (gral_surface_t *surf)
{
  Viewport *vp = reinterpret_cast<Viewport *>(surf);  
  Root::getSingleton().getRenderSystem()->_setViewport(vp);
}

void
gral_set_view_matrix (const gral_matrix_t *m)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setViewMatrix(TO_MATRIX4(*m));
}

void
gral_set_projection_matrix (const gral_matrix_t *m)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setProjectionMatrix(TO_MATRIX4(*m));
}

void
gral_set_world_matrix (const gral_matrix_t *m)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setWorldMatrix(TO_MATRIX4(*m));
}

float
gral_get_horizontal_texel_offset (void)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  return rs->getHorizontalTexelOffset();
}

float
gral_get_vertical_texel_offset (void)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  return rs->getVerticalTexelOffset();
}

gral_capabilities_t
gral_get_capabilities (void)
{
  unsigned caps = 0;
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  const RenderSystemCapabilities *ogre_caps = rs->getCapabilities();
  if (ogre_caps->hasCapability(RSC_FRAGMENT_PROGRAM))
    caps = caps | GRAL_CAP_FRAGMENT_PROGRAM;
  return gral_capabilities_t(caps);
}

void
gral_set_lighting_enabled (gral_bool_t enabled)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->setLightingEnabled(enabled);
}

void
gral_set_culling_mode (gral_culling_mode_t mode)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setCullingMode(convertEnum(mode));
}

void
gral_unbind_gpu_program (gral_gpu_program_type_t gptype)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->unbindGpuProgram(convertEnum(gptype));
}

void
gral_set_shading_type (gral_shade_type_t so)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->setShadingType(convertEnum(so));  
}

void
gral_set_surface_params (const gral_color_t *ambient,
                         const gral_color_t *diffuse, const gral_color_t *specular,
                         const gral_color_t *emissive, float shininess,
                         gral_track_vertex_color_type_t tracking)
{
  TrackVertexColourType ogre_tracking = 0;
  if (tracking & GRAL_TRACK_VERTEX_COLOR_TYPE_AMBIENT)
    ogre_tracking |= TVC_AMBIENT;
  if (tracking & GRAL_TRACK_VERTEX_COLOR_TYPE_DIFFUSE)
    ogre_tracking |= TVC_DIFFUSE;
  if (tracking & GRAL_TRACK_VERTEX_COLOR_TYPE_SPECULAR)
    ogre_tracking |= TVC_SPECULAR;
  if (tracking & GRAL_TRACK_VERTEX_COLOR_TYPE_EMISSIVE)
    ogre_tracking |= TVC_EMISSIVE;

  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setSurfaceParams(TO_COLOURVALUE(*ambient), TO_COLOURVALUE(*diffuse), TO_COLOURVALUE(*specular),
                        TO_COLOURVALUE(*emissive), shininess, ogre_tracking);
}

void
gral_set_depth_buffer_params (gral_bool_t depthTest, gral_bool_t depthWrite,
                              gral_compare_func_t depthFunction)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setDepthBufferParams(depthTest, depthWrite, convertEnum(depthFunction));
}

void
gral_set_depth_buffer_write_enabled (gral_bool_t enabled)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setDepthBufferWriteEnabled(enabled);
}

void
gral_set_color_buffer_write_enabled(gral_bool_t red,
                                    gral_bool_t green,
                                    gral_bool_t blue,
                                    gral_bool_t alpha)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setColourBufferWriteEnabled(red, green, blue, alpha);
}

void
gral_set_stencil_check_enabled (gral_bool_t enabled)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->setStencilCheckEnabled(enabled);
}

void
gral_set_stencil_buffer_params(gral_compare_func_t func, 
                               uint32_t refValue, uint32_t mask, 
                               gral_stencil_operation_t stencilFailOp, 
                               gral_stencil_operation_t depthFailOp,
                               gral_stencil_operation_t passOp, 
                               gral_bool_t twoSidedOperation)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->setStencilBufferParams(convertEnum(func), refValue, mask,
                             convertEnum(stencilFailOp), convertEnum(depthFailOp), convertEnum(passOp),
                             twoSidedOperation);
}

gral_public void
gral_clear_frame_buffer (unsigned int buffers, 
                         const gral_color_t *color, float depth, unsigned short stencil)
{
  unsigned int ogre_buffers = 0;
  if (buffers & GRAL_FRAME_BUFFER_TYPE_COLOUR)
    ogre_buffers |= FBT_COLOUR;
  if (buffers & GRAL_FRAME_BUFFER_TYPE_DEPTH)
    ogre_buffers |= FBT_DEPTH;
  if (buffers & GRAL_FRAME_BUFFER_TYPE_STENCIL)
    ogre_buffers |= FBT_STENCIL;

  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->clearFrameBuffer(ogre_buffers, TO_COLOURVALUE(*color), depth, stencil);
}

void
gral_disable_texture_units_from (size_t texUnit)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_disableTextureUnitsFrom(texUnit);
}

void
gral_set_scene_blending (gral_scene_blend_factor_t sourceFactor, gral_scene_blend_factor_t destFactor)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setSceneBlending(convertEnum(sourceFactor), convertEnum(destFactor));
}

void
gral_render (gral_render_operation_t *op)
{
  RenderOperation ogre_op;
  ogre_op.vertexData = reinterpret_cast<VertexData*>(op->vertex_data);
  ogre_op.indexData = reinterpret_cast<IndexData*>(op->index_data);
  ogre_op.useIndexes = op->use_indexes;
  ogre_op.operationType = convertEnum(op->operation_type);

  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_render(ogre_op);
}

gral_vertex_buffer_t *
gral_vertex_buffer_create (size_t vertexSize, size_t numVerts, gral_buffer_usage_t usage)
{
  HardwareVertexBufferSharedPtr ogre_buf = HardwareBufferManager::getSingleton().createVertexBuffer(
                  vertexSize, numVerts, convertEnum(usage) );
  if (ogre_buf.isNull())
    return NULL;

  gral_vertex_buffer_t *vb = new gral_vertex_buffer_t();
  vb->ogre_buf = ogre_buf;
  return vb;
}

void
gral_vertex_buffer_destroy (gral_vertex_buffer_t *vb)
{
  delete vb;
}

size_t
gral_vertex_buffer_get_size (gral_vertex_buffer_t *vb)
{
  return vb->ogre_buf->getSizeInBytes();
}

void *
gral_vertex_buffer_lock (gral_vertex_buffer_t *vb, size_t offset, size_t length,
                         gral_buffer_lock_option_t opt)
{
  return vb->ogre_buf->lock(offset, length, convertEnum(opt));
}

void
gral_vertex_buffer_unlock (gral_vertex_buffer_t *vb)
{
  vb->ogre_buf->unlock();
}

gral_index_buffer_t *
gral_index_buffer_create (gral_index_buffer_type_t itype, size_t numIndexes, 
                          gral_buffer_usage_t usage)
{
  HardwareIndexBufferSharedPtr ogre_buf = HardwareBufferManager::getSingleton().createIndexBuffer(
                 convertEnum(itype), numIndexes, convertEnum(usage) );
  if (ogre_buf.isNull())
    return NULL;

  gral_index_buffer_t *ib = new gral_index_buffer_t();
  ib->ogre_buf = ogre_buf;
  return ib;
}

void
gral_index_buffer_destroy (gral_index_buffer_t *ib)
{
  delete ib;
}

size_t
gral_index_buffer_get_size (gral_index_buffer_t *ib)
{
  return ib->ogre_buf->getSizeInBytes();
}

void *
gral_index_buffer_lock (gral_index_buffer_t *ib, size_t offset, size_t length,
                        gral_buffer_lock_option_t opt)
{
  return ib->ogre_buf->lock(offset, length, convertEnum(opt));
}

void
gral_index_buffer_unlock (gral_index_buffer_t *ib)
{
  ib->ogre_buf->unlock();
}

gral_vertex_data_t *
gral_vertex_data_create (void)
{
  VertexData *vd = new VertexData();
  return reinterpret_cast<gral_vertex_data_t*>(vd);
}

void
gral_vertex_data_destroy (gral_vertex_data_t *vd)
{
  VertexData *ogre_vd = reinterpret_cast<VertexData*>(vd);
  delete ogre_vd;
}

void
gral_vertex_data_set_start (gral_vertex_data_t *vd, size_t start)
{
  VertexData *ogre_vd = reinterpret_cast<VertexData*>(vd);
  ogre_vd->vertexStart = start;
}

void
gral_vertex_data_set_count (gral_vertex_data_t *vd, size_t count)
{
  VertexData *ogre_vd = reinterpret_cast<VertexData*>(vd);
  ogre_vd->vertexCount = count;
}

void
gral_vertex_data_add_element (gral_vertex_data_t *vertex_data,
                              unsigned short source, size_t offset,
                              gral_vertex_element_type_t theType,
                              gral_vertex_element_semantic_t semantic,
                              unsigned short index)
{
  VertexData *vd = reinterpret_cast<VertexData*>(vertex_data);
  vd->vertexDeclaration->addElement(source, offset, convertEnum(theType), convertEnum(semantic), index);
}

size_t
gral_vertex_data_get_vertex_size (gral_vertex_data_t *vertex_data,
                                  unsigned short source)
{
  VertexData *vd = reinterpret_cast<VertexData*>(vertex_data);
  return vd->vertexDeclaration->getVertexSize(source);
}

void
gral_vertex_data_bind_buffer (gral_vertex_data_t *vd,
                              unsigned short source,
                              gral_vertex_buffer_t *buffer)
{
  VertexData *ogre_vd = reinterpret_cast<VertexData*>(vd);
  ogre_vd->vertexBufferBinding->setBinding(source, buffer->ogre_buf);
}

gral_index_data_t *
gral_index_data_create (void)
{
  IndexData *id = new IndexData();
  return reinterpret_cast<gral_index_data_t*>(id);
}

void
gral_index_data_destroy (gral_index_data_t *id)
{
  IndexData *ogre_id = reinterpret_cast<IndexData*>(id);
  delete ogre_id;
}

void
gral_index_data_set_start (gral_index_data_t *id, size_t start)
{
  IndexData *ogre_id = reinterpret_cast<IndexData*>(id);
  ogre_id->indexStart = start;
}

void
gral_index_data_set_count (gral_index_data_t *id, size_t count)
{
  IndexData *ogre_id = reinterpret_cast<IndexData*>(id);
  ogre_id->indexCount = count;
}

void
gral_index_data_set_buffer (gral_index_data_t *id, gral_index_buffer_t *buffer)
{
  IndexData *ogre_id = reinterpret_cast<IndexData*>(id);
  ogre_id->indexBuffer = buffer->ogre_buf;
}

void
gral_set_texture(size_t unit, gral_bool_t enabled, gral_texture_t *tex)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTexture(unit, enabled, tex->ogre_tex);
}

void
gral_set_texture_matrix (size_t unit, const gral_matrix_t *xform, size_t numTexCoords)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureMatrix(unit, TO_MATRIX4(*xform), numTexCoords);
}

void
gral_set_texture_coord_set (size_t unit, size_t index)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureCoordSet(unit, index);
}

void
gral_set_texture_unit_filtering (size_t unit, gral_filter_option_t minFilter,
                                 gral_filter_option_t magFilter, gral_filter_option_t mipFilter)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureUnitFiltering(unit, convertEnum(minFilter), convertEnum(magFilter), convertEnum(mipFilter));
}

void
gral_set_texture_layer_anisotropy (size_t unit, unsigned int maxAnisotropy)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureLayerAnisotropy(unit, maxAnisotropy);
}

void
gral_set_texture_mipmap_bias (size_t unit, float bias)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureMipmapBias(unit, bias);
}

void
gral_set_texture_blend_mode (size_t unit, const gral_layer_blend_mode_t *bm)
{
  LayerBlendModeEx ogre_bm;
  ogre_bm.blendType = convertEnum(bm->blend_type);
  ogre_bm.operation = convertEnum(bm->operation);
  ogre_bm.source1 = convertEnum(bm->source1);
  ogre_bm.source2 = convertEnum(bm->source2);
  ogre_bm.colourArg1 = TO_COLOURVALUE(bm->color_arg1);
  ogre_bm.colourArg2 = TO_COLOURVALUE(bm->color_arg2);
  ogre_bm.alphaArg1 = bm->alpha_arg1;
  ogre_bm.alphaArg2 = bm->alpha_arg2;
  ogre_bm.factor = bm->factor;
  
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureBlendMode(unit, ogre_bm);  
}

void
gral_set_texture_addressing_mode (size_t unit, const gral_uvw_addressing_mode_t *uvw)
{
  TextureUnitState::UVWAddressingMode ogre_uvw;
  ogre_uvw.u = convertEnum(uvw->u);
  ogre_uvw.v = convertEnum(uvw->v);
  ogre_uvw.w = convertEnum(uvw->w);

  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureAddressingMode(unit, ogre_uvw);
}

void
gral_set_texture_border_color (size_t unit, const gral_color_t *color)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureBorderColour(unit, TO_COLOURVALUE(*color));
}

void
gral_set_texture_coord_calculation (size_t unit, gral_tex_coord_calc_method_t m)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setTextureCoordCalculation(unit, convertEnum(m));
}

gral_texture_t *
gral_texture_create (gral_texture_type_t tex_type,
                     unsigned int width, unsigned int height, unsigned int depth,
                     int num_mips,
                     gral_pixel_format_t format, gral_texture_usage_t usage,
                     gral_bool_t hw_gamma_correction, unsigned int fsaa)
{
  static int counter = 0;
  std::ostringstream name;
  name << "##GRAL-TEXTURE-" << (counter++);
  TexturePtr ogre_tex = TextureManager::getSingleton().createManual(
          name.str(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          convertEnum(tex_type), width, height, depth,
          num_mips, convertEnum(format), convertEnum(usage),
          NULL, hw_gamma_correction, fsaa);
  if (ogre_tex.isNull())
    return NULL;

  gral_texture_t *tex = new gral_texture_t();
  tex->ogre_tex = ogre_tex;
  return tex;
}

void
gral_texture_destroy (gral_texture_t *tus)
{
  delete tus;
}

void *
gral_texture_buffer_lock_full (gral_texture_t *tex, size_t face, size_t mipmap,
                               gral_buffer_lock_option_t options)
{
  return tex->ogre_tex->getBuffer(face, mipmap)->lock(convertEnum(options));
}

void
gral_texture_buffer_unlock (gral_texture_t *tex, size_t face, size_t mipmap)
{
  tex->ogre_tex->getBuffer(face, mipmap)->unlock();
}

static HighLevelGpuProgramPtr
createOgreProgram (gral_gpu_program_type_t gptype,
                   const char *entry_point,
                   const char *profiles)
{
  static int counter = 0;
  std::ostringstream name;
  name << "##GRAL-PROGRAM-" << (counter++);
  HighLevelGpuProgramPtr ogre_prog = HighLevelGpuProgramManager::getSingleton().createProgram(
              name.str(),
              ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
              "cg",
              convertEnum(gptype));
  if (ogre_prog.isNull())
    return ogre_prog;

  ogre_prog->setParameter("entry_point", entry_point);
  ogre_prog->setParameter("profiles", profiles);

  return ogre_prog;
}

gral_cg_program_t *
gral_cg_program_create_from_file (gral_gpu_program_type_t gptype,
                                  const char *filename,
                                  const char *entry_point,
                                  const char *profiles)
{
  HighLevelGpuProgramPtr ogre_prog = createOgreProgram(gptype, entry_point, profiles);
  if (ogre_prog.isNull())
    return NULL;

  ogre_prog->setSourceFile(filename);
  ogre_prog->load();

  if (ogre_prog->hasCompileError())
    return NULL; 

  gral_cg_program_t *prog = new gral_cg_program_t();
  prog->ogre_prog = ogre_prog;
  prog->params = ogre_prog->createParameters();
  return prog;
}

gral_cg_program_t *
gral_cg_program_create_from_source (gral_gpu_program_type_t gptype,
                                    const char *source_string,
                                    const char *entry_point,
                                    const char *profiles)
{
  HighLevelGpuProgramPtr ogre_prog = createOgreProgram(gptype, entry_point, profiles);
  if (ogre_prog.isNull())
    return NULL;

  ogre_prog->setSource(source_string);
  ogre_prog->load();

  if (ogre_prog->hasCompileError())
    return NULL; 

  gral_cg_program_t *prog = new gral_cg_program_t();
  prog->ogre_prog = ogre_prog;
  prog->params = ogre_prog->createParameters();
  return prog;
}

void
gral_cg_program_destroy (gral_cg_program_t *prog)
{
  delete prog;
}

void
gral_cg_program_set_constant_matrix (gral_cg_program_t *prog, 
                                     const char *name, const gral_matrix_t *m)
{
  prog->params->setNamedConstant(name, TO_MATRIX4(*m));  
}

void
gral_cg_program_set_constant_float (gral_cg_program_t *prog, 
                                    const char *name, float val)
{
  prog->params->setNamedConstant(name, val);
}

void
gral_cg_program_bind (gral_cg_program_t *prog)
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->bindGpuProgram(prog->ogre_prog->_getBindingDelegate());
  rs->bindGpuProgramParameters(prog->ogre_prog->getType(), prog->params);
}

CullingMode
convertEnum(gral_culling_mode_t mode)
{
  switch (mode) {
    default: ASSERT_NOT_REACHED;
    case GRAL_CULL_NONE:
      return CULL_NONE;
    case GRAL_CULL_CLOCKWISE:
      return CULL_CLOCKWISE;
    case GRAL_CULL_ANTICLOCKWISE:
      return CULL_ANTICLOCKWISE;
  }
}

static RenderOperation::OperationType
convertEnum(gral_render_operation_type_t op)
{
  switch (op) {
    default: ASSERT_NOT_REACHED;
    case GRAL_RENDER_OPERATION_TYPE_POINT_LIST:
      return RenderOperation::OT_POINT_LIST;
    case GRAL_RENDER_OPERATION_TYPE_LINE_LIST:
      return RenderOperation::OT_LINE_LIST;
    case GRAL_RENDER_OPERATION_TYPE_LINE_STRIP:
      return RenderOperation::OT_LINE_STRIP;
    case GRAL_RENDER_OPERATION_TYPE_TRIANGLE_LIST:
      return RenderOperation::OT_TRIANGLE_LIST;
    case GRAL_RENDER_OPERATION_TYPE_TRIANGLE_STRIP:
      return RenderOperation::OT_TRIANGLE_STRIP;
    case GRAL_RENDER_OPERATION_TYPE_TRIANGLE_FAN:
      return RenderOperation::OT_TRIANGLE_FAN;
  }
}

static HardwareBuffer::Usage
convertEnum(gral_buffer_usage_t usage)
{
  switch (usage) {
    default: ASSERT_NOT_REACHED;
    case GRAL_BUFFER_USAGE_STATIC:
      return HardwareBuffer::HBU_STATIC;
    case GRAL_BUFFER_USAGE_DYNAMIC:
      return HardwareBuffer::HBU_DYNAMIC;
    case GRAL_BUFFER_USAGE_WRITE_ONLY:
      return HardwareBuffer::HBU_WRITE_ONLY;
    case GRAL_BUFFER_USAGE_DISCARDABLE:
      return HardwareBuffer::HBU_DISCARDABLE;
    case GRAL_BUFFER_USAGE_STATIC_WRITE_ONLY:
      return HardwareBuffer::HBU_STATIC_WRITE_ONLY;
    case GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY:
      return HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY;
    case GRAL_BUFFER_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE:
      return HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE;
  }
}

static HardwareIndexBuffer::IndexType
convertEnum(gral_index_buffer_type_t itype)
{
  switch (itype) {
    default: ASSERT_NOT_REACHED;
    case GRAL_INDEX_BUFFER_TYPE_16BIT:
      return HardwareIndexBuffer::IT_16BIT;
    case GRAL_INDEX_BUFFER_TYPE_32BIT:
      return HardwareIndexBuffer::IT_32BIT;
  }
}

VertexElementType
convertEnum(gral_vertex_element_type_t type)
{
  switch (type) {
    default: ASSERT_NOT_REACHED;
    case GRAL_VERTEX_ELEMENT_TYPE_FLOAT1:
      return VET_FLOAT1;
    case GRAL_VERTEX_ELEMENT_TYPE_FLOAT2:
      return VET_FLOAT2;
    case GRAL_VERTEX_ELEMENT_TYPE_FLOAT3:
      return VET_FLOAT3;
    case GRAL_VERTEX_ELEMENT_TYPE_FLOAT4:
      return VET_FLOAT4;
    case GRAL_VERTEX_ELEMENT_TYPE_COLOR:
      return VET_COLOUR;
    case GRAL_VERTEX_ELEMENT_TYPE_SHORT1:
      return VET_SHORT1;
    case GRAL_VERTEX_ELEMENT_TYPE_SHORT2:
      return VET_SHORT2;
    case GRAL_VERTEX_ELEMENT_TYPE_SHORT3:
      return VET_SHORT3;
    case GRAL_VERTEX_ELEMENT_TYPE_SHORT4:
      return VET_SHORT4;
    case GRAL_VERTEX_ELEMENT_TYPE_UBYTE4:
      return VET_UBYTE4;
  }
}

VertexElementSemantic
convertEnum(gral_vertex_element_semantic_t sem)
{
  switch (sem) {
    default: ASSERT_NOT_REACHED;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_POSITION:
      return VES_POSITION;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_BLEND_WEIGHTS:
      return VES_BLEND_WEIGHTS;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_BLEND_INDICES:
      return VES_BLEND_INDICES;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_NORMAL:
      return VES_NORMAL;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_DIFFUSE:
      return VES_DIFFUSE;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_SPECULAR:
      return VES_SPECULAR;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_TEXTURE_COORDINATES:
      return VES_TEXTURE_COORDINATES;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_BINORMAL:
      return VES_BINORMAL;
    case GRAL_VERTEX_ELEMENT_SEMANTIC_TANGENT:
      return VES_TANGENT;
  }
}

GpuProgramType
convertEnum(gral_gpu_program_type_t gtype)
{
  switch (gtype) {
    default: ASSERT_NOT_REACHED;
    case GRAL_GPU_PROGRAM_TYPE_VERTEX:
      return GPT_VERTEX_PROGRAM;
    case GRAL_GPU_PROGRAM_TYPE_FRAGMENT:
      return GPT_FRAGMENT_PROGRAM;
    case GRAL_GPU_PROGRAM_TYPE_GEOMETRY:
      return GPT_GEOMETRY_PROGRAM;
  }
}

ShadeOptions
convertEnum(gral_shade_type_t so)
{
  switch (so) {
    default: ASSERT_NOT_REACHED;
    case GRAL_SHADE_TYPE_FLAT:
      return SO_FLAT;
    case GRAL_SHADE_TYPE_GOURAUD:
      return SO_GOURAUD;
    case GRAL_SHADE_TYPE_PHONG:
      return SO_PHONG;
  }
}

CompareFunction
convertEnum (gral_compare_func_t func)
{
  switch (func) {
    default: ASSERT_NOT_REACHED;
    case GRAL_COMPARE_FUNC_ALWAYS_FAIL:
      return CMPF_ALWAYS_FAIL;
    case GRAL_COMPARE_FUNC_ALWAYS_PASS:
      return CMPF_ALWAYS_PASS;
    case GRAL_COMPARE_FUNC_LESS:
      return CMPF_LESS;
    case GRAL_COMPARE_FUNC_LESS_EQUAL:
      return CMPF_LESS_EQUAL;
    case GRAL_COMPARE_FUNC_EQUAL:
      return CMPF_EQUAL;
    case GRAL_COMPARE_FUNC_NOT_EQUAL:
      return CMPF_NOT_EQUAL;
    case GRAL_COMPARE_FUNC_GREATER_EQUAL:
      return CMPF_GREATER_EQUAL;
    case GRAL_COMPARE_FUNC_GREATER:
      return CMPF_GREATER;
  }
}

StencilOperation
convertEnum (gral_stencil_operation_t op)
{
  switch (op) {
    default: ASSERT_NOT_REACHED;
    case GRAL_STENCIL_OPERATION_KEEP:
      return SOP_KEEP;
    case GRAL_STENCIL_OPERATION_ZERO:
      return SOP_ZERO;
    case GRAL_STENCIL_OPERATION_REPLACE:
      return SOP_REPLACE;
    case GRAL_STENCIL_OPERATION_INCREMENT:
      return SOP_INCREMENT;
    case GRAL_STENCIL_OPERATION_DECREMENT:
      return SOP_DECREMENT;
    case GRAL_STENCIL_OPERATION_INCREMENT_WRAP:
      return SOP_INCREMENT_WRAP;
    case GRAL_STENCIL_OPERATION_DECREMENT_WRAP:
      return SOP_DECREMENT_WRAP;
    case GRAL_STENCIL_OPERATION_INVERT:
      return SOP_INVERT;
  }
}

SceneBlendFactor
convertEnum(gral_scene_blend_factor_t factor)
{
  switch (factor) {
    default: ASSERT_NOT_REACHED;
    case GRAL_SCENE_BLEND_FACTOR_SBF_ONE:
      return SBF_ONE;
    case GRAL_SCENE_BLEND_FACTOR_ZERO:
      return SBF_ZERO;
    case GRAL_SCENE_BLEND_FACTOR_DEST_COLOUR:
      return SBF_DEST_COLOUR;
    case GRAL_SCENE_BLEND_FACTOR_SOURCE_COLOUR:
      return SBF_SOURCE_COLOUR;
    case GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_DEST_COLOUR:
      return SBF_ONE_MINUS_DEST_COLOUR;
    case GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_SOURCE_COLOUR:
      return SBF_ONE_MINUS_SOURCE_COLOUR;
    case GRAL_SCENE_BLEND_FACTOR_DEST_ALPHA:
      return SBF_DEST_ALPHA;
    case GRAL_SCENE_BLEND_FACTOR_SOURCE_ALPHA:
      return SBF_SOURCE_ALPHA;
    case GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_DEST_ALPHA:
      return SBF_ONE_MINUS_DEST_ALPHA;
    case GRAL_SCENE_BLEND_FACTOR_ONE_MINUS_SOURCE_ALPHA:
      return SBF_ONE_MINUS_SOURCE_ALPHA;
  }
}

TextureType
convertEnum(gral_texture_type_t type)
{
  switch (type) {
    default: ASSERT_NOT_REACHED;
    case GRAL_TEX_TYPE_1D:
      return TEX_TYPE_1D;
    case GRAL_TEX_TYPE_2D:
      return TEX_TYPE_2D;
    case GRAL_TEX_TYPE_3D:
      return TEX_TYPE_3D;
    case GRAL_TEX_TYPE_CUBE_MAP:
      return TEX_TYPE_CUBE_MAP;
  }
}

PixelFormat
convertEnum(gral_pixel_format_t format)
{
  switch (format) {
    default: ASSERT_NOT_REACHED;
    case GRAL_PIXEL_FORMAT_BYTE_RGB:
      return PF_BYTE_RGB;
    case GRAL_PIXEL_FORMAT_BYTE_BGR:
      return PF_BYTE_BGR;
    case GRAL_PIXEL_FORMAT_BYTE_BGRA:
      return PF_BYTE_BGRA;
    case GRAL_PIXEL_FORMAT_BYTE_RGBA:
      return PF_BYTE_RGBA;
  }
}

TextureUsage
convertEnum(gral_texture_usage_t usage)
{
  switch (usage) {
    default: ASSERT_NOT_REACHED;
    case GRAL_TEXTURE_USAGE_STATIC:
      return TU_STATIC;
    case GRAL_TEXTURE_USAGE_DYNAMIC:
      return TU_DYNAMIC;
    case GRAL_TEXTURE_USAGE_WRITE_ONLY:
      return TU_WRITE_ONLY;
    case GRAL_TEXTURE_USAGE_STATIC_WRITE_ONLY:
      return TU_STATIC_WRITE_ONLY;
    case GRAL_TEXTURE_USAGE_DYNAMIC_WRITE_ONLY:
      return TU_DYNAMIC_WRITE_ONLY;
    case GRAL_TEXTURE_USAGE_DYNAMIC_WRITE_ONLY_DISCARDABLE:
      return TU_DYNAMIC_WRITE_ONLY_DISCARDABLE;
    case GRAL_TEXTURE_USAGE_AUTOMIPMAP:
      return TU_AUTOMIPMAP;
    case GRAL_TEXTURE_USAGE_RENDERTARGET:
      return TU_RENDERTARGET;
  }
}

HardwareBuffer::LockOptions
convertEnum(gral_buffer_lock_option_t opt)
{
  switch (opt) {
    default: ASSERT_NOT_REACHED;
    case GRAL_BUFFER_LOCK_OPTION_NORMAL:
      return HardwareBuffer::HBL_NORMAL;
    case GRAL_BUFFER_LOCK_OPTION_DISCARD:
      return HardwareBuffer::HBL_DISCARD;
    case GRAL_BUFFER_LOCK_OPTION_READ_ONLY:
      return HardwareBuffer::HBL_READ_ONLY;
    case GRAL_BUFFER_LOCK_OPTION_NO_OVERWRITE:
      return HardwareBuffer::HBL_NO_OVERWRITE;
  }
}

TextureUnitState::TextureAddressingMode
convertEnum(gral_texture_addressing_mode_t tam)
{
  switch (tam) {
    default: ASSERT_NOT_REACHED;
    case GRAL_TEXTURE_ADDRESSING_MODE_WRAP:
      return TextureUnitState::TAM_WRAP;
    case GRAL_TEXTURE_ADDRESSING_MODE_MIRROR:
      return TextureUnitState::TAM_MIRROR;
    case GRAL_TEXTURE_ADDRESSING_MODE_CLAMP:
      return TextureUnitState::TAM_CLAMP;
    case GRAL_TEXTURE_ADDRESSING_MODE_BORDER:
      return TextureUnitState::TAM_BORDER;
  }
}

FilterOptions
convertEnum(gral_filter_option_t opt)
{
  switch (opt) {
    default: ASSERT_NOT_REACHED;
    case GRAL_FILTER_OPTION_NONE:
      return FO_NONE;
    case GRAL_FILTER_OPTION_POINT:
      return FO_POINT;
    case GRAL_FILTER_OPTION_LINEAR:
      return FO_LINEAR;
    case GRAL_FILTER_OPTION_ANISOTROPIC:
      return FO_ANISOTROPIC;
  }
}

LayerBlendType
convertEnum(gral_layer_blend_type_t bt)
{
  switch (bt) {
    default: ASSERT_NOT_REACHED;
    case GRAL_LAYER_BLEND_TYPE_COLOR:
      return LBT_COLOUR;
    case GRAL_LAYER_BLEND_TYPE_ALPHA:
      return LBT_ALPHA;
  }
}

LayerBlendOperationEx
convertEnum(gral_layer_blend_operation_t bt)
{
  switch (bt) {
    default: ASSERT_NOT_REACHED;
    case GRAL_LAYER_BLEND_OPERATION_SOURCE1:
      return LBX_SOURCE1;
    case GRAL_LAYER_BLEND_OPERATION_SOURCE2:
      return LBX_SOURCE2;
    case GRAL_LAYER_BLEND_OPERATION_MODULATE:
      return LBX_MODULATE;
    case GRAL_LAYER_BLEND_OPERATION_MODULATE_X2:
      return LBX_MODULATE_X2;
    case GRAL_LAYER_BLEND_OPERATION_MODULATE_X4:
      return LBX_MODULATE_X4;
    case GRAL_LAYER_BLEND_OPERATION_ADD:
      return LBX_ADD;
    case GRAL_LAYER_BLEND_OPERATION_ADD_SIGNED:
      return LBX_ADD_SIGNED;
    case GRAL_LAYER_BLEND_OPERATION_ADD_SMOOTH:
      return LBX_ADD_SMOOTH;
    case GRAL_LAYER_BLEND_OPERATION_SUBTRACT:
      return LBX_SUBTRACT;
    case GRAL_LAYER_BLEND_OPERATION_BLEND_DIFFUSE_ALPHA:
      return LBX_BLEND_DIFFUSE_ALPHA;
    case GRAL_LAYER_BLEND_OPERATION_BLEND_TEXTURE_ALPHA:
      return LBX_BLEND_TEXTURE_ALPHA;
    case GRAL_LAYER_BLEND_OPERATION_BLEND_CURRENT_ALPHA:
      return LBX_BLEND_CURRENT_ALPHA;
    case GRAL_LAYER_BLEND_OPERATION_BLEND_MANUAL:
      return LBX_BLEND_MANUAL;
    case GRAL_LAYER_BLEND_OPERATION_DOTPRODUCT:
      return LBX_DOTPRODUCT;
    case GRAL_LAYER_BLEND_OPERATION_BLEND_DIFFUSE_COLOUR:
      return LBX_BLEND_DIFFUSE_COLOUR;
  }
}

LayerBlendSource
convertEnum(gral_layer_blend_source_t bt)
{
  switch (bt) {
    default: ASSERT_NOT_REACHED;
    case GRAL_LAYER_BLEND_SOURCE_CURRENT:
      return LBS_CURRENT;
    case GRAL_LAYER_BLEND_SOURCE_TEXTURE:
      return LBS_TEXTURE;
    case GRAL_LAYER_BLEND_SOURCE_DIFFUSE:
      return LBS_DIFFUSE;
    case GRAL_LAYER_BLEND_SOURCE_SPECULAR:
      return LBS_SPECULAR;
    case GRAL_LAYER_BLEND_SOURCE_MANUAL:
      return LBS_MANUAL;
  }
}

TexCoordCalcMethod
convertEnum(gral_tex_coord_calc_method_t m)
{
  switch (m) {
    default: ASSERT_NOT_REACHED;
    case GRAL_TEX_COORD_CALC_METHOD_NONE:
      return TEXCALC_NONE;
    case GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP:
      return TEXCALC_ENVIRONMENT_MAP;
    case GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_PLANAR:
      return TEXCALC_ENVIRONMENT_MAP_PLANAR;
    case GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_REFLECTION:
      return TEXCALC_ENVIRONMENT_MAP_REFLECTION;
    case GRAL_TEX_COORD_CALC_METHOD_ENVIRONMENT_MAP_NORMAL:
      return TEXCALC_ENVIRONMENT_MAP_NORMAL;
    case GRAL_TEX_COORD_CALC_METHOD_PROJECTIVE_TEXTURE:
      return TEXCALC_PROJECTIVE_TEXTURE;
  }
}
