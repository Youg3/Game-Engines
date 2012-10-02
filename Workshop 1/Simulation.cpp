#include "Simulation.h"
#include "Extras/Timing.h"
#include <stdio.h>

//global variables
NxPhysicsSDK* physx = 0;
NxScene* scene = 0;
NxReal delta_time;

//actors
NxActor* groundPlane = 0;
NxActor* box = 0;

//user function declarations
NxActor* CreateGroundPlane();
NxActor* CreateBox();

///
/// Initialise the SDK, hardware support, debugging parameters, default gravity etc.
///
bool InitPhysX()
{
	//initialise the SDK
	physx = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION);
	if (!physx) return false;

	//visual debugging 
	physx->setParameter(NX_VISUALIZATION_SCALE, 1);
	physx->setParameter(NX_VISUALIZE_COLLISION_SHAPES, 1);
	physx->setParameter(NX_VISUALIZE_ACTOR_AXES, 1);

	//attach a remote debugger: PhysX Visual Debugger
	if (physx->getFoundationSDK().getRemoteDebugger())
		physx->getFoundationSDK().getRemoteDebugger()->connect("localhost", 5425, NX_DBG_EVENTMASK_EVERYTHING);

    //scene descriptor
    NxSceneDesc sceneDesc;
	sceneDesc.gravity = NxVec3(0,-9.8f,0);	//default gravity

	//check the hardware option first
	sceneDesc.simType = NX_SIMULATION_HW;
    scene = physx->createScene(sceneDesc);	

	//if not available run in software
	if(!scene)
	{ 
		sceneDesc.simType = NX_SIMULATION_SW; 
		scene = physx->createScene(sceneDesc);  
		if(!scene) return false;
	}

	//update the current time
	getElapsedTime();

	return true;
}

///
/// Release all resources.
///
void ReleasePhysX()
{
	if (scene) physx->releaseScene(*scene);
	if (physx) physx->release();
}

///
/// Restart the SDK and the scene.
///
void ResetPhysX()
{
	ReleasePhysX();
	InitPhysX();
	InitScene();
}

///
/// Start the processing of simulation using the elapsed time variable.
///
void SimulationStep()
{
	// Update the time step
	delta_time = getElapsedTime();

	// perform a simulation step for delta time since the last frame
	scene->simulate(delta_time);
	scene->flushStream();
}

///
/// Collect the simulation results. Complementary to SimulationStep function.
///
void GetPhysicsResults()
{
	scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
}

///
/// Initialise all actors and their properties here.
///
void InitScene()
{
	//init actors
	groundPlane = CreateGroundPlane();
	box = CreateBox();
}

///
/// Implement any manipulation on actors here.
///
void UpdateScene()
{
}

NxActor* CreateGroundPlane()
{
	// Create a static plane with a default descriptor
	NxActorDesc actorDesc;
	NxPlaneShapeDesc planeDesc;
	actorDesc.shapes.pushBack(&planeDesc);
	return scene->createActor(actorDesc);
}

NxActor* CreateBox()
{
	// Set the initial height of the box to 3.5m so that it starts off falling onto the ground
	NxReal boxStartHeight = 3.5; 

	// Actor, body and shape descriptors
	NxActorDesc actorDesc;
	NxBodyDesc bodyDesc;
	NxBoxShapeDesc boxDesc;

	// The actor has one shape, a box, 1m on a side
	boxDesc.dimensions.set(0.5, 0.5, 0.5);	//requires half dimension values
	actorDesc.shapes.pushBack(&boxDesc);

	actorDesc.body			= &bodyDesc;
	actorDesc.density		= 10.0f; // kg/m^3
	actorDesc.globalPose.t	= NxVec3(0, boxStartHeight, 0);	

	return scene->createActor(actorDesc);	
}
