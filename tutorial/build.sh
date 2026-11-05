#!/usr/bin/env bash
set -euo pipefail

# === The tutorial "Create slides with interactive 3D": the whole site ===
# Usage: ./build.sh     (needs lhtml-site; the demos are the builds committed in ../examples/*/web)
#   .site/              the English pages
#   .site/fr/           the French pages
#   .site/demos/<id>/   the live demos (a1, a2, c1, c3, c6, c7) and slide-controls.js
#   .site/decks/lhtml-site/   an example talk made with lhtml-site (html/, slides.pdf)
#   .site/decks/reveal.js/    the same demos in a reveal.js talk
# Preview: python3 -m http.server -d <a folder holding interactive-3d-slides -> .site> and open
# http://localhost:8000/interactive-3d-slides/ (the pages are made for that path).

cd "$(dirname "$0")"
LHTML_SITE="${LHTML_SITE:-lhtml-site}"

rm -rf .site .site-fr .work
echo "==> pages, English and French"
"$LHTML_SITE" -i configure.yaml
"$LHTML_SITE" -i configure.fr.yaml
cp -R .site-fr .site/fr

echo "==> live demos"
mkdir -p .site/demos
for web in ../examples/*/web; do
    example=$(dirname "$web")
    id=$(basename "$example")
    short=${id%%-*}                      # a1-opengl-cube -> a1
    cp -R "$web" ".site/demos/$short"
    cp "$example/poster.png" ".site/demos/$short/"
done
cp ../bridge/slide-controls.js .site/demos/

echo "==> example talks"
mkdir -p .work .site/decks
cp -R decks/lhtml-site .work/lhtml-site
mkdir -p .work/lhtml-site/src/_demo
cp -R .site/demos/. .work/lhtml-site/src/_demo/
if "$LHTML_SITE" doctor 2>/dev/null | grep -q '^PDF export.*: ready'; then
    "$LHTML_SITE" -i .work/lhtml-site/configure.yaml
else
    echo "warning: no PDF export (lhtml-site doctor): the lhtml-site talk has no slides.pdf" >&2
    "$LHTML_SITE" -i .work/lhtml-site/configure.yaml --target html
fi
cp -R .work/lhtml-site/.site .site/decks/lhtml-site
cp -R decks/reveal.js .site/decks/reveal.js

echo "==> done: .site/"
