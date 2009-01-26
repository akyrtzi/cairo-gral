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

#include "OgreCairoRenderer.h"
#include "OgreCairoCanvas.h"
#include <cairo-gral.h>
#include <gral-ogre.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>

using namespace Ogre;

CairoRenderer::CairoRenderer()
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  if (StringUtil::startsWith(rs->getName(), "Direct3D", false/*lowerCase*/)) {
    if (rs->getConfigOptions()["Floating-point mode"].currentValue == "Fastest")
      OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Direct3D's floating-point mode should be 'Consistent' "
                  "not 'Fastest', otherwise cairo will not work.",
                  "CairoRenderer::CairoRenderer");

  }
}

CairoRenderer::~CairoRenderer()
{
  for (VPSurfaces::iterator I=mSurfaces.begin(), E=mSurfaces.end(); I!=E; ++I) {
    I->first->getTarget()->removeListener(this);
    cairo_surface_destroy(I->second);
  }
}

void CairoRenderer::addCanvas(CairoCanvas *canvas, Viewport *vp)
{
  std::pair<VPCanvases::iterator, VPCanvases::iterator> range = mCanvases.equal_range(vp);
  VPCanvases::iterator canvasIt = range.first;
  for (; canvasIt != range.second; ++canvasIt)
    if (canvasIt->second == canvas)
      break;
  if (canvasIt == range.second) {
    // The canvas isn't attached to this viewport, add it.
    mCanvases.insert(VPCanvasPair(vp, canvas));

    if (range.first == range.second) {
      // This is the first canvas that will be attached to this viewport.
      attachToViewport(vp);
    }
  }
}

void CairoRenderer::removeCanvas(CairoCanvas *canvas, Viewport *vp)
{
  std::pair<VPCanvases::iterator, VPCanvases::iterator> range = mCanvases.equal_range(vp);
  VPCanvases::iterator canvasIt = range.first;
  for (; canvasIt != range.second; ++canvasIt)
    if (canvasIt->second == canvas)
      break;
  if (canvasIt != range.second) {
    // The canvas is attached to this viewport, remove it.
    mCanvases.erase(canvasIt);

    if (mCanvases.find(vp) == mCanvases.end()) {
      // The viewport has no canvases attached to it.
      detachFromViewport(vp);
    }
  }
}

void CairoRenderer::attachToViewport(Viewport *vp)
{
  gral_surface_t *gral_srf = gral_ogre_surface_from_viewport(vp);
  cairo_surface_t *surface = cairo_gral_surface_create(gral_srf);
  assert(surface);
  assert(mSurfaces[vp] == NULL);
  mSurfaces[vp] = surface;

  vp->getTarget()->addListener(this);
}

void CairoRenderer::detachFromViewport(Viewport *vp)
{
  vp->getTarget()->removeListener(this);

  assert(mSurfaces[vp]);
  cairo_surface_destroy(mSurfaces[vp]);
  mSurfaces.erase(vp);
}

void CairoRenderer::postViewportUpdate(const RenderTargetViewportEvent& evt)
{
  Viewport *vp = evt.source;
  std::pair<VPCanvases::iterator, VPCanvases::iterator> range = mCanvases.equal_range(vp);

  bool renderInited = false;
  for (VPCanvases::iterator I = range.first; I != range.second; ++I) {
    CairoCanvas *canvas = I->second;

    if (!canvas->needsRendering())
      continue;

    if (!renderInited) {
      initialiseRenderState(vp);
      renderInited = true;
    }

    assert(mSurfaces[vp]);
    cairo_t *cr = cairo_create(mSurfaces[vp]);
    canvas->onDraw(cr);
    cairo_destroy(cr);
  }

  if (renderInited)
    finaliseRenderState();
}

void CairoRenderer::initialiseRenderState(Viewport *vp) const
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_setViewport(vp);
  rs->_beginFrame();

  rs->_setDepthBias(0, 0);
  rs->_setFog(FOG_NONE);
  rs->unbindGpuProgram(GPT_GEOMETRY_PROGRAM);
}

void CairoRenderer::finaliseRenderState() const
{
  RenderSystem *rs = Root::getSingleton().getRenderSystem();
  rs->_endFrame();
}
