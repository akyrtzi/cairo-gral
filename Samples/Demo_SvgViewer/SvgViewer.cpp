/*
-----------------------------------------------------------------------------
This source file uses OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/
/*
-----------------------------------------------------------------------------
Filename:    SvgViewer.cpp
Description: Demo of loading and displaying SVG (Scalable Vector Graphics) files.

Next SVG image:     ']'
Previous SVG image: '['

Start/Stop animation: SPACE

Pan: Mouse:    Left button + move
     Keyboard: A/W/S/D or Left/Right/Up/Down

Zoom: Mouse:    Right button + move
      Keyboard: '='/'-'
-----------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "OgreCairoRenderer.h"
#include "OgreCairoCanvas.h"
#include <svg-cairo.h>

typedef std::vector<svg_cairo_t *> SvgContexts;

// Listener class for frame updates
class SvgListener : public ExampleFrameListener, public CairoCanvas
{
    FrameEvent frameEvent;
    Vector2 translate;
    Real scale;
    bool spinning;
    int svgIndex;

    cairo_matrix_t cr_mat_rotation;
    SvgContexts svgs;

public:
    SvgListener(SvgContexts &svgCtxs, RenderWindow* win, Camera* cam)
        : ExampleFrameListener(win, cam)
    {
      svgs = svgCtxs;
      translate = Vector2::ZERO;
      scale = 1;
      spinning = true;
      svgIndex = 0;
      cairo_matrix_init_identity(&cr_mat_rotation);
    }

    // Implement CairoCanvas::onDraw.
    virtual void onDraw(cairo_t *cr) {

        // Boost performance of bezier spline rendering. (1 pixel tolerance)
        cairo_set_tolerance(cr, 1);

        setupCairoMatrix(cr);

        svg_cairo_render (svgs[svgIndex], cr);
    }

    void setupCairoMatrix(cairo_t *cr) {
        if (spinning) {
          // Rotate a bit at each frame.
          Degree angle = Degree(frameEvent.timeSinceLastFrame * 60);
          cairo_matrix_rotate(&cr_mat_rotation, angle.valueRadians());
        }

        unsigned int svg_width, svg_height;
        svg_cairo_get_size(svgs[svgIndex], &svg_width, &svg_height);
        Vector2 picCenter(svg_width/2, svg_height/2);

        unsigned int width, height;
        getCanvasSize(cr, width, height); // From CairoCanvas' methods.
        Vector2 screenCenter(width/2, height/2);

        cairo_matrix_t mat;
        cairo_matrix_init_translate(&mat, -picCenter.x*scale, -picCenter.y*scale);
        cairo_matrix_multiply(&mat, &mat, &cr_mat_rotation);
        cairo_matrix_scale(&mat, scale, scale);

        cairo_matrix_t pan_mat;
        Vector2 pan = screenCenter + translate*scale;
        cairo_matrix_init_translate(&pan_mat, pan.x, pan.y);
        cairo_matrix_multiply(&mat, &mat, &pan_mat);

        cairo_set_matrix(cr, &mat);
    }

    bool frameRenderingQueued(const FrameEvent& evt)
    {
	if( ExampleFrameListener::frameRenderingQueued(evt) == false )
		return false;

        frameEvent = evt;
        return true;
    }

    virtual bool processUnbufferedMouseInput(const FrameEvent& evt)
    {
        // Rotation factors, may not be used if the second mouse button is pressed
        // 2nd mouse button - slide, otherwise rotate
        const OIS::MouseState &ms = mMouse->getMouseState();
        if( ms.buttonDown( OIS::MB_Left ) )
        {
          translate.x += ms.X.rel/scale;
          translate.y += ms.Y.rel/scale;
        }

        if( ms.buttonDown( OIS::MB_Right ) )
        {
          scale += ms.Y.rel * 0.01;
        }

        return true;
    }

    virtual bool processUnbufferedKeyInput(const FrameEvent& evt) {
        if (! ExampleFrameListener::processUnbufferedKeyInput(evt))
          return false;

        if(mKeyboard->isKeyDown(OIS::KC_LEFT) || mKeyboard->isKeyDown(OIS::KC_A))
          translate.x -= (mMoveScale*2)/scale;	// Move camera left

        if(mKeyboard->isKeyDown(OIS::KC_RIGHT) || mKeyboard->isKeyDown(OIS::KC_D))
          translate.x += (mMoveScale*2)/scale;	// Move camera RIGHT

        if(mKeyboard->isKeyDown(OIS::KC_UP) || mKeyboard->isKeyDown(OIS::KC_W) )
          translate.y -= (mMoveScale*2)/scale;	// Move camera forward

        if(mKeyboard->isKeyDown(OIS::KC_DOWN) || mKeyboard->isKeyDown(OIS::KC_S) )
          translate.y += (mMoveScale*2)/scale;	// Move camera backward

        if (mKeyboard->isKeyDown(OIS::KC_EQUALS))
          scale += mMoveScale/50;

        if (mKeyboard->isKeyDown(OIS::KC_MINUS))
          scale -= mMoveScale/50;

        if( mKeyboard->isKeyDown(OIS::KC_LBRACKET) && mTimeUntilNextToggle <= 0 ) {
          svgIndex--;
          if (svgIndex < 0)
            svgIndex = svgs.size()-1;
          mTimeUntilNextToggle = 1;
        }

        if( mKeyboard->isKeyDown(OIS::KC_RBRACKET) && mTimeUntilNextToggle <= 0 ) {
          svgIndex++;
          if (svgIndex == svgs.size())
            svgIndex = 0;
          mTimeUntilNextToggle = 1;
        }

        if( mKeyboard->isKeyDown(OIS::KC_SPACE) && mTimeUntilNextToggle <= 0 ) {
          spinning = !spinning;
          mTimeUntilNextToggle = 1;
        }

        return true;
    }
};

class CairoTexApplication : public ExampleApplication
{
public:
    CairoTexApplication() : cairoRenderer(0) {}
    ~CairoTexApplication() {
      delete cairoRenderer;
    }

protected:
    CairoRenderer *cairoRenderer;
    SvgContexts svgs;

    void createFrameListener(void)
    {
        SvgListener *svgListener = new SvgListener(svgs, mWindow, mCamera);
        // For CairoCanvas
        cairoRenderer->addCanvas(svgListener, mWindow->getViewport(0));

        // For ExampleFrameListener
        mFrameListener = svgListener;
        mRoot->addFrameListener(svgListener);
    }

    // Just override the mandatory create scene method
    void createScene(void)
    {
        cairoRenderer = new CairoRenderer();

        FileInfoListPtr fileList =
          ResourceGroupManager::getSingleton().findResourceFileInfo(
              ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
              "*.svg");

        for (FileInfoList::iterator I=fileList->begin(), E=fileList->end();
             I != E; ++I) {
          FileInfo &file = *I;

          svg_cairo_t *svg_cr;
          svg_cairo_status_t status = svg_cairo_create(&svg_cr);
          if (status != SVG_CAIRO_STATUS_SUCCESS)
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
            "svg_cairo context creation failed!",
            "SvgApplication::createScene");

          String relPath = String("../../Samples/Media/") + file.path + file.filename;
          status = svg_cairo_parse(svg_cr, relPath.c_str());
          if (status != SVG_CAIRO_STATUS_SUCCESS)
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
            "Svg file parsing failed!",
            "SvgApplication::createScene");

          svgs.push_back(svg_cr);
        }

        if (svgs.size() == 0)
          OGRE_EXCEPT(Exception::ERR_INVALID_STATE,
                     "No svg file found!",
                     "SvgApplication::createScene");

        mWindow->getViewport(0)->setBackgroundColour(ColourValue(0.8, 0.8, 0.8));

        Overlay *overlay = OverlayManager::getSingleton().getByName("SvgViewerOverLay");
        overlay->show();
    }
};



#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char **argv)
#endif
{
    // Create application object
    CairoTexApplication app;

    try {
        app.go();
    } catch( Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occured: " << e.getFullDescription();
#endif
    }


    return 0;
}

#ifdef __cplusplus
}
#endif
