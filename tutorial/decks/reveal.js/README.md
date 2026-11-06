# An example talk with interactive 3D, made with reveal.js

From the tutorial *Create slides with interactive 3D*:
https://scholartools.fr/interactive-3d-slides/01_tutorial/08_presentations/index.html

Twelve slides, with titles, text, and interactive 3D models (C++ OpenGL programs compiled to WebAssembly)
in some of them.

| Path | What |
|---|---|
| `index.html` | the whole presentation, a `<section>` per slide |
| `demos/a1/` ... `demos/c7/` | the demos, already built for the web (`index.html`, `project.js`, `project.wasm`, ...) |
| `demos/slide-controls.js` | the buttons written in the slides, which drive the demos |

## Show it

In this folder:

```sh
python3 -m http.server 8080
```

then open http://localhost:8080/ (a page opened as a file cannot load the demos: a web server is needed).
Go from slide to slide with the arrow keys; click in a demo to turn it, the arrow keys still go to the next
slide.

reveal.js is loaded from the internet (cdn.jsdelivr.net). For a talk without network, download its `dist/`
folder (https://github.com/hakimel/reveal.js) next to `index.html`, and change the three lines that load it.

## Your own talk

Edit `index.html`: a slide is a `<section>`; a demo is an `<iframe class="demo" data-src="demos/...">`. To put
your own program, build it for the web as in the tutorial, and copy its build into `demos/`.
