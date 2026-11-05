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
             '.js': 'javascript', '.json': 'json', '.py': 'python', '.sh': 'bash', '.conf': 'nginx'}

# What each example needs, besides its own folder (repository-relative paths)
BRIDGE = ['bridge/slide_bridge.hpp', 'bridge/slide_bridge.cpp', 'bridge/slide-frame.html', 'bridge/slide-controls.js']
RAW_OPENGL = BRIDGE + ['examples/cmake/raw_opengl_demo.cmake', 'examples/third_party/glad']
CGP = BRIDGE + ['scripts/build_examples.py', 'tests/test_demo.py']
EXAMPLES = {
    'a1-opengl-cube': RAW_OPENGL,
    'a2-opengl-uniforms': RAW_OPENGL,
    'c1-cgp-template': CGP,
    'c3-cgp-waving-surface': CGP,
    'c6-cgp-marching-cubes': CGP,
    'c7-cgp-drag-points': CGP,
}


def tracked():
    """The files of the repository that git tracks or would track (not ignored: no build output, no
    local state), without the tutorial itself."""
    out = subprocess.run(['git', '-C', REPO, 'ls-files', '--cached', '--others', '--exclude-standard'],
                         check=True, capture_output=True, text=True).stdout
    return sorted(f for f in set(out.splitlines())
                  if not f.startswith('tutorial/') and os.path.isfile(os.path.join(REPO, f)))


def fragments():
    target = os.path.join(TUTORIAL, 'src', '_code')
    shutil.rmtree(target, ignore_errors=True)
    count = 0
    for path in tracked():
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


def publish(site):
    files = tracked()
    code = os.path.join(site, 'code')
    shutil.rmtree(code, ignore_errors=True)
    for path in files:
        os.makedirs(os.path.dirname(os.path.join(code, path)), exist_ok=True)
        shutil.copy2(os.path.join(REPO, path), os.path.join(code, path))

    def write_zip(name, paths):
        with zipfile.ZipFile(os.path.join(code, name), 'w', zipfile.ZIP_DEFLATED) as z:
            for path in sorted(set(paths)):
                z.write(os.path.join(REPO, path), os.path.join('interactive-3d-slides', path))
        return os.path.getsize(os.path.join(code, name)) / 1e6

    for example, needs in EXAMPLES.items():
        size = write_zip(example + '.zip', expand(['examples/' + example] + needs + ['README.md', 'Makefile'], files))
        print(f'zip: code/{example}.zip ({size:.1f} MB)')
    size = write_zip('interactive-3d-slides.zip', files)
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
