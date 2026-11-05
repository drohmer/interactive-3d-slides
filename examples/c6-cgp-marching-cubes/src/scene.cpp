// Origin: the CGP example of github.com/drohmer/epita_ani3d_lab_code-2027
// (examples/02_marching_cubes/02_marching_cube_dynamic), as in the documentation of CGP.
// slide-demos: the parameters declared to the bridge (end of initialize()), the size of the
// ImGui window set from the slide, and two controls hidden on the web (gui_helper.cpp).
#include "scene.hpp"
#include "slide_bridge.hpp"


using namespace cgp;




void scene_structure::initialize()
{
	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.set_rotation_axis_y();
	// slide-demos: farther than the original { 3.0f, 2.0f, 2.0f }, so that the whole domain fits a wide slide
	camera_control.look_at({ 6.0f, 4.5f, 3.5f }, {0,0,0}, {0,0,1});
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());


	// Initialization for the Implicit Surface
	// ***************************************** //

	// slide-demos: the same variables as the ImGui window, declared to the bridge. The slide
	// changes them (address of the iframe, postMessage, slide-controls.js); the ImGui window
	// shows the new values by itself at the next frame, as it reads the variables.
	// Their values given at start (?isovalue=0.3) are applied here, before the first field.
	auto refield = [this] {
		implicit_surface.set_domain(gui.domain.samples, gui.domain.length);
		implicit_surface.update_field(field_function, gui.isovalue);
	};
	slide::param("isovalue", gui.isovalue, 0.0f, 1.0f, 0.01f)
		.on_change([this] { implicit_surface.update_marching_cube(gui.isovalue); });
	slide::param("power_c", field_function.sc, 0.0f, 2.0f, 0.05f).with_label("power C").on_change(refield);
	slide::param("noise", field_function.noise_magnitude, 0.0f, 1.0f, 0.05f).on_change(refield);
	slide::param("samples", gui.domain.samples, 8, 100).on_change(refield);
	slide::param("domain", gui.display.domain).with_label("box");
	// size of the text of ImGui: larger in a slide (?gui_scale=1.6), read by the template at each frame
	slide::param("gui_scale", project::gui_scale, 0.5f, 3.0f, 0.1f).with_label("ImGui size");

	refield();
}



void scene_structure::display_frame()
{
	// Update the camera view matrix from the camera controller
	environment.camera_view = camera_control.camera_model.matrix_view();

	// Set the light to the current position of the camera
	environment.light = camera_control.camera_model.position();
	
	// The standard frame
	if (gui.display.frame)
		draw(global_frame, environment);

	if (gui.display.surface)    // Display the implicit surface
		draw(implicit_surface.drawable_param.shape, environment);

	if (gui.display.wireframe) // Display the wireframe of the implicit surface
		draw_wireframe(implicit_surface.drawable_param.shape, environment, { 0,0,0 });

	if (gui.display.domain)    // Display the boundary of the domain
		draw(implicit_surface.drawable_param.domain_box, environment);

}

void scene_structure::display_gui()
{
	// Handle the gui values and the updates using the helper methods (*)
	implicit_surface.gui_update(gui, field_function);
}

void scene_structure::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move();
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click();
}
void scene_structure::keyboard_event()
{
	camera_control.action_keyboard();
}
void scene_structure::idle_frame()
{
	camera_control.idle_frame();
}

