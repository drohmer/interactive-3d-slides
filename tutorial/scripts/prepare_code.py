"""The code of the examples in the pages of the tutorial, and its downloads.

Usage (called by ../build.sh):
  python3 scripts/prepare_code.py fragments          before the build of the pages
  python3 scripts/prepare_code.py publish <site>     after it

fragments: src/_code/<file>.md for each file of the repository shown in a page, a fenced code block
           inside {% raw %}, so that a page writes {% include "_code/examples/a1-opengl-cube/main.cpp.md" %}
           and always shows the file as it is in the repository (src/_code/ is not committed).
publish:   <site>/code/: the files of the repository that git tracks (sources, web builds, posters,
           slides), served as they are: each one can be opened or downloaded, and the slides of the
           examples run live from there (examples/<id>/slides.html);
           <site>/code/<id>.zip for each example, with what it needs to build (the bridge, and the cmake
           module and glad for raw OpenGL, the build script for CGP), and <site>/code/interactive-3d-slides.zip
           for the whole repository.
"""
import os, shutil, subprocess, sys, zipfile

TUTORIAL = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.dirname(TUTORIAL)

LANGUAGES = {'.cpp': 'cpp', '.hpp': 'cpp', '.txt': 'cmake', '.cmake': 'cmake', '.html': 'html',
             '.js': 'javascript', '.json': 'json', '.py': 'python', '.sh': 'bash', '.conf': 'nginx', '.yaml': 'yaml', '.css': 'css', '.j2': 'html', '.glsl': 'glsl'}

# What each example needs, besides its own folder (repository-relative paths)
BRIDGE = ['bridge/slide_bridge.hpp', 'bridge/slide_bridge.cpp', 'bridge/slide-frame.html', 'bridge/slide-controls.js']
RAW_OPENGL = BRIDGE + ['examples/cmake/raw_opengl_demo.cmake', 'examples/third_party/glad']
CGP = BRIDGE + ['scripts/build_examples.py', 'tests/test_demo.py']
# The number of each example in the pages of the tutorial, its title, and its page
TITLES = {
    'a1-opengl-cube': (1, 'Rotating cube', '02_cube'),
    'a2-opengl-uniforms': (2, 'Uniforms driven live', '03_panel'),
    'c1-cgp-template': (3, 'CGP program', '04_cgp'),
    'c3-cgp-waving-surface': (4, 'Buttons in the slide', '05_buttons'),
    'c6-cgp-marching-cubes': (5, 'GUI with ImGui', '06_imgui'),
    'c7-cgp-drag-points': (6, 'Values sent back to the slide', '07_readout'),
    'c4-cgp-load-obj': (7, 'Models from files', '../03_examples/01_models'),
    'c8-cgp-sketch': (8, 'Sketch with the mouse', '../03_examples/02_sketch'),
    'c9-cgp-compare': (9, 'Two demos side by side', '../03_examples/03_compare'),
}
SITE = 'https://scholartools.fr/interactive-3d-slides/'
# Not in the zips: the configuration of the web server of the tutorial
NOT_IN_ZIPS = ('ops/',)
EXAMPLES = {
    'a1-opengl-cube': RAW_OPENGL,
    'a2-opengl-uniforms': RAW_OPENGL,
    'c1-cgp-template': CGP,
    'c3-cgp-waving-surface': CGP,
    'c6-cgp-marching-cubes': CGP,
    'c7-cgp-drag-points': CGP,
    'c4-cgp-load-obj': CGP,
    'c8-cgp-sketch': CGP,
    'c9-cgp-compare': CGP,
}


def tracked():
    """The files of the repository that git tracks or would track (not ignored: no build output, no
    local state), without the tutorial itself."""
    out = subprocess.run(['git', '-C', REPO, 'ls-files', '--cached', '--others', '--exclude-standard'],
                         check=True, capture_output=True, text=True).stdout
    return sorted(f for f in set(out.splitlines())
                  if not f.startswith('tutorial/') and os.path.isfile(os.path.join(REPO, f)))


def deck_files():
    """The sources of the example talks (tutorial/decks/), shown in the pages too."""
    out = subprocess.run(['git', '-C', REPO, 'ls-files', '--cached', '--others', '--exclude-standard', 'tutorial/decks'],
                         check=True, capture_output=True, text=True).stdout
    return sorted(f for f in set(out.splitlines()) if os.path.isfile(os.path.join(REPO, f)))


def fragments():
    target = os.path.join(TUTORIAL, 'src', '_code')
    shutil.rmtree(target, ignore_errors=True)
    count = 0
    for path in tracked() + deck_files():
        language = LANGUAGES.get(os.path.splitext(path)[1])
        if language is None or '/web/' in path or 'third_party/' in path:
            continue
        with open(os.path.join(REPO, path), encoding='utf-8') as f:
            code = f.read().rstrip('\n')
        if '{% endraw %}' in code or '~~~~~' in code:
            sys.exit(f'{path}: cannot be shown in a page (it holds {{% endraw %}} or ~~~~~)')
        fragment = os.path.join(target, path + '.md')
        os.makedirs(os.path.dirname(fragment), exist_ok=True)
        with open(fragment, 'w', encoding='utf-8') as f:
            f.write('{% raw %}\n~~~~' + language + '\n' + code + '\n~~~~\n{% endraw %}\n')
        count += 1
    print(f'fragments: {count} files in src/_code/')


def expand(paths, files):
    """The tracked files under each path (a file, or a folder)."""
    result = []
    for p in paths:
        result += [f for f in files if f == p or f.startswith(p.rstrip('/') + '/')]
    return result


def lhtml_demo(example, files):
    """The demo of the lhtml-site slide of an example (examples/<id>/lhtml-site/src/_demo/, not in the repository):
    its web build, its poster and slide-controls.js, so that the slide builds as it is from the zip."""
    if not os.path.isdir(os.path.join(REPO, 'examples', example, 'lhtml-site')):
        return []
    demo = 'examples/' + example + '/lhtml-site/src/_demo/'
    web = 'examples/' + example + '/web/'
    pairs = [(f, demo + f[len(web):]) for f in files if f.startswith(web)]
    pairs += [('examples/' + example + '/poster.png', demo + 'poster.png'), ('bridge/slide-controls.js', demo + 'slide-controls.js')]
    return pairs


def example_readme(example):
    """The README at the top of the zip of an example: what is inside, and the steps in order."""
    number, title, page = TITLES[example]
    url = SITE + os.path.normpath('01_tutorial/' + page) + '/index.html'
    slide = sorted(x for x in os.listdir(os.path.join(REPO, 'examples', example, 'lhtml-site', 'src')) if not x.startswith('_'))[0]
    cgp = example.startswith('c')
    build = (f"""```sh
source <emsdk>/emsdk_env.sh                                       # the Emscripten SDK, in each new terminal
git clone https://github.com/drohmer/cgp-2027.git ../cgp-2027     # the CGP project, once
CGP_DIR=../cgp-2027 python3 scripts/build_examples.py {example}
```

The script copies the files of the example over the CGP project, adds the bridge, builds for the web, and copies
the result into `examples/{example}/web/` and `examples/{example}/lhtml-site/src/_demo/`. Add `--native` to also
build the desktop version. Details: `examples/{example}/README.md`.""" if cgp else f"""```sh
cd examples/{example}
cmake -S . -B build && cmake --build build && ./build/project     # desktop (needs GLFW)
source <emsdk>/emsdk_env.sh                                       # the Emscripten SDK, in each new terminal
emcmake cmake -S . -B build_web && cmake --build build_web        # web
```

The web build is copied into `web/` and `lhtml-site/src/_demo/`: reload the slide to see your change.
Details: `examples/{example}/README.md`.""")
    emsdk = (r"<emsdk>\emsdk_env.ps1                                         # the Emscripten SDK (emsdk_env.bat in cmd)" "\n"
             r"winget install Ninja-build.Ninja                                # once: emcmake needs Ninja on Windows")
    windows = (f"""```powershell
python -m http.server 8080                                        # step 1 (or: py -m http.server 8080)
```

```powershell
{emsdk}
git clone https://github.com/drohmer/cgp-2027.git ..\\cgp-2027     # the CGP project, once
$env:CGP_DIR = "..\\cgp-2027"
python scripts\\build_examples.py {example}
```""" if cgp else f"""```powershell
python -m http.server 8080                                        # step 1 (or: py -m http.server 8080)
```

The desktop version needs GLFW, for instance from [vcpkg](https://vcpkg.io/) (`vcpkg install glfw3`):

```powershell
cd examples\\{example}
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>\\scripts\\buildsystems\\vcpkg.cmake
cmake --build build --config Release
build\\Release\\project.exe
```

The web version:

```powershell
{emsdk}
emcmake cmake -S . -B build_web
cmake --build build_web
```""")
    return f"""# Example {number}: {title}

From the tutorial *Create slides with interactive 3D*: {url}

## What is in this folder

| Path | What |
|---|---|
| `examples/{example}/` | the example: its code, `README.md`, and its slides |
| `examples/{example}/slides.html` | the slide in reveal.js |
| `examples/{example}/lhtml-site/` | the same slide in lhtml-site (`src/{slide}/index.html.j2`), with the demo in `src/_demo/` |
| `examples/{example}/web/` | the demo, already built for the web |
| `bridge/` | the code that connects the program and the slide (used by the build) |
| {"`scripts/build_examples.py`" if cgp else "`examples/cmake/`, `examples/third_party/glad/`"} | {"the build of the CGP examples" if cgp else "the build of the raw OpenGL examples, and the OpenGL loader for the desktop"} |

## 1. See the demo in its slide (nothing to install but Python)

```sh
python3 -m http.server 8080
```

in this folder, then open http://localhost:8080/examples/{example}/slides.html
(a page opened as a file cannot load the demo: a web server is needed). The slide loads reveal.js from the
internet.

## 2. The same slide in lhtml-site

With lhtml-site installed (https://scholartools.fr/lhtml/, `pipx install lhtml-site`):

```sh
cd examples/{example}/lhtml-site
lhtml-site --serve --watch
```

then open http://127.0.0.1:8000/ . `lhtml-site` alone builds it into `.site/html/`.

## 3. Change the program and rebuild it

{build}

## On Windows

The same steps, with the commands of PowerShell (not tested on Windows by the author):

{windows}
"""


def full_readme():
    """The README at the top of the zip of the whole repository: the README of the repository, without what
    the zip does not hold (the pages of the tutorial, the configuration of the server)."""
    with open(os.path.join(REPO, 'README.md'), encoding='utf-8') as f:
        text = f.read()
    lines = [l for l in text.splitlines() if not l.startswith(('| [tutorial/]', '| [ops/]'))]
    text = '\n'.join(lines) + '\n'
    text = text.replace('This repository holds the tutorial and its code:',
                        'This folder holds the code of the tutorial (the repository '
                        'https://github.com/drohmer/interactive-3d-slides without the pages of the tutorial):')
    return text.split('\n## Build the site of the tutorial')[0].rstrip() + '\n'


def publish(site):
    files = tracked()
    code = os.path.join(site, 'code')
    shutil.rmtree(code, ignore_errors=True)
    for path in files:
        os.makedirs(os.path.dirname(os.path.join(code, path)), exist_ok=True)
        shutil.copy2(os.path.join(REPO, path), os.path.join(code, path))

    def write_zip(name, root, paths, extra=(), readme=None):
        with zipfile.ZipFile(os.path.join(code, name), 'w', zipfile.ZIP_DEFLATED) as z:
            if readme is not None:
                z.writestr(root + '/README.md', readme)
            for path in sorted(set(paths)):
                if path.startswith(NOT_IN_ZIPS) or (readme is not None and path == 'README.md'):
                    continue
                z.write(os.path.join(REPO, path), os.path.join(root, path))
            for source, path in extra:
                z.write(os.path.join(REPO, source), os.path.join(root, path))
        return os.path.getsize(os.path.join(code, name)) / 1e6

    # an example: a folder named after it, with its README at the top, the example and what it needs
    for example, needs in EXAMPLES.items():
        size = write_zip(example + '.zip', example, expand(['examples/' + example] + needs + ['LICENSE'], files),
                         lhtml_demo(example, files), example_readme(example))
        print(f'zip: code/{example}.zip ({size:.1f} MB)')
    demos = [pair for example in EXAMPLES for pair in lhtml_demo(example, files)]
    size = write_zip('interactive-3d-slides.zip', 'interactive-3d-slides', files, demos, readme=full_readme())
    print(f'zip: code/interactive-3d-slides.zip ({size:.1f} MB), {len(files)} files in code/')
    # the fragments are for the pages only
    shutil.rmtree(os.path.join(site, '_code'), ignore_errors=True)
    shutil.rmtree(os.path.join(site, 'fr', '_code'), ignore_errors=True)


if __name__ == '__main__':
    if sys.argv[1:2] == ['fragments']:
        fragments()
    elif sys.argv[1:2] == ['publish'] and len(sys.argv) == 3:
        publish(os.path.abspath(sys.argv[2]))
    else:
        sys.exit(__doc__)
