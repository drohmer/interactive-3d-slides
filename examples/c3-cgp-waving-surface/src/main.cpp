// CGP library
#include "cgp/cgp.hpp"

// Running application
#include "application.hpp"

// Custom scene of this code
#include "scene.hpp"

// slide-demos: the parameters of the demo, set from the slide
#include "slide_bridge.hpp"


int main(int argc, char* argv[])
{
	// The only change to the main() of the template: keep the arguments
	// (--ampl 0.2 ..., given by the address of the iframe)
	slide::parse(argc, argv);

	scene_structure scene;
	application_structure app;
	app.initialize(argv[0], &scene);

	app.start_loop();

	return 0;
}
