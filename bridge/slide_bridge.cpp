#include "slide_bridge.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace slide {

namespace {

struct registry_structure {
	std::vector<std::unique_ptr<parameter>> parameters;   // in the order of declaration
	std::map<std::string, std::string> arguments;         // --key value of the command line
	std::vector<std::function<void(std::string const&)>> any_change;
};

registry_structure& registry()
{
	static registry_structure r;
	return r;
}

parameter* find(std::string const& key)
{
	for (auto& p : registry().parameters)
		if (p->key == key)
			return p.get();
	return nullptr;
}

std::string number_text(float value)
{
	char buffer[32];
	std::snprintf(buffer, sizeof(buffer), "%.6g", double(value));
	return buffer;
}

bool read_number(std::string const& text, float& value)
{
	if (text.empty())
		return false;
	char* end = nullptr;
	errno = 0;
	float const v = std::strtof(text.c_str(), &end);
	if (errno != 0 || end == text.c_str() || *end != '\0')
		return false;
	value = v;
	return true;
}

bool read_toggle(std::string const& text, bool& value)
{
	if (text == "1" || text == "true" || text == "on" || text == "yes") { value = true; return true; }
	if (text == "0" || text == "false" || text == "off" || text == "no") { value = false; return true; }
	return false;
}

// Declares a parameter (a second declaration of the same key replaces the first)
// and gives it the value of the command line, if any, without the callbacks
parameter& declare(std::unique_ptr<parameter> p)
{
	auto& list = registry().parameters;
	parameter* result = p.get();
	bool replaced = false;
	for (auto& existing : list)
		if (existing->key == p->key) { existing = std::move(p); replaced = true; break; }
	if (!replaced)
		list.push_back(std::move(p));

	if (result->label.empty())
		result->label = result->key;
	auto const it = registry().arguments.find(result->key);
	if (it != registry().arguments.end() && result->type != kind::action) {
		if (!result->set(it->second))
			std::fprintf(stderr, "slide_bridge: --%s %s: value not accepted\n", result->key.c_str(), it->second.c_str());
	}
	return *result;
}

std::unique_ptr<parameter> make(std::string const& key, kind type)
{
	auto p = std::make_unique<parameter>();
	p->key = key;
	p->type = type;
	return p;
}

std::string json_string(std::string const& text)
{
	std::string out = "\"";
	for (unsigned char c : text) {
		switch (c) {
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (c < 0x20) {
				char buffer[8];
				std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
				out += buffer;
			}
			else
				out += char(c);
		}
	}
	return out + "\"";
}

char const* type_name(kind type)
{
	switch (type) {
	case kind::number: return "number";
	case kind::integer: return "integer";
	case kind::toggle: return "toggle";
	case kind::choice: return "choice";
	case kind::action: return "action";
	}
	return "number";
}

}  // namespace


parameter& parameter::with_label(std::string text, std::string short_text)
{
	label = std::move(text);
	short_label = std::move(short_text);
	return *this;
}

parameter& parameter::with_class(std::string css_class)
{
	css = std::move(css_class);
	return *this;
}

parameter& parameter::on_change(std::function<void()> callback)
{
	callbacks.push_back(std::move(callback));
	return *this;
}


void parse(int argc, char* argv[])
{
	auto& arguments = registry().arguments;
	for (int k = 1; k < argc; ++k) {
		std::string const word = argv[k];
		if (word.rfind("--", 0) != 0 || word.size() <= 2)
			continue;
		std::string const key = word.substr(2);
		bool const has_value = k + 1 < argc && std::string(argv[k + 1]).rfind("--", 0) != 0;
		std::string const value = has_value ? argv[k + 1] : "1";
		if (has_value)
			++k;
		arguments[key] = value;
		if (parameter* p = find(key))
			if (p->type != kind::action && !p->set(value))
				std::fprintf(stderr, "slide_bridge: --%s %s: value not accepted\n", key.c_str(), value.c_str());
	}
}

bool has_arg(std::string const& key)
{
	return registry().arguments.count(key) > 0;
}

std::string arg(std::string const& key, std::string const& fallback)
{
	auto const it = registry().arguments.find(key);
	return it == registry().arguments.end() ? fallback : it->second;
}


parameter& param(std::string const& key, float& value, float min, float max, float step)
{
	float* v = &value;
	return param(key, std::function<float()>([v] { return *v; }), std::function<void(float)>([v](float x) { *v = x; }), min, max, step);
}

parameter& param(std::string const& key, std::function<float()> get, std::function<void(float)> set_value,
                 float min, float max, float step)
{
	auto p = make(key, kind::number);
	p->min = min; p->max = max; p->step = step;
	p->get = [get] { return number_text(get()); };
	p->set = [set_value](std::string const& text) {
		float x;
		if (!read_number(text, x)) return false;
		set_value(x);
		return true;
	};
	return declare(std::move(p));
}

parameter& param(std::string const& key, int& value, int min, int max)
{
	int* v = &value;
	auto p = make(key, kind::integer);
	p->min = float(min); p->max = float(max); p->step = 1;
	p->get = [v] { return std::to_string(*v); };
	p->set = [v](std::string const& text) {
		float x;
		if (!read_number(text, x)) return false;
		*v = int(x < 0 ? x - 0.5f : x + 0.5f);
		return true;
	};
	return declare(std::move(p));
}

parameter& param(std::string const& key, bool& value)
{
	bool* v = &value;
	return param(key, std::function<bool()>([v] { return *v; }), std::function<void(bool)>([v](bool x) { *v = x; }));
}

parameter& param(std::string const& key, std::function<bool()> get, std::function<void(bool)> set_value)
{
	auto p = make(key, kind::toggle);
	p->get = [get] { return std::string(get() ? "1" : "0"); };
	p->set = [set_value](std::string const& text) {
		bool x;
		if (!read_toggle(text, x)) return false;
		set_value(x);
		return true;
	};
	return declare(std::move(p));
}

parameter& param(std::string const& key, std::string& value, std::vector<choice> choices)
{
	std::string* v = &value;
	auto p = make(key, kind::choice);
	p->choices = std::move(choices);
	parameter* raw = p.get();
	p->get = [v] { return *v; };
	p->set = [v, raw](std::string const& text) {
		if (!raw->choices.empty()) {
			bool known = false;
			for (auto const& c : raw->choices)
				known = known || c.value == text;
			if (!known) return false;
		}
		*v = text;
		return true;
	};
	return declare(std::move(p));
}

parameter& action(std::string const& key, std::function<void()> callback)
{
	auto p = make(key, kind::action);
	p->get = [] { return std::string(); };
	p->set = [](std::string const&) { return true; };
	p->callbacks.push_back(std::move(callback));
	return declare(std::move(p));
}


bool set(std::string const& key, std::string const& value)
{
	parameter* p = find(key);
	if (p == nullptr) {
		std::fprintf(stderr, "slide_bridge: unknown parameter %s\n", key.c_str());
		return false;
	}
	if (!p->set(value)) {
		std::fprintf(stderr, "slide_bridge: %s = %s: value not accepted\n", key.c_str(), value.c_str());
		return false;
	}
	for (auto& callback : p->callbacks)
		callback();
	for (auto& callback : registry().any_change)
		callback(key);
	return true;
}

std::string describe()
{
	std::string out = "{\"version\":1,\"params\":[";
	bool first = true;
	for (auto const& p : registry().parameters) {
		if (!first) out += ",";
		first = false;
		out += "{\"key\":" + json_string(p->key);
		out += ",\"type\":\"" + std::string(type_name(p->type)) + "\"";
		out += ",\"label\":" + json_string(p->label);
		if (!p->short_label.empty()) out += ",\"short\":" + json_string(p->short_label);
		if (!p->css.empty()) out += ",\"class\":" + json_string(p->css);
		std::string const value = p->get();
		switch (p->type) {
		case kind::number:
		case kind::integer:
			out += ",\"value\":" + value;
			out += ",\"min\":" + number_text(p->min) + ",\"max\":" + number_text(p->max);
			out += ",\"step\":" + number_text(p->step);
			break;
		case kind::toggle:
			out += std::string(",\"value\":") + (value == "1" ? "true" : "false");
			break;
		case kind::choice:
			out += ",\"value\":" + json_string(value) + ",\"choices\":[";
			for (size_t k = 0; k < p->choices.size(); ++k) {
				if (k > 0) out += ",";
				out += "[" + json_string(p->choices[k].value) + "," + json_string(p->choices[k].label) + "]";
			}
			out += "]";
			break;
		case kind::action:
			break;
		}
		out += "}";
	}
	return out + "]}";
}

void on_any_change(std::function<void(std::string const&)> callback)
{
	registry().any_change.push_back(std::move(callback));
}

void reset()
{
	registry() = registry_structure();
}

}  // namespace slide


#ifdef __EMSCRIPTEN__
// Called by slide-frame.html: Module.ccall('slide_set', 'number', ['string', 'string'], [key, value])
extern "C" EMSCRIPTEN_KEEPALIVE int slide_set(char const* key, char const* value)
{
	return slide::set(key, value) ? 1 : 0;
}

// Called by slide-frame.html: JSON.parse(Module.ccall('slide_describe', 'string', [], []))
extern "C" EMSCRIPTEN_KEEPALIVE char const* slide_describe()
{
	static std::string text;
	text = slide::describe();
	return text.c_str();
}
#endif
