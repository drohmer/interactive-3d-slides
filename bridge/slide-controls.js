// slide-controls.js: controls in the slide itself, outside the demo, with HTML attributes only.
//
// Load it in the slide (<script src=".../slide-controls.js"></script>), then:
//   <button data-demo-set="shape=ripple">Ripple</button>     sets a parameter of the demo
//   <button data-demo-set="ampl=0.2&speed=6">Strong</button> sets several at once
//   <button data-demo-set="drop">Drop</button>               runs an action (a parameter without value)
//   <span data-demo-value="ampl" data-digits="2"></span>      shows the current value of a parameter
//   <input type="range" data-demo-input="ampl" min="0" max="0.3" step="0.01">
//                                                            a slider of the slide, which sets the parameter
// A button whose settings are the current values of the demo gets the class "on".
// The demo is the first iframe.demo of the page (or the first iframe); data-demo="#id" names another, and
// data-demo="#left, #right" (any CSS selector) drives several demos at once: the buttons and the sliders
// set all of them, the values shown are those of the first one.
//
// The demo is a page made from slide-frame.html: the messages go through postMessage, so the demo
// may come from another origin. From a script: slideDemo.set({ampl: 0.2}), slideDemo.params().
(function () {
  'use strict';
  var params = new Map();   // demo window -> {key: parameter}

  function framesOf(element) {
    var selector = element && element.getAttribute('data-demo');
    if (selector) return Array.prototype.slice.call(document.querySelectorAll(selector));
    var frame = document.querySelector('iframe.demo') || document.querySelector('iframe');
    return frame ? [frame] : [];
  }

  function frameOf(element) {
    return framesOf(element)[0] || null;
  }

  function parse(text) {
    var settings = {};
    text.split('&').forEach(function (pair) {
      if (!pair) return;
      var k = pair.indexOf('=');
      if (k < 0) settings[decodeURIComponent(pair)] = '';
      else settings[decodeURIComponent(pair.slice(0, k))] = decodeURIComponent(pair.slice(k + 1));
    });
    return settings;
  }

  function set(settings, frames) {
    frames = frames ? (Array.isArray(frames) ? frames : [frames]) : framesOf(null);
    frames.forEach(function (frame) {
      if (frame && frame.contentWindow) frame.contentWindow.postMessage({slide: {set: settings}}, '*');
    });
  }

  function same(value, text) {
    if (typeof value === 'boolean') return value === (text === '1' || text === 'true' || text === 'on');
    if (typeof value === 'number') return Math.abs(value - Number(text)) < 1e-6;
    return String(value) === text;
  }

  function update() {
    document.querySelectorAll('[data-demo-set]').forEach(function (button) {
      var frame = frameOf(button);
      var current = frame && params.get(frame.contentWindow);
      if (!current) return;
      var settings = parse(button.getAttribute('data-demo-set'));
      var keys = Object.keys(settings);
      var on = keys.length > 0 && keys.every(function (key) {
        var p = current[key];
        return p && p.type !== 'action' && same(p.value, settings[key]);
      });
      button.classList.toggle('on', on);
    });
    document.querySelectorAll('[data-demo-input]').forEach(function (input) {
      if (input === document.activeElement) return;   // not while it is moved
      var frame = frameOf(input);
      var current = frame && params.get(frame.contentWindow);
      var p = current && current[input.getAttribute('data-demo-input')];
      if (p && typeof p.value === 'number') input.value = p.value;
    });
    document.querySelectorAll('[data-demo-value]').forEach(function (span) {
      var frame = frameOf(span);
      var current = frame && params.get(frame.contentWindow);
      var p = current && current[span.getAttribute('data-demo-value')];
      if (!p) return;
      var digits = span.getAttribute('data-digits');
      span.textContent = (typeof p.value === 'number' && digits !== null) ? p.value.toFixed(Number(digits)) : String(p.value);
    });
  }

  document.addEventListener('click', function (event) {
    var button = event.target.closest && event.target.closest('[data-demo-set]');
    if (!button) return;
    event.preventDefault();
    set(parse(button.getAttribute('data-demo-set')), framesOf(button));
  });

  document.addEventListener('input', function (event) {
    var input = event.target.closest && event.target.closest('[data-demo-input]');
    if (!input) return;
    var settings = {};
    settings[input.getAttribute('data-demo-input')] = input.value;
    set(settings, framesOf(input));
  });

  // a slider released keeps the focus, and the slide tools ignore the arrow keys while an input has it:
  // give the focus back to the page, so that the arrow keys change the slide again
  document.addEventListener('change', function (event) {
    var input = event.target.closest && event.target.closest('[data-demo-input]');
    if (input) input.blur();
  });

  window.addEventListener('message', function (event) {
    var m = event.data && event.data.slide;
    if (!m || typeof m !== 'object') return;
    if (m.key) {   // a navigation key from a demo of another origin
      // a deck of lhtml-site handles these messages itself (window.__lhtmlNavigation)
      if (!window.__lhtmlNavigation) {
        // with keyCode and which: some slide tools (reveal.js) read them
        var copy = new KeyboardEvent('keydown', {key: m.key, bubbles: true, cancelable: true});
        var code = {ArrowLeft: 37, ArrowUp: 38, ArrowRight: 39, ArrowDown: 40, PageUp: 33, PageDown: 34}[m.key] || 0;
        Object.defineProperty(copy, 'keyCode', {get: function () { return code; }});
        Object.defineProperty(copy, 'which', {get: function () { return code; }});
        document.dispatchEvent(copy);
      }
      return;
    }
    if (m.params) {
      var current = {};
      m.params.forEach(function (p) { current[p.key] = p; });
      params.set(event.source, current);
      update();
    }
  });

  window.slideDemo = {
    set: set,
    params: function (frame) { frame = frame || frameOf(null); return frame ? params.get(frame.contentWindow) : undefined; }
  };
})();
