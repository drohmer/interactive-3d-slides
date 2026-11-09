#!/usr/bin/env bash
set -euo pipefail

# === The tutorial "Create slides with interactive 3D": the whole site ===
# Usage: ./build.sh     (needs lhtml-site; the demos are the builds committed in ../examples/*/web)
#   .site/              the English pages, and sitemap.xml (scripts/seo.py)
#   .site/fr/           the French pages
#   .site/demos/<id>/   the live demos (a1, a2, c1, c3, c6, c7) and slide-controls.js
#   .site/decks/lhtml-site/   an example talk made with lhtml-site (html/, slides.pdf)
#   .site/decks/reveal.js/    the same demos in a reveal.js talk
#   .site/code/               the files of the repository (to see, to download, the slides of the examples live) and zips
# Preview: python3 -m http.server -d <a folder holding interactive-3d-slides -> .site> and open
# http://localhost:8000/interactive-3d-slides/ (the pages are made for that path).

cd "$(dirname "$0")"
LHTML_SITE="${LHTML_SITE:-lhtml-site}"

rm -rf .site .site-fr .work
echo "==> the code shown in the pages (src/_code/, from the files of the repository)"
python3 scripts/prepare_code.py fragments
echo "==> pages, English and French"
"$LHTML_SITE" -i configure.yaml
"$LHTML_SITE" -i configure.fr.yaml
cp -R .site-fr .site/fr
echo "==> canonical, hreflang, Open Graph and sitemap.xml"
python3 scripts/seo.py .site

echo "==> the code to download, and the slides of the examples, live (.site/code/)"
python3 scripts/prepare_code.py publish .site

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

echo "==> the lhtml-site slide of each example, live (.site/code/examples/<id>/lhtml-site/html/)"
for deck in ../examples/*/lhtml-site; do
    example=$(dirname "$deck")
    id=$(basename "$example")
    rm -rf ".work/$id"; mkdir -p ".work/$id"
    cp -R "$deck" ".work/$id/lhtml-site"
    mkdir -p ".work/$id/lhtml-site/src/_demo"
    cp -R "$example/web/." "$example/poster.png" ../bridge/slide-controls.js ".work/$id/lhtml-site/src/_demo/"
    "$LHTML_SITE" -i ".work/$id/lhtml-site/configure.yaml" --target html >/dev/null
    rm -rf ".site/code/examples/$id/lhtml-site/html"
    cp -R ".work/$id/lhtml-site/.site/html" ".site/code/examples/$id/lhtml-site/html"
    echo "    $id"
done

echo "==> example talks"
TALK_DEMOS="a1 a2 c3 c6 c7"                        # the demos used by the two talks
mkdir -p .work .site/decks
cp -R decks/lhtml-site .work/talk-lhtml-site
mkdir -p .work/talk-lhtml-site/src/_demo
for d in $TALK_DEMOS; do cp -R ".site/demos/$d" .work/talk-lhtml-site/src/_demo/; done
cp .site/demos/slide-controls.js .work/talk-lhtml-site/src/_demo/
if "$LHTML_SITE" doctor 2>/dev/null | grep -q '^PDF export.*: ready'; then
    "$LHTML_SITE" -i .work/talk-lhtml-site/configure.yaml
else
    echo "warning: no PDF export (lhtml-site doctor): the lhtml-site talk has no slides.pdf" >&2
    "$LHTML_SITE" -i .work/talk-lhtml-site/configure.yaml --target html
fi
cp -R .work/talk-lhtml-site/.site .site/decks/lhtml-site
cp -R decks/reveal.js .site/decks/reveal.js
mkdir -p .site/decks/reveal.js/demos                # the talk is self-contained: its models next to it
for d in $TALK_DEMOS; do cp -R ".site/demos/$d" .site/decks/reveal.js/demos/; done
cp .site/demos/slide-controls.js .site/decks/reveal.js/demos/

echo "==> the two talks to download (.site/code/talk-*.zip)"
rm -f .site/code/talk-reveal.zip .site/code/talk-lhtml-site.zip
# reveal.js: the talk as it is shown (README.md, index.html, demos/), in a folder talk-reveal/
rm -rf .work/talk-reveal && cp -R .site/decks/reveal.js .work/talk-reveal
(cd .work && zip -qr ../.site/code/talk-reveal.zip talk-reveal -x '*.DS_Store')
# lhtml-site: the sources of the talk, with the demos in src/_demo/ (build it with lhtml-site)
(cd .work && zip -qr ../.site/code/talk-lhtml-site.zip talk-lhtml-site -x 'talk-lhtml-site/.site/*' \
    'talk-lhtml-site/.structure/*' 'talk-lhtml-site/.verification/*' 'talk-lhtml-site/.layout/*' '*.DS_Store')
ls -l .site/code/talk-*.zip | awk '{print "    " $5 " bytes  " $9}'

echo "==> done: .site/"
