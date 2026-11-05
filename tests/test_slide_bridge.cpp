// Unit tests of the bridge, native build:
//   c++ -std=c++17 -Wall -Wextra -I bridge bridge/slide_bridge.cpp tests/test_slide_bridge.cpp -o build/test_slide_bridge
//   ./build/test_slide_bridge
#include "slide_bridge.hpp"

#include <cmath>
#include <cstdio>
#include <string>

static int failures = 0;
#define CHECK(condition) do { if (!(condition)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition); ++failures; } } while (0)

static bool contains(std::string const& text, std::string const& part) { return text.find(part) != std::string::npos; }

static void test_arguments_before_and_after_declaration()
{
	slide::reset();
	char const* argv[] = {"demo", "--freq", "2.5", "--model", "bunny", "--size", "800x600", "--skeleton", "--zoom", "-1.5"};
	slide::parse(10, const_cast<char**>(argv));

	// read before anything is declared
	CHECK(slide::arg("size") == "800x600");
	CHECK(slide::arg("skeleton") == "1");        // lone flag
	CHECK(slide::arg("missing", "x") == "x");
	CHECK(!slide::has_arg("missing"));

	// declared after parse: takes the value given at start, without the callback
	float freq = 1.0f;
	int calls = 0;
	slide::param("freq", freq, 0.3f, 5.0f).on_change([&] { ++calls; });
	CHECK(std::fabs(freq - 2.5f) < 1e-6f);
	CHECK(calls == 0);

	std::string model = "spot";
	slide::param("model", model, {"spot", "bunny"});
	CHECK(model == "bunny");

	float zoom = 1.0f;
	slide::param("zoom", zoom, -10.0f, 10.0f);
	CHECK(std::fabs(zoom + 1.5f) < 1e-6f);      // negative value, not taken for a flag

	// declared before parse: parse gives it its value
	slide::reset();
	bool play = true;
	slide::param("play", play);
	char const* argv2[] = {"demo", "--play", "0"};
	slide::parse(3, const_cast<char**>(argv2));
	CHECK(play == false);
}

static void test_set_from_the_slide()
{
	slide::reset();
	float ampl = 0.3f;
	int calls = 0;
	std::string last_key;
	slide::param("ampl", ampl, 0.0f, 1.0f, 0.05f).on_change([&] { ++calls; });
	slide::on_any_change([&](std::string const& key) { last_key = key; });

	CHECK(slide::set("ampl", "0.7"));
	CHECK(std::fabs(ampl - 0.7f) < 1e-6f);
	CHECK(calls == 1);
	CHECK(last_key == "ampl");

	CHECK(!slide::set("ampl", "abc"));            // refused, unchanged
	CHECK(!slide::set("ampl", "0.5x"));
	CHECK(std::fabs(ampl - 0.7f) < 1e-6f);
	CHECK(calls == 1);
	CHECK(!slide::set("unknown", "1"));

	std::string mode = "lbs";
	slide::param("mode", mode, {"lbs", "drag", "ds"});
	CHECK(slide::set("mode", "ds"));
	CHECK(mode == "ds");
	CHECK(!slide::set("mode", "other"));
	CHECK(mode == "ds");

	bool skeleton = false;
	slide::param("skeleton", skeleton);
	CHECK(slide::set("skeleton", "true") && skeleton);
	CHECK(slide::set("skeleton", "0") && !skeleton);
	CHECK(!slide::set("skeleton", "maybe"));

	int count = 3;
	slide::param("count", count, 1, 10);
	CHECK(slide::set("count", "7") && count == 7);

	int resets = 0;
	slide::action("reset", [&] { ++resets; });
	CHECK(slide::set("reset", ""));
	CHECK(resets == 1);
}

static void test_getter_setter_with_units()
{
	slide::reset();
	float omega = 2.0f * 3.14159265f * 1.5f;   // stored: 2 pi f
	slide::param("freq", [&] { return omega / (2.0f * 3.14159265f); }, [&](float f) { omega = 2.0f * 3.14159265f * f; }, 0.3f, 5.0f, 0.1f);
	CHECK(contains(slide::describe(), "\"value\":1.5"));
	CHECK(slide::set("freq", "2"));
	CHECK(std::fabs(omega - 4.0f * 3.14159265f) < 1e-4f);
}

static void test_describe()
{
	slide::reset();
	float freq = 1.5f;
	std::string model = "spot";
	bool play = true;
	slide::param("freq", freq, 0.3f, 5.0f, 0.1f).with_label("frequency <i>f</i> (Hz)", "<i>f</i>").with_class("time");
	slide::param("model", model, {{"spot", "Cow"}, {"bunny", "Bunny \"B\""}});
	slide::param("play", play);
	slide::action("reset", [] {});
	std::string const json = slide::describe();
	CHECK(contains(json, "{\"version\":1,\"params\":["));
	CHECK(contains(json, "{\"key\":\"freq\",\"type\":\"number\",\"label\":\"frequency <i>f</i> (Hz)\",\"short\":\"<i>f</i>\",\"class\":\"time\",\"value\":1.5,\"min\":0.3,\"max\":5,\"step\":0.1}"));
	CHECK(contains(json, "\"choices\":[[\"spot\",\"Cow\"],[\"bunny\",\"Bunny \\\"B\\\"\"]]"));
	CHECK(contains(json, "{\"key\":\"play\",\"type\":\"toggle\",\"label\":\"play\",\"value\":true}"));
	CHECK(contains(json, "{\"key\":\"reset\",\"type\":\"action\",\"label\":\"reset\"}"));

	// two plain strings are two choices (not one std::string built from two pointers)
	std::string shape = "cube";
	slide::param("shape", shape, {"cube", "sphere"});
	CHECK(contains(slide::describe(), "\"choices\":[[\"cube\",\"cube\"],[\"sphere\",\"sphere\"]]"));
	std::string one = "a";
	slide::param("one", one, {{"a", "Label A"}});
	CHECK(contains(slide::describe(), "\"choices\":[[\"a\",\"Label A\"]]"));

	// a second declaration of a key replaces the first
	slide::param("freq", freq, 0.0f, 10.0f);
	std::string const again = slide::describe();
	CHECK(contains(again, "\"max\":10"));
	CHECK(again.find("\"key\":\"freq\"") == again.rfind("\"key\":\"freq\""));
}

static void test_readout()
{
	slide::reset();
	float length = 2.5f;
	slide::readout("length", [&] { return length; }).with_label("length");
	CHECK(contains(slide::describe(), "{\"key\":\"length\",\"type\":\"readout\",\"label\":\"length\",\"value\":2.5}"));
	length = 3.25f;
	CHECK(contains(slide::describe(), "\"value\":3.25"));
	CHECK(!slide::set("length", "1"));            // read only
	CHECK(std::fabs(length - 3.25f) < 1e-6f);
	slide::notify();                              // no effect in a native build
}

int main()
{
	test_readout();
	test_arguments_before_and_after_declaration();
	test_set_from_the_slide();
	test_getter_setter_with_units();
	test_describe();
	if (failures == 0)
		std::printf("slide_bridge: all tests passed\n");
	return failures == 0 ? 0 : 1;
}
