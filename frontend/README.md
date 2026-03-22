# laserDESK 2026 – frontend

Svelte Kit (TypeScript, Svelte 5) UI: **workflow-driven** steps, REST to `laserdesk_backend`, and a separate **`/rtc`** view with `BroadcastChannel` logging.

## Prerequisites

- Node 20+ recommended
- Backend on HTTP (default `http://127.0.0.1:8080`) — see [`../backend/README.md`](../backend/README.md)

## Development

```bash
npm install
# Terminal 1: backend (mock RTC is fine)
#   ../backend/build/Release/laserdesk_backend.exe --port 8080
# Terminal 2:
npm run dev
```

The dev server proxies **`/api/*`** to the backend. Override target:

```bash
set LASERDESK_BACKEND_URL=http://127.0.0.1:9090
npm run dev
```

## Workflows

- **Schema:** [`../docs/workflows/workflow-schema.json`](../docs/workflows/workflow-schema.json)
- **Reference workflow:** `static/workflows/reference-minimal-demo.json` (keep in sync with the copy under `docs/workflows/` when editing)

## Production static build (GitHub Pages)

The [deploy workflow](../.github/workflows/deploy-github-pages.yml) sets **`SVELTEKIT_BASE_PATH=/laserDESK2026`** so assets and routes match a **project site** at `https://<user>.github.io/laserDESK2026/`.

Local check (PowerShell):

```powershell
$env:SVELTEKIT_BASE_PATH = "/laserDESK2026"
npm run build
```

Output is `frontend/build/`. Hosted users must point the UI at their local API (see **`/usage`** route and `src/lib/api/config.ts`); enable backend **`LASERDESK_CORS_ORIGIN`** for your `github.io` origin.

## Quality

```bash
npm run check
```

## E2E (Playwright)

Playwright starts **two** servers: `laserdesk_backend` on port **18080** (via `scripts/e2e-backend.mjs`) and Vite on **5173** with `LASERDESK_BACKEND_URL` pointing at 18080.

1. Build the C++ backend (CMake) so `laserdesk_backend` exists under `../backend/build/...`.
2. Optionally set `LASERDESK_BACKEND_EXE` to the full path of the binary.
3. Install browsers once: `npx playwright install chromium`
4. Run: `npm run test:e2e`

```bash
npm run test:e2e
npm run test:e2e:ui
```

If the backend binary is missing, `e2e-backend.mjs` exits with an error before tests run.
