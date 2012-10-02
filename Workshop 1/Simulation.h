/// \file Simulation.h
///
/// \brief PhysX routines and user defined functions.
/// \date September, 2011
/// \author Grzegorz Cielniak
///

#pragma once

#include "NxPhysics.h"
#include "NxCooking.h"

/// Initialise the scene.
void InitScene();

/// User defined routine.
void UpdateScene();

/// Initialise the PhysX SDK.
bool InitPhysX();

/// Release the PhysX SDK.
void ReleasePhysX();

/// Reset the PhysX SDK.
void ResetPhysX();

/// Start a single step of simulation.
void SimulationStep();

/// Collect the simulation results.
void GetPhysicsResults();

