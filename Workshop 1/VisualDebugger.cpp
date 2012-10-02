#include "VisualDebugger.h"
#include "Simulation.h"
#include "Extras/HUD.h"
#include "Extras/DrawObjects.h"
#include "Extras/Timing.h"
#include "Extras/UserData.h"
#include <GL/glut.h>

//extern variables, defined in Simulation.cpp
extern NxScene* scene;
extern NxReal delta_time;

//global variables
bool bHardwareScene = false;
bool bPause = false;
bool bShadows = true;
RenderingMode rendering_mode = RENDER_SOLID;
DebugRenderer gDebugRenderer;
const NxDebugRenderable* debugRenderable = 0;
NxActor* gSelectedActor = 0;
HUD hud;

// Force globals
NxVec3	gForceVec(0,0,0);
NxReal	gForceStrength	= 20000;

// Keyboard globals
#define MAX_KEYS 256
bool gKeys[MAX_KEYS];

// Camera globals
float	gCameraAspectRatio = 1.0f;
NxVec3	gCameraPos(0,5,-15);
NxVec3	gCameraForward(0,0,1);
NxVec3	gCameraRight(-1,0,0);
const NxReal gCameraSpeed = 10;

///
/// Assign callback functions, set-up lighting and initialise HUD.
///
void Init(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	//callbacks
	glutSetWindow(glutCreateWindow("Workshop 1"));
	glutDisplayFunc(RenderCallback);
	glutReshapeFunc(ReshapeCallback);
	glutIdleFunc(IdleCallback);
	glutKeyboardFunc(KeyPress);
	glutKeyboardUpFunc(KeyRelease);
	glutSpecialFunc(KeySpecial);
	glutMouseFunc(MouseCallback);
	glutMotionFunc(MotionCallback);
	atexit(ExitCallback);

	//default render states
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_CULL_FACE);

	//lighting
	glEnable(GL_LIGHTING);
	float AmbientColor[]    = { 0.0f, 0.1f, 0.2f, 0.0f };
	float AmbientColor2[]    = { 0.2f, 0.1f, 0.0f, 0.0f };			
	float DiffuseColor[]    = { 0.2f, 0.2f, 0.2f, 0.0f };			
	float SpecularColor[]   = { 0.5f, 0.5f, 0.5f, 0.0f };			
	float Position[]        = { 100.0f, 100.0f, -400.0f, 1.0f };

	//default lighting
	glLightfv(GL_LIGHT0, GL_AMBIENT, AmbientColor);	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, DiffuseColor);	
	glLightfv(GL_LIGHT0, GL_SPECULAR, SpecularColor);	
	glLightfv(GL_LIGHT0, GL_POSITION, Position);
	glEnable(GL_LIGHT0);

	//alternative lighting
	glLightfv(GL_LIGHT1, GL_AMBIENT, AmbientColor2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, DiffuseColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, SpecularColor);
	glLightfv(GL_LIGHT1, GL_POSITION, Position);

	// Initialise the PhysX SDK.
	if (!InitPhysX())
	{
		printf("Could not initialise PhysX.\n");
		ExitCallback();
	}

	// Initialise the simulation scene.
	InitScene();
	
	MotionCallback(0,0);

	PrintControls();

	InitHUD();
}

///
/// Start the first step of simulation and enter the GLUT's main loop.
///
void StartMainLoop() 
{
	glutMainLoop(); 
}

///
/// Initialise HUD with default display strings for simulation type and a pause message.
///
void InitHUD()
{
	hud.Clear();

	//hardware/software message
	if (scene->getSimType() == NX_SIMULATION_HW)
		hud.AddDisplayString("Hardware Scene", 0.74f, 0.92f);
	else
		hud.AddDisplayString("Software Scene", 0.74f, 0.92f);

	//pause message
	hud.AddDisplayString("", 0.3f, 0.55f);
}

void Display()
{
	//clear display buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera view
	SetupCamera();

	//display scene
	if (rendering_mode != RENDER_WIREFRAME)
		RenderActors(bShadows);
	if (debugRenderable)
		gDebugRenderer.renderData(*debugRenderable);

	//render HUD
	hud.Render();

	glFlush();
	glutSwapBuffers();
}

///
/// Perform one step of simulation, process input keys and render the scene.
///
void RenderCallback()
{
	if (scene && !bPause)
	{
		//get new results
		GetPhysicsResults();

		debugRenderable = 0;
		if (rendering_mode != RENDER_SOLID)
			debugRenderable = scene->getDebugRenderable();

		//user defined process function
		UpdateScene();

		//handle keyboard
		KeyHold();

		//start new simulation step
		SimulationStep();
	}

	Display();
}

///
/// Draw a force arrow.
///
void DrawForce(NxActor* actor, NxVec3& forceVec, const NxVec3& color)
{
	if (actor)
	{
		// Draw only if the force is large enough
		NxReal force = forceVec.magnitude();
		if (force < 0.1)  return;

		forceVec = 2*forceVec/force;

		NxVec3 pos = actor->getCMassGlobalPosition();
		DrawArrow(pos, pos + forceVec, color);
	}
}

///
/// Render all actors in default colour. Render the selected actor in alternative colour.
///
void RenderActors(bool shadows)
{
	//iterate through all actors
	NxU32 nbActors = scene->getNbActors();
	NxActor** actors = scene->getActors();
	while (nbActors--)
	{
		NxActor* actor = *actors++;

		if (actor == gSelectedActor) //draw the selected actor using GL_LIGHT1
		{
			ActorUserData ud;
			ud.flags |= UD_RENDER_USING_LIGHT1;
			actor->userData = &ud;
			DrawActor(actor, 0, false);
			actor->userData = 0;
			//draw force arrow
			DrawForce(gSelectedActor, gForceVec, NxVec3(1,1,0));
		}
		else
			DrawActor(actor, 0, false); //draw all actors using GL_LIGHT0

		//draw shadows
		if (shadows)
			DrawActorShadow(actor, false);
	}
}

///
/// Check if the actor is selectable (dynamic or kinematic).
///
bool IsSelectable(NxActor* actor)
{
   NxShape*const* shapes = gSelectedActor->getShapes();
   NxU32 nShapes = gSelectedActor->getNbShapes();
   while (nShapes--)
       if (shapes[nShapes]->getFlag(NX_TRIGGER_ENABLE)) 
           return false;

   if(!actor->isDynamic())
	   return false;

   return true;
}

///
/// Select the next actor on the scene.
//
void SelectNextActor()
{
   NxU32 nbActors = scene->getNbActors();
   NxActor** actors = scene->getActors();
   for(NxU32 i = 0; i < nbActors; i++)
   {
       if (actors[i] == gSelectedActor)
       {
           NxU32 j = 1;
           gSelectedActor = actors[(i+j)%nbActors];
           while (!IsSelectable(gSelectedActor))
           {
               j++;
               gSelectedActor = actors[(i+j)%nbActors];
           }
           break;
       }
   }

   if (!gSelectedActor)
   {
	   for(NxU32 i = 0; i < nbActors; i++)
	   {
		   gSelectedActor = actors[i];
		   if (IsSelectable(gSelectedActor))
			   return;
	   }
	   gSelectedActor = 0; // none found
   }
}

/// 
/// Apply a force vector to the selected actor.
///
NxVec3 ApplyForceToActor(NxActor* actor, const NxVec3& forceDir, const NxReal forceStrength)
{
	NxVec3 forceVec = forceStrength*forceDir*delta_time;
	if (actor)
		actor->addForce(forceVec);
	return forceVec;
}

///
/// Setup the camera view veriables.
///
void SetupCamera()
{
	gCameraAspectRatio = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);

	// Setup camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, gCameraAspectRatio, 1.0f, 10000.0f);
	gluLookAt(gCameraPos.x, gCameraPos.y, gCameraPos.z,
		gCameraPos.x+gCameraForward.x, gCameraPos.y+gCameraForward.y, gCameraPos.z+gCameraForward.z,
		0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

///
/// Setup the camera view variables.
///
void ResetCamera()
{
	gCameraAspectRatio = 1.0f;
	gCameraPos = NxVec3(0,5,-15);
	gCameraForward = NxVec3(0,0,1);
	gCameraRight = NxVec3(-1,0,0);
}

///
/// Change the view point and aspect ratio.
///
void ReshapeCallback(int width, int height)
{
	glViewport(0, 0, width, height);
	gCameraAspectRatio = float(width)/float(height);
}

///
/// Handle all single key presses. 
///
void KeyPress(unsigned char key, int x, int y)
{
	if (!gKeys[key]) // ensure the keypress is only executed once
	{
		switch (key)
		{
		case 'b': //toggle between different rendering modes
			if (rendering_mode == RENDER_SOLID)
				rendering_mode = RENDER_WIREFRAME;
			else if (rendering_mode == RENDER_WIREFRAME)
				rendering_mode = RENDER_BOTH;
			else if (rendering_mode == RENDER_BOTH)
				rendering_mode = RENDER_SOLID;
			break;
		case 'p':
			bPause = !bPause; 
			if (bPause)
				hud.SetDisplayString(1, "Paused - Hit \"p\" to Unpause", 0.3f, 0.55f);
			else
				hud.SetDisplayString(1, "", 0.3f, 0.55f);	
			getElapsedTime(); 
			break; 
		case 'r':
			SelectNextActor();
			break;
		case 'x': 
			bShadows = !bShadows; 
			break;
		case 27: //ESC
			exit(0);
			break;
		default:
			break;
		}
	}

	gKeys[key] = true;
}

///
/// Handle all depressed keys.
///
void KeyHold()
{
	// Process camera keys
	for (int i = 0; i < MAX_KEYS; i++)
	{	
		if (!gKeys[i])  { continue; }

		switch (i)
		{
			// Camera controls
		case 'w':
			gCameraPos += gCameraForward*gCameraSpeed*delta_time;
			break;
		case 's':
			gCameraPos -= gCameraForward*gCameraSpeed*delta_time;
			break;
		case 'a':
			gCameraPos -= gCameraRight*gCameraSpeed*delta_time;
			break;
		case 'd':
			gCameraPos += gCameraRight*gCameraSpeed*delta_time;
			break;
		case 'z':
			gCameraPos -= NxVec3(0,1,0)*gCameraSpeed*delta_time;
			break;
		case 'q':
			gCameraPos += NxVec3(0,1,0)*gCameraSpeed*delta_time;
			break;
		// Force controls
		case 'i':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(0,0,1),gForceStrength);
			break;
		case 'k':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(0,0,-1),gForceStrength);
			break;
		case 'j':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(1,0,0),gForceStrength);
			break;
		case 'l':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(-1,0,0),gForceStrength);
			break;
		case 'u':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(0,1,0),gForceStrength);
			break;
		case 'm':
			gForceVec = ApplyForceToActor(gSelectedActor,NxVec3(0,-1,0),gForceStrength);
			break;
		}
	}
}

///
/// Handle all key releases.
///
void KeyRelease(unsigned char key, int x, int y)
{
	gKeys[key] = false;

	switch (key)
	{
	//force controls
	case 'i': case 'k': case 'j': case 'l': case 'u': case 'm':
		gForceVec = NxVec3(0,0,0);
		break;
	default:
		break;
	}
}

///
/// Handle all special keys.
///
void KeySpecial(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F10: // Reset PhysX and View
		ResetPhysX();
		ResetCamera();
		gSelectedActor = 0;
		break; 
	default:
		break;
	}
}

//mouse functions
int mx = 0;
int my = 0;

///
/// Handle mouse events.
///
void MouseCallback(int button, int state, int x, int y)
{
	mx = x;
	my = y;
}

///
/// Set-up camera view variables depending on mouse movement.
///
void MotionCallback(int x, int y)
{
	int dx = mx - x;
	int dy = my - y;

	gCameraForward.normalize();
	gCameraRight.cross(gCameraForward,NxVec3(0,1,0));

	NxQuat qx(NxPiF32 * dx * 20 / 180.0f, NxVec3(0,1,0));
	qx.rotate(gCameraForward);
	NxQuat qy(NxPiF32 * dy * 20 / 180.0f, gCameraRight);
	qy.rotate(gCameraForward);

	mx = x;
	my = y;
}

void IdleCallback() { glutPostRedisplay(); }

///
/// Release PhysX SDK on exit.
///
void ExitCallback() { ReleasePhysX(); }

///
/// Simple info printed out in the command line.
///
void PrintControls()
{
	printf("\n Flight Controls:\n ----------------\n w = forward, s = back\n a = strafe left, d = strafe right\n q = up, z = down\n");
    printf("\n Force Controls:\n ---------------\n i = +z, k = -z\n j = +x, l = -x\n u = +y, m = -y\n");
	printf("\n Miscellaneous:\n --------------\n p   = Pause\n x   = Toggle Shadows\n r   = Select Actor\n  b   = Toggle Visualisation Mode\n F10 = Reset scene\n ESC = Exit\n");
}
