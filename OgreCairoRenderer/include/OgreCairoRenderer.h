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

#ifndef _OGRE_CAIRO_RENDERER_H_
#define _OGRE_CAIRO_RENDERER_H_

#include <cairo.h>
#include <OgreRenderTargetListener.h>
#include <map>

namespace Ogre {

  class CairoCanvas;

class CairoRenderer : public RenderTargetListener {
public:
  CairoRenderer();
  virtual ~CairoRenderer();

  void addCanvas(CairoCanvas *canvas, Viewport *vp);
  void removeCanvas(CairoCanvas *canvas, Viewport *vp);

protected:
  virtual void postViewportUpdate(const RenderTargetViewportEvent& evt);

private:
  typedef std::multimap<Viewport *, CairoCanvas *> VPCanvases;
  typedef std::pair<Viewport *, CairoCanvas *> VPCanvasPair;
  VPCanvases mCanvases;

  typedef std::map<Viewport *, cairo_surface_t *> VPSurfaces;
  VPSurfaces mSurfaces;

  void attachToViewport(Viewport *vp);
  void detachFromViewport(Viewport *vp);
  void initialiseRenderState(Viewport *vp) const;
  void finaliseRenderState() const;
};

}

#endif // _OGRE_CAIRO_RENDERER_H_