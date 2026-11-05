#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include <string>

using cgp::mesh_drawable;
using cgp::curve_drawable;
using cgp::numarray;
using cgp::vec3;


// Settings of the curve, declared to the bridge in initialize()
struct curve_parameters {
	std::string type = "bezier";  // bezier (approaches the points) or spline (Catmull-Rom, goes through them)
	int points = 4;               // number of control points
	bool polygon = true;          // the control polygon
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

	curve_parameters settings;
	numarray<vec3> control;       // the control points, moved with the mouse
	int picked = -1;              // index of the dragged point, -1 if none

	mesh_drawable ball;           // one sphere, drawn at each control point
	curve_drawable curve;         // the curve, sampled
	curve_drawable polygon;       // the control polygon
	numarray<vec3> samples;
	float length = 0;             // length of the curve, shown by the slide
	float notified_length = -1;

	void reset_points();
	void update_curve();

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
