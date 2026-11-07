#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include <string>
#include <vector>

using cgp::mesh;
using cgp::mesh_drawable;
using cgp::vec3;


// A stroke: the points given by the mouse, and a ribbon drawn along them. WebGL draws lines one pixel
// wide, too thin on a projector: the ribbon is a mesh, as wide as wanted.
struct stroke_structure {
	std::vector<vec3> points;
	vec3 color;
	float width = 0.03f;
	mesh_drawable ribbon;
};

// The parameters of the demo, declared to the bridge in initialize()
struct sketch_parameters {
	std::string color = "black";  // the color of the next strokes: black, red, blue
	float width = 0.03f;          // the width of the next strokes
	bool draw = true;             // true: the mouse draws; false: it turns the camera
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

	mesh_drawable sheet;                  // the white sheet behind the strokes
	std::vector<stroke_structure> strokes;
	sketch_parameters parameters;
	bool drawing = false;                 // true while the left button draws the last stroke

	vec3 mouse_position_3d() const;
	void update_ribbon(stroke_structure& stroke);
	void undo();
	void clear();

	void mouse_move_event();
	void mouse_click_event();
	void keyboard_event();
	void idle_frame();
};
