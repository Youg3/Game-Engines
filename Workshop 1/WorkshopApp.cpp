#include <stdio.h>
#include "Simulation.h"
#include "VisualDebugger.h"

int main(int argc, char** argv)
{
	// Initialise the framework
	Init(argc, argv);

	//Start the main loop.
	StartMainLoop();
	
	return 0;
}
