#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include <string>
#include <vector>

using cgp::mesh_drawable;


// The parameters of the demo, declared to the bridge in initialize()
struct model_parameters {
	std::string model = "camel";  // the model shown: camel (one OBJ file) or sponza (OBJ, MTL and textures)
	bool turn = true;             // the camel turns by itself
	bool gui = false;             // the ImGui window of the template (hidden in a slide)
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

	mesh_drawable camel;                  // a single mesh, without texture
	std::vector<mesh_drawable> sponza;    // one mesh per material, each with its texture
	float camel_triangles = 0;            // the number of triangles of each model (readout)
	float sponza_triangles = 0;           // known once Sponza is read
	model_parameters parameters;
	float angle = 0;

	void place_camera();
	void load_sponza();

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
