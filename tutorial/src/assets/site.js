// The two languages: the English site at the root, the French one in fr/ (configure.fr.yaml), with
// the same paths. A link in the top bar goes to the same page in the other language.
(function () {
  var lang = document.documentElement.lang === 'fr' ? 'fr' : 'en';
  var actions = document.querySelector('.topbar-actions');
  if (!actions || document.body.classList.contains('page-not-found')) return;
  var root = new URL(document.body.dataset.root || './', location.href);
  var path = location.pathname.slice(root.pathname.length) + location.hash;
  var other = lang === 'fr' ? new URL('../' + path, root) : new URL('fr/' + path, root);
  var a = document.createElement('a');
  a.className = 'icon-btn lang-switch';
  a.href = other.href;
  a.hreflang = lang === 'fr' ? 'en' : 'fr';
  a.lang = a.hreflang;
  a.textContent = lang === 'fr' ? 'EN' : 'FR';
  a.title = lang === 'fr' ? 'Read this page in English' : 'Lire cette page en français';
  actions.insertBefore(a, actions.firstChild);
})();
