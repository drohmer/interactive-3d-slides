#!/usr/bin/env bash
set -euo pipefail

# === The tutorial "Create slides with interactive 3D": deploy ===
# Usage: ./deploy.sh            build (build.sh), then upload .site/ (with fr/, demos/, decks/)
#        ./deploy.sh --dry-run  build, then show what would be uploaded or deleted, without changing anything
#   .site/ -> damien-scaleway:/var/www/scholartools.fr/interactive-3d-slides/
# The server needs the nginx blocks of ../ops/nginx/interactive-3d-slides.conf (once).

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REMOTE_DIR="/var/www/scholartools.fr/interactive-3d-slides"
UPLOAD_HOST="damien-scaleway"
DRY_RUN=""
if [ "${1:-}" = "--dry-run" ]; then DRY_RUN="--dry-run"; fi

cd "$SCRIPT_DIR"
./build.sh

echo "==> Uploading to ${UPLOAD_HOST}:${REMOTE_DIR}/ ${DRY_RUN}"
if [ -z "$DRY_RUN" ]; then
    ssh "$UPLOAD_HOST" "mkdir -p '$REMOTE_DIR'"
fi
rsync -avz --delete $DRY_RUN \
    -e "ssh" \
    --exclude='.DS_Store' \
    .site/ \
    "${UPLOAD_HOST}:${REMOTE_DIR}/"

echo ""
if [ -n "$DRY_RUN" ]; then
    echo "=== Dry run: nothing was uploaded ==="
else
    echo "=== Deploy complete ==="
    echo "https://scholartools.fr/interactive-3d-slides/"
    echo "https://scholartools.fr/interactive-3d-slides/fr/"
fi
