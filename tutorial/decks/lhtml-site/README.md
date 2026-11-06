# An example talk with interactive 3D, made with lhtml-site

From the tutorial *Create slides with interactive 3D*:
https://scholartools.fr/interactive-3d-slides/01_tutorial/08_presentations/index.html

Twelve slides, with titles, text, and interactive 3D models (C++ OpenGL programs compiled to WebAssembly)
in some of them.

| Path | What |
|---|---|
| `configure.yaml` | the build: the slides in the browser (`html`), and a PDF |
| `src/01_title/` ... `src/12_conclusion/` | one folder per slide, in the order of their names: `index.html.j2` |
| `src/_style/talk.css` | the buttons of the slides, and the image of a model in the PDF |
| `src/_demo/` | the models, already built for the web, with their poster, and `slide-controls.js` |

A folder whose name starts with an underscore is not a slide: it is copied as it is.

## Build and show it

lhtml-site is installed with `pipx install lhtml-site` (see https://scholartools.fr/lhtml/). In this folder:

```sh
lhtml-site --serve --watch
```

then open http://127.0.0.1:8000/ . `lhtml-site` alone builds the slides into `.site/html/`, and the PDF
`.site/slides.pdf` when the PDF export is available (`lhtml-site doctor` tells).

In a slide, `demo::../_demo/c3/index.html?...(.live)` shows the model on screen, and
`img::../_demo/c3/poster.png(.still)` its image in the PDF (the classes `.live` and `.still` are in
`src/_style/talk.css`).
