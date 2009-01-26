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
Filename:    CairoTexture.cpp
Description: Demo of using cairo to draw to a texture.

Clock drawing code was based on a BetaCairo sample.
-----------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "OgreCairoRenderer.h"
#include "OgreCairoCanvas.h"
#include <sys/timeb.h>
#include <time.h>

#define CLOCK_UPDATE 0.125f

class CairoClock : public CairoCanvas, public ExampleFrameListener {
  float timeSince, hours, minutes, seconds;

public:
  CairoClock(RenderWindow* win, Camera* cam) : ExampleFrameListener(win,cam) {
    timeSince = 1000;
  }

  bool frameRenderingQueued(const FrameEvent& evt) {
    timeSince += evt.timeSinceLastFrame;
    return true;
  }

  virtual void onDraw(cairo_t *cr) {

    // Get the current time.
    getTime();

    // Basic bits:

    unsigned int width, height;
    getCanvasSize(cr, width, height); // From CairoCanvas' methods.

    // Paint the Drawing with red, this pretty much wipes over the previous drawing.
    cairo_set_source_rgb(cr, 1,0,0);
    cairo_paint(cr);

    // Set some nice smooth line ends.
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // Draw a outline around the texture, using white colour and a line thickness of 40 pixels.
    cairo_set_source_rgb(cr, 1,1,1);
    cairo_set_line_width(cr, 40);
    cairo_rectangle(cr, 0,0, width, height);
    cairo_stroke(cr);

    // The Clock face:

    // Path out an arc (semi or full circle)
    cairo_arc(cr, 
              width / 2,	// X (Center of the image)
              height / 2, // Y (Center of the image)
              width / 4,	// Radius (1/4 of the image size)
              0,				// Start at 0 Degrees
              Math::PI*2				// End at 360 Degrees
              );

    // Fill it in (with the previous colour:grey)
    cairo_fill(cr);

    // The Hour hand:

    // Set the colour to red again.
    cairo_set_source_rgb(cr,1,0,0);

    // Set the line thickness of 12 pixels.
    cairo_set_line_width(cr, 12);

    // Move to the center of the image
    cairo_move_to(cr, width/2, height/2);

    // Using a simple bit of trig. path the hour hand from the center to the correct hour, using a length of 1/5th of the imagesize.
    cairo_line_to(cr,
                  (width / 2) + (Math::Sin(hours * Ogre::Math::PI / 6.0f) * (width / 5.0f)),
                  (height / 2) + (-Math::Cos(hours * Ogre::Math::PI / 6.0f) * (height / 5.0f)));

    // Stroke it.
    cairo_stroke(cr);


    // The Minute hand:

    // Move to the center of the image
    cairo_move_to(cr, width/2, height/2);

    // Now draw the hour hand, using a length of 1/6th of the imagesize.
    cairo_line_to(cr,
                  (width / 2) + (Math::Sin((minutes + (seconds / 60.0f)) * Ogre::Math::PI / 30.0f) * (width / 6.0f)), 
                  (height / 2) + (-Math::Cos((minutes + (seconds / 60.0f)) * Ogre::Math::PI / 30.0f) * (height / 6.0f))
                  );

    // Stroke it, but do not delete the path we just drew.
    cairo_stroke_preserve(cr);

    // Set the line thickness to 4 pixels
    cairo_set_line_width(cr, 4);

    // Set the colour to white
    cairo_set_source_rgb(cr, 1,1,1);

    // Now stroke the path again. This gives the line a nice outlined effect.
    cairo_stroke(cr);


    // The second hand, and previous seconds:

    // Set the line thickness to 4 pixels
    cairo_set_line_width(cr, 4);

    // Set the a ColourValue to the Colour whitesmoke
    ColourValue c(0.8,0.8,0.8);

    int lastSecond = ceil(seconds);
    float a = 1.0 / seconds;

    // For each previous second, draw it out, with a subtle fade (alpha). The alpha decreases based how close it is to the current second.
    for (int i=0;i < lastSecond;i++) {
      cairo_set_source_rgba(cr, c.r, c.g, c.b, a * i);
      cairo_move_to(cr, (width / 2)  + (Math::Sin(i * Ogre::Math::PI / 30.0f) * (width / 4) ), (height / 2)  + (-Math::Cos(i * Ogre::Math::PI / 30.0f) * (height / 4)));
      cairo_line_to(cr, (width / 2)  + (Math::Sin(i * Ogre::Math::PI / 30.0f) * (width / 3.5f) ) , (height / 2)  + (-Math::Cos(i * Ogre::Math::PI / 30.0f) * (height / 3.5f)));
      cairo_stroke(cr);
    }	

    // Draw the main second.
    cairo_set_source_rgb(cr, c.r,c.g,c.b);
    cairo_move_to(cr, (width / 2)  + (Math::Sin(seconds * Ogre::Math::PI / 30.0f) * (width / 4) ), (height / 2)  + (-Math::Cos(seconds * Ogre::Math::PI / 30.0f) * (height / 4)));
    cairo_line_to(cr, (width / 2)  + (Math::Sin(seconds * Ogre::Math::PI / 30.0f) * (width / 3.5f) ) , (height / 2)  + (-Math::Cos(seconds * Ogre::Math::PI / 30.0f) * (height / 3.5f)));
    cairo_stroke(cr);

    // Outer ring:

    // Set the colour to white
    cairo_set_source_rgb(cr, 1,1,1);
    // Path out an arc, from the center, for a radius of 1/3.5th of the image
    cairo_arc(cr, (width / 2),(height / 2), (width / 3.5f), 0, Math::PI*2);
    // And stroke it.
    cairo_stroke(cr);

  }

  virtual bool needsRendering() {
    if (timeSince >= CLOCK_UPDATE) {
      timeSince = 0;
      return true;
    }

    return false;
  }

  void getTime() {
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo;
    timeinfo = localtime(&rawtime);
    minutes = timeinfo->tm_min; 
    hours = timeinfo->tm_hour;
    seconds = timeinfo->tm_sec;

    struct _timeb timebuffer;
    _ftime( &timebuffer );
    seconds += (timebuffer.millitm * 0.001f);
  }
}; 


class CairoTexApplication : public ExampleApplication
{
public:
    CairoTexApplication() {
      cairoRenderer = 0;
      clock = 0;
    }
    ~CairoTexApplication() {
      delete cairoRenderer;
      delete clock;
    }

protected:
    CairoRenderer *cairoRenderer;
    CairoClock *clock;

    // Just override the mandatory create scene method
    void createScene(void)
    {
        cairoRenderer = new CairoRenderer();

        uint fsaa = 8;
        RenderSystem *rs = Root::getSingleton().getRenderSystem();
        if (StringUtil::startsWith(rs->getName(), "OpenGL", false/*lowerCase*/)) {
          // There's some weird freezing going on with OpenGL when fsaa != 0,
          // disable it for now.
          fsaa = 0;
        }

        TexturePtr cairoTex = TextureManager::getSingleton().createManual( "CairoTex", 
              ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 
              512, 512, 0, PF_R8G8B8, TU_RENDERTARGET,0,false, fsaa);
        RenderTarget *rttTex = cairoTex->getBuffer()->getRenderTarget();
        Viewport *vp = rttTex->addViewport(NULL);
        vp->setClearEveryFrame( false );
        vp->setOverlaysEnabled( false );

        clock = new CairoClock(mWindow, mCamera);
        cairoRenderer->addCanvas(clock, vp);
        mRoot->addFrameListener(clock);

        MaterialPtr mat = MaterialManager::getSingleton().create("CairoMat",
              ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mat->getTechnique(0)->getPass(0)->createTextureUnitState("CairoTex");

        // Set ambient light
        mSceneMgr->setAmbientLight(ColourValue(0.8, 0.8, 0.8));

        // Create a skydome
        mSceneMgr->setSkyDome(true, "Examples/CloudySky", 5, 8);

        // Create a light
        Light* l = mSceneMgr->createLight("MainLight");
        // Accept default settings: point light, white diffuse, just set position
        // NB I could attach the light to a SceneNode if I wanted it to move automatically with
        //  other objects, but I don't
        l->setPosition(20,80,50);

        Entity *ent;

        // Define a floor plane mesh
        Plane p;
        p.normal = Vector3::UNIT_Y;
        p.d = 200;
        MeshManager::getSingleton().createPlane(
            "FloorPlane", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
            p, 200000, 200000, 20, 20, true, 1, 50, 50, Vector3::UNIT_Z);

        // Create an entity (the floor)
        ent = mSceneMgr->createEntity("floor", "FloorPlane");
        ent->setMaterialName("Examples/RustySteel");
        // Attach to child of root node, better for culling (otherwise bounds are the combination of the 2)
        mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);

        // Add a head, give it it's own node
        SceneNode* cubeNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        ent = mSceneMgr->createEntity("head", "cube.mesh");
        cubeNode->attachObject(ent);

        ent->setMaterialName("CairoMat");

        // Create the camera node & attach camera
        SceneNode* camNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        camNode->attachObject(mCamera);

        mCamera->lookAt(cubeNode->getPosition());

        // Put in a bit of fog for the hell of it
        mSceneMgr->setFog(FOG_EXP, ColourValue::White, 0.0002);
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
