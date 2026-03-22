#!/usr/bin/env bash
# Run laserdesk_backend after a local CMake build (macOS / Linux).
# Optional: export LASERDESK_CORS_ORIGIN=https://your-user.github.io when using the hosted UI.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/backend/build/laserdesk_backend"
if [[ ! -f "$BIN" ]]; then
	echo "laserdesk_backend not found at:" >&2
	echo "  $BIN" >&2
	echo "Build first from the repo root:" >&2
	echo "  cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release" >&2
	echo "  cmake --build backend/build --parallel" >&2
	exit 1
fi
PORT="${LASERDESK_PORT:-8080}"
exec "$BIN" --port "$PORT" "$@"
