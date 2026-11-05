#pragma once

// slide_bridge: the parameters of a demo, declared once, set from the slide.
//
// A demo shown in a slide (an iframe) receives its settings in two ways:
//   - at start, from the address of the iframe: slide-frame.html turns
//     index.html?freq=2&model=bunny into the arguments --freq 2 --model bunny;
//   - while it runs, from the controls of the page or of the slide, through
//     the exported function slide_set(key, value).
// Each parameter is declared once, with its range or its choices: the page
// builds its sliders and buttons from slide_describe() (JSON).
//
//   slide::parse(argc, argv);                          // keep the --key value pairs
//   slide::param("freq", scene.freq, 0.3f, 5.0f);      // a float in [0.3, 5]
//   slide::param("model", scene.model, {"spot", "bunny"})
//       .on_change([&] { scene.load(scene.model); });  // called when the slide changes it
//
// A parameter declared after parse() takes the value given at start (without
// calling on_change: the program reads its variables when it initializes).
// slide::arg() reads an argument before anything is declared (a window size).
//
// No dependency but the standard library. Native builds compile it too: the
// same arguments then come from the command line.

#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace slide {

enum class kind { number, integer, toggle, choice, action, readout };

// One choice of a parameter: a value, and the label of its button (the value by default).
//   {"spot", "bunny"}                       two choices, labelled by their values
//   {{"spot", "Cow"}, {"bunny", "Bunny"}}   two choices with their labels
struct choice {
	std::string value, label;
	choice(char const* v) : value(v), label(v) {}
	choice(std::string v) : value(v), label(std::move(v)) {}
	choice(std::string v, std::string l) : value(std::move(v)), label(std::move(l)) {}
};

struct parameter {
	std::string key;
	kind type = kind::number;
	std::string label;        // shown in the panel (may hold HTML, such as <i>f</i>)
	std::string short_label;  // shown in the compact panel (compact=1)
	std::string css;          // class of the row in the panel (to color a group of controls)
	float min = 0, max = 1, step = 0;
	std::vector<choice> choices;

	std::function<std::string()> get;               // current value, as text
	std::function<bool(std::string const&)> set;    // false: value refused
	std::vector<std::function<void()>> callbacks;   // after a change from the slide

	// Chained settings: slide::param(...).label("frequency (Hz)").on_change(...)
	parameter& with_label(std::string text, std::string short_text = "");
	parameter& with_class(std::string css_class);
	parameter& on_change(std::function<void()> callback);
};

// Keep the "--key value" pairs of the command line (a lone "--key" counts as "1"),
// and give their values to the parameters already declared.
void parse(int argc, char* argv[]);
// Value of an argument of the command line, declared or not
bool has_arg(std::string const& key);
std::string arg(std::string const& key, std::string const& fallback = "");

// Declarations, bound to a variable of the program
parameter& param(std::string const& key, float& value, float min, float max, float step = 0);
parameter& param(std::string const& key, int& value, int min, int max);
parameter& param(std::string const& key, bool& value);
parameter& param(std::string const& key, std::string& value, std::vector<choice> choices);
// Declarations through a getter and a setter, when the variable has other units
// (a frequency in Hz shown, 2 pi f stored) or is not a plain variable
parameter& param(std::string const& key, std::function<float()> get, std::function<void(float)> set,
                 float min, float max, float step = 0);
parameter& param(std::string const& key, std::function<bool()> get, std::function<void(bool)> set);
// A button without a value (reset, next step...)
parameter& action(std::string const& key, std::function<void()> callback);
// A value computed by the program, shown by the page or the slide, not set by them
// (the length of a curve, the number of vertices...)
parameter& readout(std::string const& key, std::function<float()> get);

// Tell the page that the program changed values by itself (a drag with the mouse, a
// readout that changed): the page and the slide show them again. Cheap; at most once a
// frame is useful. Does nothing in a native build.
void notify();

// Change a parameter as the slide does: converts the text, then calls its
// callbacks. Returns false for an unknown key or a value that cannot be read.
bool set(std::string const& key, std::string const& value);
// The declared parameters, their current values, ranges and choices, as JSON
std::string describe();

// Called after every change from the slide, with its key (a redraw, a log...)
void on_any_change(std::function<void(std::string const&)> callback);

// Forget everything (tests)
void reset();

}  // namespace slide
