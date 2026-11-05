"""Build the examples for the web, test them in a browser, and capture their posters.

Usage (the Emscripten SDK active: source <emsdk>/emsdk_env.sh):
  python3 scripts/build_examples.py [id ...]          all the examples by default
  LHTML_PYTHON=<python with playwright> python3 scripts/build_examples.py

For each examples/<id>/ that has a CMakeLists.txt and a demo.json:
  1. web build in build/<id>-web/ (emcmake cmake, cmake --build);
  2. copy of index.html and project.js/.wasm/.data into examples/<id>/web/ (committed: the
     demos work without Emscripten, and offline);
  3. test in a browser (tests/test_demo.py) and poster examples/<id>/poster.png (image for the PDF).
Log: build/build_examples.log. Exit code 0 when every example builds and passes.
"""
import os, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXAMPLES = os.path.join(ROOT, 'examples')
BUILD = os.path.join(ROOT, 'build')
DEFAULT_PYTHON = os.path.expanduser('~/Library/Application Support/pipx/venvs/lhtml-site/bin/python')
OUTPUTS = ['index.html', 'project.js', 'project.wasm', 'project.data']


def log(message, stream):
    line = time.strftime('%H:%M:%S ') + message
    print(line, flush=True)
    stream.write(line + '\n')
    stream.flush()


def run(command, stream, cwd=None):
    stream.write('$ ' + ' '.join(command) + '\n')
    stream.flush()
    return subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT).returncode == 0


def main():
    if shutil.which('emcmake') is None:
        sys.exit('emcmake not found: activate the Emscripten SDK first (source <emsdk>/emsdk_env.sh)')
    python = os.environ.get('LHTML_PYTHON', DEFAULT_PYTHON)
    if not os.path.exists(python):
        python = sys.executable
    ids = sys.argv[1:] or sorted(d for d in os.listdir(EXAMPLES)
                                 if os.path.exists(os.path.join(EXAMPLES, d, 'CMakeLists.txt'))
                                 and os.path.exists(os.path.join(EXAMPLES, d, 'demo.json')))
    os.makedirs(BUILD, exist_ok=True)
    failed = []
    with open(os.path.join(BUILD, 'build_examples.log'), 'w') as stream:
        for n, ident in enumerate(ids, 1):
            source = os.path.join(EXAMPLES, ident)
            build = os.path.join(BUILD, ident + '-web')
            log(f'[{n}/{len(ids)}] {ident}: web build', stream)
            ok = run(['emcmake', 'cmake', '-S', source, '-B', build, '-DCMAKE_BUILD_TYPE=Release'], stream) \
                and run(['cmake', '--build', build, '-j', '8'], stream)
            if not ok:
                log(f'[{n}/{len(ids)}] {ident}: BUILD FAILED (see the log)', stream)
                failed.append(ident)
                continue
            web = os.path.join(source, 'web')
            shutil.rmtree(web, ignore_errors=True)
            os.makedirs(web)
            for name in OUTPUTS:
                if os.path.exists(os.path.join(build, name)):
                    shutil.copy2(os.path.join(build, name), web)
            size = sum(os.path.getsize(os.path.join(web, f)) for f in os.listdir(web)) / 1e6
            log(f'[{n}/{len(ids)}] {ident}: web/ {size:.1f} MB; browser test', stream)
            ok = run([python, os.path.join(ROOT, 'tests', 'test_demo.py'), web, os.path.join(source, 'demo.json'),
                      '--poster', os.path.join(source, 'poster.png')], stream)
            log(f'[{n}/{len(ids)}] {ident}: ' + ('passed' if ok else 'TEST FAILED (see the log)'), stream)
            if not ok:
                failed.append(ident)
        log('done: ' + ('all passed' if not failed else 'failed: ' + ', '.join(failed)), stream)
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
