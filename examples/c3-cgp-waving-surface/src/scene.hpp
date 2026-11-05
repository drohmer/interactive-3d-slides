#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include <string>

using cgp::mesh;
using cgp::mesh_drawable;


// The parameters of the wave: declared to the bridge in initialize(), so that the page,
// the slide and the ImGui window change the same variables
struct wave_parameters {
	float amplitude = 0.12f;
	float wave_number = 9.0f;     // spatial frequency (radians per unit of length)
	float speed = 4.0f;           // temporal frequency (radians per second)
	float damping = 1.0f;         // decay with the distance to the center
	std::string shape = "ripple"; // ripple (from the center) or waves (parallel)
	bool wireframe = false;
	bool play = true;
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

	mesh surface_mesh;            // CPU copy, moved at each frame
	mesh_drawable surface;        // GPU buffers

	// The wireframe as segments: glPolygonMode (used by draw_wireframe) does not exist in WebGL
	cgp::curve_drawable wire;
	cgp::numarray<cgp::uint2> wire_edges;   // pairs of vertices of the grid lines
	cgp::numarray<cgp::vec3> wire_position; // their two ends, updated at each frame
	wave_parameters wave;
	float t = 0;                  // time since the last drop

	void update_wire();

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
