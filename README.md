# Create slides with interactive 3D

**The tutorial: [scholartools.fr/interactive-3d-slides](https://scholartools.fr/interactive-3d-slides/)**
(in English, and [in French](https://scholartools.fr/interactive-3d-slides/fr/)).

How to put interactive 3D models, typically made with OpenGL, in your presentations: a C++ program
(raw OpenGL 3.3, or a library such as CGP) compiled to WebAssembly with [Emscripten](https://emscripten.org/),
shown in a slide as an iframe, and driven from the slide while you speak, in reveal.js or lhtml-site. The tutorial
goes through complete examples, from a turning cube in raw OpenGL to marching cubes with ImGui: each one with
all its code, to download, a slide in reveal.js (`slides.html`) and in lhtml-site (`lhtml-site/`), and a live demo.

This repository holds the tutorial and its code:

| Folder | What |
|---|---|
| [tutorial/](tutorial/) | The pages of the tutorial (English `src/`, French `src-fr/`), two example talks (lhtml-site, reveal.js), `build.sh` that builds the site (the code of the pages is included from the files of the repository), and `deploy.sh` |
| [bridge/](bridge/) | The bridge between a program and a slide: `slide_bridge.hpp/.cpp`, `slide-frame.html`, `slide-controls.js` |
| [examples/](examples/) | Six example programs, from a cube in raw OpenGL 3.3 to marching cubes with ImGui, with their web builds, posters and tests |
| [tests/](tests/), [scripts/](scripts/) | The tests of the bridge and of the examples, and the build of the examples |
| [ops/](ops/) | The configuration of the web server of the tutorial (nginx) |

The *bridge* is the small piece of code that connects a program to a slide: the settings given in the address
of the frame, the controls of the panel, the buttons written in the slide, and the keyboard kept by the slides.

The rest of this README is the reference of the bridge; the tutorial explains how to use it.

## What the bridge does

A demo in a slide needs the same plumbing every time:

- **Settings in the address of the iframe.** `index.html?freq=2&model=bunny` starts the program with
  `--freq 2 --model bunny`: one build serves several slides, each with its own preset.
- **Live controls.** Sliders and buttons, in the iframe or in the slide itself, that change the
  program while it runs.
- **The deck keeps the keyboard.** Once you click in the demo, the arrow keys still go to the next slide.
- **Ready for the PDF.** The PDF export of lhtml-site waits for the first frames, and the canvas can be captured.

You declare each parameter once in C++, with its range or its choices; the page builds its controls from them.

## The bridge, C++ side: `bridge/slide_bridge.hpp`

Two files, `slide_bridge.hpp` and `slide_bridge.cpp`, with no dependency but the standard library.
Add them to your sources.

```cpp
#include "slide_bridge.hpp"

int main(int argc, char* argv[])
{
    slide::parse(argc, argv);                        // keep the --key value pairs
    std::string size = slide::arg("size", "800x600"); // read an argument before anything exists

    // ... create the window, initialize the scene ...

    slide::param("freq", scene.freq, 0.3f, 5.0f, 0.1f)          // a float in [0.3, 5], step 0.1
        .with_label("frequency <i>f</i> (Hz)", "<i>f</i>");      // label, and short label (compact panel)
    slide::param("model", scene.model, {{"spot", "Cow"}, {"bunny", "Bunny"}})
        .on_change([&] { scene.load(scene.model); });             // called when the slide changes it
    slide::param("play", scene.play);                             // a bool
    slide::action("reset", [&] { scene.reset(); });               // a button without a value

    // ... main loop ...
}
```

- A parameter declared after `parse()` takes the value given at start, without calling `on_change`
  (your program reads its variables when it initializes).
- When the stored value has other units than the one shown, declare it with a getter and a setter:

  ```cpp
  slide::param("freq", [&] { return omega / (2 * pi); }, [&](float f) { omega = 2 * pi * f; }, 0.3f, 5.0f);
  ```

- `.with_class("time")` gives a CSS class to the row of the control, to color a group of controls.
- A value computed by the program, shown by the page and the slide but not set by them, is a readout;
  when the program changes values by itself (a drag with the mouse), `slide::notify()` tells the page:

  ```cpp
  slide::readout("length", [&] { return curve_length; });
  slide::notify();   // after a change made by the program; the page shows it at the next frame
  ```
- The web build exports `slide_set(key, value)` and `slide_describe()` (JSON of the parameters).
  Add them to the link options:

  ```
  -sEXPORTED_FUNCTIONS=_main,_slide_set,_slide_describe -sEXPORTED_RUNTIME_METHODS=ccall
  ```

- The native build compiles the same code: the arguments then come from the command line.

## The bridge, page side: `bridge/slide-frame.html`

Copy it next to `project.js`, `project.wasm` and `project.data` as `index.html`, and adapt the
`CONFIG` block at its top (name of the script, default arguments).

| In the address | Effect |
|---|---|
| `?freq=2&model=bunny` | arguments of the program: `--freq 2 --model bunny` |
| `&controls=freq,model,mode:lbs:drag` | controls of the panel, in this order; `mode:lbs:drag` keeps only two buttons of `mode` |
| `&compact=1` | short labels, small panel |
| `&panel=tl`, `tr`, `bl` or `br` | corner of the panel (default `tr`) |
| `&build=name` | loads `name/project.js`: several builds in one folder |

In a slide of lhtml-site, with the web build of the demo in `src/_demo/` (as in `examples/<id>/lhtml-site/`):

```
demo::../_demo/index.html?model=spot&freq=2&controls=freq,model&compact=1
```

### Driving the demo from the slide

With [bridge/slide-controls.js](bridge/slide-controls.js), buttons in the text of the slide drive the demo
with HTML attributes only:

```html
<script src="slide-controls.js"></script>
<button data-demo-set="shape=waves">Waves</button>        <!-- sets a parameter -->
<button data-demo-set="ampl=0.25&k=14">Strong</button>    <!-- several at once -->
<button data-demo-set="drop">Drop</button>                <!-- an action -->
A = <span data-demo-value="ampl" data-digits="2"></span>  <!-- a value, live -->
```

A button gets the class `on` while its settings are the current values of the demo. The demo is the first
`iframe.demo` of the page (or the first iframe); `data-demo="#id"` names another.

Underneath, the slide talks to the demo with `postMessage`, which also works when the demo is hosted elsewhere:

```js
const demo = document.querySelector('iframe').contentWindow;
demo.postMessage({slide: {set: {freq: 3, model: 'bunny'}}}, '*');
demo.postMessage({slide: {describe: true}}, '*');
```

The demo answers its start and each change:

```js
window.addEventListener('message', (e) => {
  const m = e.data && e.data.slide;    // {ready: true, params: [...]}
  if (m && m.changed) console.log(m.changed.key, m.changed.accepted, m.params);
});
```

Messages sent before the demo is ready are kept and applied once it starts.

## Tests

```sh
make test
```

runs the unit tests of the C++ bridge (with the address and undefined-behavior sanitizers).

```sh
source <emsdk>/emsdk_env.sh
git clone https://github.com/drohmer/cgp-2027.git ../cgp-2027     # the CGP project, once
CGP_DIR=../cgp-2027 make examples
```

builds each example for the web (`--native`: also for the desktop, and runs it), copies the build into
`examples/<id>/web/` and `examples/<id>/lhtml-site/src/_demo/`, tests it in headless Chromium
and captures `examples/<id>/poster.png` (the image for the PDF). The checks of an example are listed in its
`demo.json` (see [tests/test_demo.py](tests/test_demo.py)); every example is also checked for a canvas that
is drawn and fills the frame, the arrow keys reaching the slide, and no JavaScript error. Playwright comes
from the Python that runs the script, or from `LHTML_PYTHON`; without it, the test and the poster are skipped
(`--no-test` skips them too). Log: `build/build_examples.log`.

The CGP examples hold only the files that differ from the template project of CGP: the script stages them
over a copy of that project (`CGP_DIR`, a clone of [cgp-2027](https://github.com/drohmer/cgp-2027), with its
`CMakeLists.txt`, `cgp/`, `src/`, `shaders/`), adds the bridge, and builds with the `CMakeLists.txt` of CGP.

The bridge was checked end to end on the demo of the talk *Dynamic Skinning* (JFIG 2026) ported onto it:
settings from the address, generated panel, sliders and buttons, `postMessage` from the parent page, a refused
value, a change of model, the arrow keys reaching the deck, the readiness promise, and a canvas that is drawn
and moves (headless Chromium, WebGL by SwiftShader).

## Examples

First examples, from basic to advanced, each with another kind of interface:

| Id | Example | Interface | Status |
|---|---|---|---|
| A1 | [Raw OpenGL 3.3: a turning cube](examples/a1-opengl-cube/) | none; settings in the address | done |
| A2 | [A1 with uniforms driven live](examples/a2-opengl-uniforms/) | panel in the iframe, or the slide | done |
| C1 | [The CGP template, as is](examples/c1-cgp-template/) | mouse in the canvas, ImGui | done |
| C3 | [A waving surface](examples/c3-cgp-waving-surface/) | panel in the iframe, and buttons in the slide | done |
| C6 | [Interactive marching cubes](examples/c6-cgp-marching-cubes/) | ImGui, enlarged for the projector | done |
| C7 | [Select and drag points](examples/c7-cgp-drag-points/) | direct manipulation; a value sent to the slide | done |

## License

MIT, see [LICENSE](LICENSE). The third-party code keeps its own terms: glad and `khrplatform.hpp` in
[examples/third_party/glad](examples/third_party/glad/README.md); the CGP examples build with the
[CGP library](https://github.com/drohmer/cgp-2027) (MIT).

## Build the site of the tutorial

```sh
tutorial/build.sh
```

builds the pages (English and French), copies the live demos (`examples/*/web`) and builds the two example
talks into `tutorial/.site/`, to serve at `/interactive-3d-slides/`. Needs lhtml-site.
