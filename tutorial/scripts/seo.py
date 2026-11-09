"""What search engines read, added after lhtml-site has built both languages (run by build.sh).

    python3 scripts/seo.py .site

- in the <head> of each page of the tutorial (English in .site/, French in .site/fr/): canonical,
  hreflang (en, fr, x-default) when the page exists in both languages, Open Graph; the home pages
  get a real title instead of "Home";
- .site/sitemap.xml: the pages of both languages, with their alternates.

Only the pages of the tutorial are touched: demos/, decks/ and code/ (the demos and example talks)
are left as built, and the pages marked noindex (404) are skipped.
"""

import html
import re
import sys
from pathlib import Path

BASE = 'https://scholartools.fr/interactive-3d-slides/'
LANGS = {
    'en': {'base': BASE, 'locale': 'en_US',
           'home': 'Interactive 3D in slides: OpenGL and CGP models in your presentations'},
    'fr': {'base': BASE + 'fr/', 'locale': 'fr_FR',
           'home': 'La 3D interactive dans vos slides : des modèles OpenGL et CGP dans vos présentations'},
}
SKIP = ('demos/', 'decks/', 'code/', 'fr/', 'assets/')
IMAGE = BASE + 'demos/c6/poster.png'


def pages(root):
    """{rel: path} of the tutorial pages under root, rel as in the address ('' for the home)."""
    out = {}
    for p in sorted(root.rglob('*.html')):
        rel = p.relative_to(root).as_posix()
        if rel.startswith(SKIP) or rel == '404.html':
            continue
        text = p.read_text(encoding='utf-8')
        if re.search(r'<meta name="robots" content="[^"]*noindex', text):
            continue
        out[re.sub(r'(^|/)index\.html$', r'\1', rel)] = p
    return out


def head(lang, rel, title, desc, both):
    url = LANGS[lang]['base'] + rel
    tags = [f'<link rel="canonical" href="{url}">']
    if rel in both:
        tags += [f'<link rel="alternate" hreflang="{c}" href="{LANGS[c]["base"] + rel}">' for c in LANGS]
        tags.append(f'<link rel="alternate" hreflang="x-default" href="{BASE + rel}">')
    other = 'fr' if lang == 'en' else 'en'
    tags += [
        '<meta property="og:type" content="website">',
        '<meta property="og:site_name" content="ScholarTools">',
        f'<meta property="og:title" content="{html.escape(title, quote=True)}">',
        f'<meta property="og:description" content="{html.escape(desc, quote=True)}">',
        f'<meta property="og:url" content="{url}">',
        f'<meta property="og:locale" content="{LANGS[lang]["locale"]}">',
        f'<meta property="og:locale:alternate" content="{LANGS[other]["locale"]}">',
        f'<meta property="og:image" content="{IMAGE}">',
        '<meta name="twitter:card" content="summary_large_image">',
    ]
    return '\n'.join(tags) + '\n'


def patch(lang, listed, both):
    for rel, p in listed.items():
        text = p.read_text(encoding='utf-8')
        if 'rel="canonical"' in text:
            continue                                   # already done (a second run)
        if rel == '':
            text = re.sub(r'<title>[^<]*</title>', f'<title>{html.escape(LANGS[lang]["home"])}</title>',
                          text, count=1)
        title = html.unescape(re.search(r'<title>([^<]*)</title>', text).group(1))
        m = re.search(r'<meta name="description" content="([^"]*)"', text)
        desc = html.unescape(m.group(1)) if m else ''
        text = text.replace('</head>', head(lang, rel, title, desc, both) + '</head>', 1)
        p.write_text(text, encoding='utf-8')


def sitemap(listed, both):
    urls = []
    for lang, pl in listed.items():
        for rel in pl:
            alt = ''
            if rel in both:
                alt = ''.join(f'\n    <xhtml:link rel="alternate" hreflang="{c}" href="{LANGS[c]["base"] + rel}"/>'
                              for c in LANGS)
                alt += f'\n    <xhtml:link rel="alternate" hreflang="x-default" href="{BASE + rel}"/>'
            urls.append(f'  <url>\n    <loc>{LANGS[lang]["base"] + rel}</loc>{alt}\n  </url>')
    return ('<?xml version="1.0" encoding="UTF-8"?>\n'
            '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9" xmlns:xhtml="http://www.w3.org/1999/xhtml">\n'
            + '\n'.join(urls) + '\n</urlset>\n')


def main():
    site = Path(sys.argv[1])
    listed = {'en': pages(site), 'fr': pages(site / 'fr')}
    both = set(listed['en']) & set(listed['fr'])
    for lang in LANGS:
        patch(lang, listed[lang], both)
    (site / 'sitemap.xml').write_text(sitemap(listed, both), encoding='utf-8')
    print(f"SEO: {len(listed['en'])} pages en, {len(listed['fr'])} pages fr ({len(both)} in both), sitemap.xml")


if __name__ == '__main__':
    main()
