# laserDESK 2026 – Agent rules (Cursor)

## Backend (C++)

- **Language:** C++17. Keep the **RTC portable core** free of Windows-only APIs; isolate any future PCIe/DLL bridge behind `IRtcClient`.
- **RTC module:** `backend/src/rtc/` — `mock_rtc_client`, `ethernet_rtc_client`, shared `rif/` (UDP + TGM RAW framing per RTC6 §16.10).
- **HTTP API:** REST under `/api/v1/…` as in `docs/api/openapi-phase-a-sketch.yaml`. Server entry: `backend/src/main.cpp`; routes: `backend/src/http/api_router.cpp`.
- **Build:** `backend/README.md` – CMake, targets `laserdesk_backend`, `laserdesk_tests`.
- **Tests:** `laserdesk_tests` (GoogleTest). Set `LASERDESK_RTC6_HOST` (optional `LASERDESK_RTC6_PORT`) to run the Ethernet integration test; otherwise it skips.

## Frontend (Svelte Kit)

- **Root:** `frontend/` — TypeScript, Svelte 5, workflow definitions validated against `docs/workflows/workflow-schema.json` (served copy under `frontend/static/workflows/`).
- **Workflow:** Only steps from the active workflow JSON are shown; generic building blocks live in `frontend/src/lib/`.
- **API:** Browser calls same-origin `/api/v1/…`; Vite dev proxy uses `LASERDESK_BACKEND_URL` (default `http://127.0.0.1:8080`).
- **RTC route:** `/rtc` — monitor health/status and **BroadcastChannel** `laserdesk-rtc-v1` (`frontend/src/lib/laser/rtcChannel.ts`).
- **Scene editor:** `/editor` — Konva canvas (`SceneEditor.svelte`), job tree + **parameters** beside canvas (wide shell); laser **presets** with editable names and **preset parameters** (power, speed, etc.) in **`EntityLaserPanel`** without requiring a selection; optional **`entity_label`** for stable tree names; optional **`job_group_id`** for nested **job tree** groups — **↑/↓** on a **row** moves **one entity** (with `sanitizeFragmentedJobGroups` in `sceneV1.ts`); **↑/↓** on the **folder** when the **full group** is selected moves the **whole block**; **Shift+click** multi-select (`scene/selection.ts`); multi-select **move** and **transform** (rotate/resize) as a group on the canvas; **Line** / **Rect** tools: **drag** to place with **dashed preview**.
- **E2E:** Playwright in `frontend/e2e/`; `npm run test:e2e` starts backend + dev server (see `frontend/README.md`).
- **Phase F (distribution):** Static build + `paths.base` for GitHub project Pages; `LASERDESK_CORS_ORIGIN` on backend for `github.io`; workflows in `.github/workflows/` (Pages, Release backend, backend CI).

## Scope

- MVP features: `docs/requirements/mvp-feature-katalog.md` (includes Phase G: DXF → RTC job; demo `demo/dxf/SCANLABLogo.dxf`).
- Implementation phases: `docs/implementation-plan.md`.
