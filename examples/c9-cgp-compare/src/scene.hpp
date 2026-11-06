#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include <string>

using cgp::mesh_drawable;


// The parameters of the demo, declared to the bridge in initialize(). Two copies of the demo, side by
// side in a slide, differ by "normals" (given in the address of each frame) and share the others
// (set by the same buttons and slider of the slide).
struct deformation_parameters {
	std::string normals = "exact"; // exact: the normals of the deformed surface; plane: those of the flat plane
	float amplitude = 0.06f;       // A
	float frequency = 20.0f;       // k
	bool play = true;
	bool gui = false;              // the ImGui window of the template (hidden in a slide)
};


struct scene_structure : cgp::scene_inputs_generic {

	void initialize();
	void display_frame();
	void display_gui();

	environment_structure environment;
	window_structure window;
	input_devices inputs;

	camera_controller_orbit_euler camera_control;
	camera_projection_perspective camera_projection;

	mesh_drawable surface;   // a grid, deformed in the vertex shader
	deformation_parameters parameters;
	float t = 0;             // the time of the deformation, restarted by "restart"

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
