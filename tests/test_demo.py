"""End-to-end test of a demo in a slide, and capture of its poster.

Usage: python test_demo.py <web_dir> <demo.json> [--poster out.png]
  <web_dir>    index.html (bridge/slide-frame.html) and project.js/.wasm(/.data)
  <demo.json>  what to check (see examples/*/demo.json):
    "query":   address of the demo in the test slide (settings, controls=...)
    "panel":   number of rows expected in the panel (optional)
    "steps":   list of {"set": {key: value}}       postMessage from the slide
                       {"click": "text"}            click on the button of the panel with this text
                       {"slide_click": "text"}      click on a button of the slide (outside the demo)
                       {"slide_text": [sel, text]}  text of an element of the slide
                       {"slide_on": ["text", ...]}  the buttons of the slide shown as on
                       {"drag": [[x0, y0], [x1, y1]]}  mouse drag in the demo (pixels of the frame)
                       {"drag_pixels": {"pixels": "b > 150 && b - r > 80", "by": [dx, dy]}}
                                                    drag from the leftmost group of pixels that match
                       {"remember": key}, {"changed": key}   a value changed since it was remembered
                       {"slide_shows": [sel, key, digits]}   the slide shows the current value of key
                       {"key": "z"}                  a key pressed in the demo (after a click or a drag in it)
                       {"wait": seconds}
                       {"snapshot": "name"}         keep the image of the canvas
                       {"same": ["a", "b"]}         the two images are equal
                       {"differ": ["a", "b"]}       the two images differ
                       {"expect": {key: value}}     values of the parameters (numbers within 1e-3)
    "slide":   HTML put in the test slide under the demo, with bridge/slide-controls.js (optional)
    "poster":  {"query": ..., "wait": seconds}      address and delay of the poster image (optional)
    "pair":    {"queries": [q_left, q_right], "slide": HTML, "input": {key: value}}   (optional)
               two copies of the demo, #left and #right, in one slide: each slider data-demo-input=key of
               the slide is set to value, and both demos must take it (controls with data-demo="#left, #right")

Every demo is also checked for: the ready message, window.__lhtmlReady, a canvas that is not blank,
the arrow keys reaching the slide while the demo has the focus, and no JavaScript error.
Needs Playwright (the Python of lhtml-site has it). WebGL by SwiftShader: no GPU needed.
Exit code 0 when every check passes.
"""
import argparse, functools, hashlib, http.server, json, os, shutil, sys, tempfile, threading, time
from playwright.sync_api import sync_playwright

BROWSER_ARGS = ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader']

DECK = '''<!DOCTYPE html><html><head><meta charset="utf-8"><title>test slide</title></head>
<body style="margin:0">
<iframe id="demo" class="demo" src="demo/index.html?{query}" style="width:{w}px;height:{h}px;border:0;display:block"></iframe>
<div id="slide">{slide}</div>
<script src="slide-controls.js"></script>
<script>
  window.messages = []; window.keys = [];
  window.addEventListener('message', function (e) {{ if (e.data && e.data.slide) window.messages.push(e.data.slide); }});
  document.addEventListener('keydown', function (e) {{ window.keys.push(e.key); }});
</script></body></html>'''


PAIR = '''<!DOCTYPE html><html><head><meta charset="utf-8"><title>test slide, two demos</title></head>
<body style="margin:0">
<iframe id="left" class="demo" src="demo/index.html?{q0}" style="width:{w}px;height:{h}px;border:0"></iframe>
<iframe id="right" class="demo" src="demo/index.html?{q1}" style="width:{w}px;height:{h}px;border:0"></iframe>
<div id="slide">{slide}</div>
<script src="slide-controls.js"></script></body></html>'''


def check_pair(browser, port, pair, check, errors):
    """Two copies of the demo in one slide, driven by the same slider."""
    page = browser.new_page(viewport={'width': 1240, 'height': 600})
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.goto(f'http://127.0.0.1:{port}/pair.html')
    both = "['left', 'right'].every(id => window.slideDemo.params(document.getElementById(id)))"
    page.wait_for_function(both, timeout=60000)
    for key, value in pair.get('input', {}).items():
        selector = f'[data-demo-input="{key}"]'
        page.focus(selector)
        page.evaluate("""([sel, v]) => { const i = document.querySelector(sel); i.value = v;
            i.dispatchEvent(new Event('input', {bubbles: true})); i.dispatchEvent(new Event('change', {bubbles: true})); }""",
                      [selector, str(value)])
        got = lambda: page.evaluate(f"""['left', 'right'].map(id =>
            window.slideDemo.params(document.getElementById(id))['{key}'].value)""")
        deadline = time.time() + 5
        while time.time() < deadline and not all(abs(v - value) < 1e-3 for v in got()):
            time.sleep(0.1)
        check(f'slider {key} = {value} in both demos', all(abs(v - value) < 1e-3 for v in got()), got())
        check(f'slider {key} gives the focus back', not page.evaluate(f"document.activeElement.matches('{selector}')"))
    page.close()


class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass


def serve(root):
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Quiet, directory=root))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def open_demo(browser, port, query, w, h, errors):
    page = browser.new_page(viewport={'width': w + 40, 'height': h + 160})
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.on('console', lambda m: errors.append(m.text) if m.type == 'error' else None)
    page.goto(f'http://127.0.0.1:{port}/deck_{abs(hash(query))}.html')
    page.wait_for_function("window.messages.some(m => m.ready)", timeout=60000)
    return page


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('web_dir')
    parser.add_argument('spec')
    parser.add_argument('--poster')
    parser.add_argument('--shot', help='screenshot of the test slide after the steps')
    parser.add_argument('--size', default='1200x700')
    options = parser.parse_args()
    spec = json.load(open(options.spec))
    w, h = (int(x) for x in options.size.split('x'))

    root = tempfile.mkdtemp(prefix='slide-demo-test-')
    shutil.copytree(options.web_dir, os.path.join(root, 'demo'))
    queries = [spec.get('query', '')] + ([spec['poster'].get('query', '')] if 'poster' in spec else [])
    for q in queries:
        with open(os.path.join(root, f'deck_{abs(hash(q))}.html'), 'w') as f:
            f.write(DECK.format(query=q, w=w, h=h, slide=spec.get('slide', '') if q == queries[0] else ''))
    shutil.copy2(os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'bridge', 'slide-controls.js'), root)
    if 'pair' in spec:
        q0, q1 = spec['pair']['queries']
        with open(os.path.join(root, 'pair.html'), 'w') as f:
            f.write(PAIR.format(q0=q0, q1=q1, w=600, h=400, slide=spec['pair'].get('slide', '')))
    server = serve(root)
    port = server.server_address[1]

    failures = []
    def check(name, condition, detail=''):
        print(('ok   ' if condition else 'FAIL ') + name + (f'  {detail}' if detail and not condition else ''))
        if not condition:
            failures.append(name)

    with sync_playwright() as p:
        browser = p.chromium.launch(args=BROWSER_ARGS)
        errors = []
        t0 = time.time()
        page = open_demo(browser, port, spec.get('query', ''), w, h, errors)
        print(f'     ready after {time.time() - t0:.1f} s')
        demo = page.frames[1]

        def params():
            return {q['key']: q for q in page.evaluate('window.messages')[-1]['params']}
        def image():
            return demo.evaluate("document.getElementById('canvas').toDataURL()")

        check('ready message', True)
        check('__lhtmlReady resolves', demo.evaluate("window.__lhtmlReady.ready().then(() => true)"))
        blank = demo.evaluate("""(() => { const c = document.getElementById('canvas'); const t = document.createElement('canvas');
            t.width = c.width; t.height = c.height; return t.toDataURL(); })()""")
        check('canvas not blank', image() != blank)
        size = demo.evaluate("[innerWidth, innerHeight, canvas.width, canvas.height]")
        check('canvas fills the frame', size[0:2] == size[2:4], size)
        if 'panel' in spec:
            rows = demo.evaluate("document.querySelectorAll('#controls .row').length")
            check(f'panel has {spec["panel"]} rows', rows == spec['panel'], rows)

        snapshots, remembered = {}, {}
        for step in spec.get('steps', []):
            if 'set' in step:
                page.evaluate("(s) => document.getElementById('demo').contentWindow.postMessage({slide: {set: s}}, '*')", step['set'])
                time.sleep(0.25)
            elif 'click' in step:
                page.frame_locator('#demo').locator('#controls button', has_text=step['click']).first.click()
                time.sleep(0.25)
            elif 'slide_click' in step:
                page.locator('#slide button', has_text=step['slide_click']).first.click()
                time.sleep(0.3)
            elif 'slide_on' in step:
                on = page.evaluate("[...document.querySelectorAll('#slide button.on')].map(b => b.textContent.trim())")
                check(f'slide buttons on: {step["slide_on"]}', on == step['slide_on'], on)
            elif 'slide_text' in step:
                selector, text = step['slide_text']
                got = page.locator('#slide ' + selector).first.text_content()
                check(f'slide shows {selector} = {text}', got == text, got)
            elif 'remember' in step:
                remembered[step['remember']] = params()[step['remember']]['value']
            elif 'changed' in step:
                key = step['changed']
                now = params()[key]['value']
                check(f'{key} changed', now != remembered.get(key), (remembered.get(key), now))
            elif 'slide_shows' in step:
                selector, key, digits = step['slide_shows']
                expected = f"{params()[key]['value']:.{digits}f}"
                got = page.locator('#slide ' + selector).first.text_content()
                check(f'slide shows the current {key}', got == expected, (got, expected))
            elif 'drag_pixels' in step:
                # drag from the leftmost group of pixels of the canvas that match a condition on r, g, b
                spec_drag = step['drag_pixels']
                start = demo.evaluate("""(test) => {
                    const c = document.getElementById('canvas'), t = document.createElement('canvas');
                    t.width = c.width; t.height = c.height;
                    const g = t.getContext('2d'); g.drawImage(c, 0, 0);
                    const d = g.getImageData(0, 0, t.width, t.height).data, keep = new Function('r', 'g', 'b', 'return ' + test);
                    let points = [];
                    for (let y = 0; y < t.height; y += 2) for (let x = 0; x < t.width; x += 2) {
                        const i = 4 * (y * t.width + x);
                        if (keep(d[i], d[i + 1], d[i + 2])) points.push([x, y]);
                    }
                    if (points.length === 0) return null;
                    const left = Math.min(...points.map(p => p[0]));
                    points = points.filter(p => p[0] < left + 40);
                    return [points.reduce((s, p) => s + p[0], 0) / points.length, points.reduce((s, p) => s + p[1], 0) / points.length];
                }""", spec_drag['pixels'])
                check('pixels found for the drag', start is not None)
                if start:
                    x0, y0 = start
                    dx, dy = spec_drag['by']
                    page.mouse.move(x0, y0)
                    page.mouse.down()
                    for k in range(1, 11):
                        page.mouse.move(x0 + dx * k / 10, y0 + dy * k / 10)
                        time.sleep(0.03)
                    page.mouse.up()
                    time.sleep(0.3)
            elif 'drag' in step:
                (x0, y0), (x1, y1) = step['drag']
                page.mouse.move(x0, y0)
                page.mouse.down()
                for k in range(1, 11):
                    page.mouse.move(x0 + (x1 - x0) * k / 10, y0 + (y1 - y0) * k / 10)
                    time.sleep(0.02)
                page.mouse.up()
                time.sleep(0.2)
            elif 'key' in step:   # a key pressed in the demo (the demo has the keyboard after a click in it)
                page.keyboard.press(step['key'])
                time.sleep(0.3)
            elif 'wait' in step:
                time.sleep(step['wait'])
            elif 'snapshot' in step:
                snapshots[step['snapshot']] = hashlib.md5(image().encode()).hexdigest()
            elif 'same' in step:
                a, b = step['same']
                check(f'{a} and {b} are the same image', snapshots[a] == snapshots[b])
            elif 'differ' in step:
                a, b = step['differ']
                check(f'{a} and {b} differ', snapshots[a] != snapshots[b])
            elif 'expect' in step:
                current = params()
                for key, value in step['expect'].items():
                    got = current.get(key, {}).get('value')
                    ok = abs(got - value) < 1e-3 if isinstance(value, (int, float)) and not isinstance(value, bool) and isinstance(got, (int, float)) else got == value
                    check(f'{key} = {value}', ok, got)

        if options.shot:
            page.screenshot(path=options.shot)
            print(f'     shot of the test slide: {options.shot}')
        demo.locator('#canvas').click(position={'x': w // 2, 'y': h // 2})
        page.keyboard.press('ArrowRight')
        time.sleep(0.2)
        check('ArrowRight reaches the slide', 'ArrowRight' in page.evaluate('window.keys'), page.evaluate('window.keys'))
        page.close()

        if 'pair' in spec:
            check_pair(browser, port, spec['pair'], check, errors)

        if options.poster and 'poster' in spec:
            poster = open_demo(browser, port, spec['poster'].get('query', ''), w, h, errors)
            time.sleep(spec['poster'].get('wait', 0.5))
            poster.locator('#demo').screenshot(path=options.poster)
            print(f'     poster: {options.poster}')
            poster.close()

        check('no JavaScript error', not errors, errors[:5])
        browser.close()

    server.shutdown()
    shutil.rmtree(root, ignore_errors=True)
    print('ALL PASSED' if not failures else f'{len(failures)} FAILED: {failures}')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
