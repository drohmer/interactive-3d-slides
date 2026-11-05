"""Build the examples for the web, test them in a browser, and capture their posters.

Usage (the Emscripten SDK active: source <emsdk>/emsdk_env.sh):
  python3 scripts/build_examples.py [--native] [id ...]      all the examples by default
  CGP_DIR=<cgp project> LHTML_PYTHON=<python with playwright> python3 scripts/build_examples.py

Two kinds of examples, given by the field "kind" of their demo.json:
  "opengl"  examples/<id>/CMakeLists.txt (raw OpenGL 3.3, examples/cmake/raw_opengl_demo.cmake)
  "cgp"     only the files that differ from the CGP template project: src/ (scene.cpp...),
            shaders/, assets/. They are staged over a copy of the CGP project (CGP_DIR: its
            CMakeLists.txt, cgp/, src/, shaders/, scripts/) in build/<id>-stage/, with the bridge,
            and built by the CMakeLists of CGP itself.

For each example:
  1. web build in build/<id>-web/ (and, with --native, a native build in build/<id>-native/,
     run for two seconds to check that it starts);
  2. copy of index.html and project.js/.wasm/.data into examples/<id>/web/ (committed: the
     demos work without Emscripten, and offline);
  3. test in a browser (tests/test_demo.py) and poster examples/<id>/poster.png (image for the PDF).
Log: build/build_examples.log. Exit code 0 when every example builds and passes.
"""
import json, os, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXAMPLES = os.path.join(ROOT, 'examples')
BRIDGE = os.path.join(ROOT, 'bridge')
BUILD = os.path.join(ROOT, 'build')
DEFAULT_PYTHON = os.path.expanduser('~/Library/Application Support/pipx/venvs/lhtml-site/bin/python')
DEFAULT_CGP = os.path.expanduser('~/drives/damien/cgp_library/cgp_library/cgp-library')
OUTPUTS = ['index.html', 'project.js', 'project.wasm', 'project.data']

# Appended to the CMakeLists.txt of CGP in a staged project
CGP_CMAKE_SUFFIX = '''
# ---- interactive-3d-slides: the bridge, and the page of the demo for a slide
if(EMSCRIPTEN)
    target_link_options(${{PROJECT_NAME}} PRIVATE
        -sEXPORTED_FUNCTIONS=_main,_slide_set,_slide_describe -sEXPORTED_RUNTIME_METHODS=ccall)
    # after the copy of the template of CGP (POST_BUILD commands run in order)
    add_custom_command(TARGET ${{PROJECT_NAME}} POST_BUILD
        COMMAND ${{CMAKE_COMMAND}} -E copy "{frame}" "${{CMAKE_BINARY_DIR}}/index.html")
endif()
'''


def log(message, stream):
    line = time.strftime('%H:%M:%S ') + message
    print(line, flush=True)
    stream.write(line + '\n')
    stream.flush()


def run(command, stream, cwd=None):
    stream.write('$ ' + ' '.join(command) + '\n')
    stream.flush()
    return subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT).returncode == 0


def stage_cgp(ident, cgp_dir):
    """A CGP project: the template, the files of the example on top, and the bridge."""
    source = os.path.join(EXAMPLES, ident)
    stage = os.path.join(BUILD, ident + '-stage')
    shutil.rmtree(stage, ignore_errors=True)
    os.makedirs(stage)
    shutil.copy2(os.path.join(cgp_dir, 'CMakeLists.txt'), stage)
    for folder in ['cgp', 'src', 'shaders', 'scripts']:
        shutil.copytree(os.path.join(cgp_dir, folder), os.path.join(stage, folder))
    for folder in ['src', 'shaders', 'assets']:
        if os.path.isdir(os.path.join(source, folder)):
            shutil.copytree(os.path.join(source, folder), os.path.join(stage, folder), dirs_exist_ok=True)
    for name in ['slide_bridge.hpp', 'slide_bridge.cpp']:
        shutil.copy2(os.path.join(BRIDGE, name), os.path.join(stage, 'src'))
    with open(os.path.join(stage, 'CMakeLists.txt'), 'a') as f:
        f.write(CGP_CMAKE_SUFFIX.format(frame=os.path.join(BRIDGE, 'slide-frame.html')))
    return stage


def check_native(build, project, stream):
    """Runs the native program for two seconds: it must start and not stop by itself.
    It runs from the root of its project: CGP looks for shaders/ from the current folder."""
    program = os.path.join(build, 'project')
    stream.write(f'$ (cd {project}; {program}) (2 s)\n')
    stream.flush()
    process = subprocess.Popen([program], cwd=project, stdout=stream, stderr=subprocess.STDOUT)
    time.sleep(2)
    alive = process.poll() is None
    process.kill()
    process.wait()
    return alive


def main():
    args = sys.argv[1:]
    native = '--native' in args
    args = [a for a in args if a != '--native']
    if shutil.which('emcmake') is None:
        sys.exit('emcmake not found: activate the Emscripten SDK first (source <emsdk>/emsdk_env.sh)')
    python = os.environ.get('LHTML_PYTHON', DEFAULT_PYTHON)
    if not os.path.exists(python):
        python = sys.executable
    cgp_dir = os.environ.get('CGP_DIR', DEFAULT_CGP)
    ids = args or sorted(d for d in os.listdir(EXAMPLES) if os.path.exists(os.path.join(EXAMPLES, d, 'demo.json')))
    os.makedirs(BUILD, exist_ok=True)
    failed = []
    with open(os.path.join(BUILD, 'build_examples.log'), 'w') as stream:
        for n, ident in enumerate(ids, 1):
            tag = f'[{n}/{len(ids)}] {ident}'
            source = os.path.join(EXAMPLES, ident)
            kind = json.load(open(os.path.join(source, 'demo.json'))).get('kind', 'opengl')
            if kind == 'cgp':
                if not os.path.exists(os.path.join(cgp_dir, 'cgp')):
                    log(f'{tag}: no CGP project in {cgp_dir} (set CGP_DIR)', stream)
                    failed.append(ident)
                    continue
                project = stage_cgp(ident, cgp_dir)
            else:
                project = source

            build = os.path.join(BUILD, ident + '-web')
            log(f'{tag}: web build ({kind})', stream)
            started = time.time()
            ok = run(['emcmake', 'cmake', '-S', project, '-B', build, '-DCMAKE_BUILD_TYPE=Release'], stream) \
                and run(['cmake', '--build', build, '-j', '8'], stream)
            if not ok:
                log(f'{tag}: BUILD FAILED (see the log)', stream)
                failed.append(ident)
                continue
            log(f'{tag}: web build done in {time.time() - started:.0f} s', stream)

            if native:
                build_native = os.path.join(BUILD, ident + '-native')
                log(f'{tag}: native build', stream)
                ok = run(['cmake', '-S', project, '-B', build_native, '-DCMAKE_BUILD_TYPE=Release'], stream) \
                    and run(['cmake', '--build', build_native, '-j', '8'], stream) \
                    and check_native(build_native, project, stream)
                log(f'{tag}: native ' + ('starts' if ok else 'FAILED (see the log)'), stream)
                if not ok:
                    failed.append(ident)

            web = os.path.join(source, 'web')
            shutil.rmtree(web, ignore_errors=True)
            os.makedirs(web)
            for name in OUTPUTS:
                if os.path.exists(os.path.join(build, name)):
                    shutil.copy2(os.path.join(build, name), web)
            size = sum(os.path.getsize(os.path.join(web, f)) for f in os.listdir(web)) / 1e6
            log(f'{tag}: web/ {size:.1f} MB; browser test', stream)
            ok = run([python, os.path.join(ROOT, 'tests', 'test_demo.py'), web, os.path.join(source, 'demo.json'),
                      '--poster', os.path.join(source, 'poster.png')], stream)
            log(f'{tag}: ' + ('passed' if ok else 'TEST FAILED (see the log)'), stream)
            if not ok and ident not in failed:
                failed.append(ident)
        log('done: ' + ('all passed' if not failed else 'failed: ' + ', '.join(failed)), stream)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
