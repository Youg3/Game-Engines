/// \file VisualDebugger.h
///
/// \brief A set of routines for visualisation, rendering, handling keyboard and mouse events.
/// \date September, 2011
/// \author Grzegorz Cielniak
///

#pragma once

#include "Extras\DebugRenderer.h"

///
///Rendering mode.
///
enum RenderingMode
{
	/// render solid shapes
	RENDER_SOLID,
	/// render debug wireframe
	RENDER_WIREFRAME,
	/// render both solid shapes and wireframe
	RENDER_BOTH			
};

///Initialise the framework.
void Init(int argc, char** argv);

///Start the main loop.
void StartMainLoop();

///Initialise HUD.
void InitHUD();

///Rendering callback.
void RenderCallback();

///Render all actors on the scene.
void RenderActors(bool shadows);

///Setup the camera view.
void SetupCamera();

///Reset the camera view.
void ResetCamera();

///Resize window callback.
void ReshapeCallback(int width, int height);

///Handle key presses.
void KeyPress(unsigned char key, int x, int y);

///Handle key releases.
void KeyRelease(unsigned char key, int x, int y);

///Handle key holds.
void KeyHold();

///Handle special key presses.
void KeySpecial(int key, int x, int y);

///Handle mouse events.
void MouseCallback(int button, int state, int x, int y);

///Handle mouse movement.
void MotionCallback(int x, int y);

///Idle callback.
void IdleCallback();

///Exit callback.
void ExitCallback();

///Print out the help information.
void PrintControls();