# slide-demos

Interactive 3D in a slide: a C++ program (raw OpenGL 3.3, or the CGP library),
compiled to WebAssembly with [Emscripten](https://emscripten.org/), shown in a slide as an iframe,
and driven by the slide while you speak.

The repository holds a small bridge between the program and the slide, and (soon) a gallery of
examples. It goes with the recipes of [lhtml-site](https://scholartools.fr/lhtml/), but the demos are
plain web pages: any slide tool that can show an iframe can use them.

> Status: work in progress. The bridge is written and tested on a real talk; the examples are next.

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

In a slide of lhtml-site:

```
demo::../../_demo/index.html?model=spot&freq=2&controls=freq,model&compact=1
```

### Driving the demo from the slide

The slide talks to the demo with `postMessage`, which also works when the demo is hosted elsewhere:

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

The bridge was checked end to end on the demo of the talk *Dynamic Skinning* (JFIG 2026) ported onto it:
settings from the address, generated panel, sliders and buttons, `postMessage` from the parent page, a refused
value, a change of model, the arrow keys reaching the deck, the readiness promise, and a canvas that is drawn
and moves (headless Chromium, WebGL by SwiftShader).

## Roadmap

First examples, from basic to advanced, each with another kind of interface:

| Id | Example | Interface |
|---|---|---|
| A1 | Raw OpenGL 3.3: a turning cube | none; settings in the address |
| A2 | A1 with uniforms driven live | panel in the iframe |
| C1 | The CGP template, as is | mouse in the canvas |
| C3 | A waving surface | panel in the iframe, and buttons in the slide |
| C6 | Interactive marching cubes | ImGui |
| C7 | Select and drag points | direct manipulation |

## License

To be decided.
