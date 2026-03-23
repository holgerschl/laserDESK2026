# laserDESK 2026 – Implementation Plan

This document turns the project proposal in `docs/laserDESK_2026_Projektplan.html` into a concrete implementation path, aligned with SCANLAB **laserDESK** (1.6.x) user documentation and the **RTC6** manual (Rev. 1.1.3 en-US, software package 1.22.x line).

---

## 1. Product goals (from the Projektplan)

- **Workflow-centric UI**: Show only steps and functions belonging to the active customer workflow; avoid a single overloaded menu surface.
- **Stack**: **Svelte Kit** + TypeScript frontend; **backend** encapsulates laser and RTC logic; communication via **REST** (MVP) with a path to **WebSocket** for live status and logs.
- **RTC6**: Use the **Remote Interface** model (Ethernet board, command telegrams) so the control PC is **not** tied to a Windows-only PCIe driver for the core product vision.
- **Customization**: Structured **workflow definitions** (JSON/YAML or DSL) plus versioned **Cursor prompts** under `docs/prompts/` (per customer folder).
- **References** for patterns and parity discussions: **Autodesk Machine Control Framework** (AMCF) and behaviour of current **laserDESK** where relevant.

Project rules in `AGENTS.md` (when present) apply: e.g. C++17 backend core without Windows-specific calls in the RTC kernel, MVP scope from `docs/requirements/mvp-feature-katalog.md` when that file exists.

---

## 2. Critical distinction: two different “remote” concepts

| Concept | What it is | Implication |
|--------|------------|-------------|
| **laserDESK remote control** | External master drives the **laserDESK application** (job selection, text fields, start, etc.). laserDESK acts as **server**. Transport: **serial or TCP**. Details: separate manual *Definition of the Remote Control Functionality* (not in the standard PDF set alone). | Relevant only if you **integrate with or automate laserDESK 1.6** as a process. |
| **RTC6 Remote Interface** | **RTC6 Ethernet** board feature: **Remote Interface Mode** ON → **command telegrams** load/control jobs or change parameters (speed, laser power, transforms, etc.). Host OS can be **non-Windows**; no platform-specific PCIe driver required for that path. **BIOS-ETH** and **RTC6 software package** must match. | Primary integration path for **laserDESK 2026** as a **new** control stack. |

**Decision for MVP (recommended):** Implement against **RTC6 Remote Interface** and your own HTTP API. Treat legacy laserDESK remote control as a **separate optional track** (only if product requires driving **SLLaserDesk.exe**).

Document this in a one-page **ADR** in `docs/` when the repo grows.

---

## 3. Target architecture

### Backend

- **Language**: C++17 for the RTC and domain core (per `AGENTS.md`).
- **RTC module** (e.g. under `backend/src/rtc/`): client for **Ethernet + Remote Interface**; **no** Windows RTC DLL in the portable core. If a temporary PCIe/DLL bridge is needed for lab bring-up, isolate it behind an interface and do not let it become the long-term design.
- **Domain layer**: Job and execution concepts **inspired** by laserDESK (marking workflow, parameter sets, ordered execution) but **owned** by this product—**do not** block MVP on full `.sld` import or binary format reverse engineering unless explicitly in the MVP catalog.
- **REST API**: `/api/v1/...` (e.g. cpp-httplib as in phase-2 backend notes). Resources might include jobs, run control, hardware/correction **handles**, health, and logs.
- **Mock RTC**: Same interface as the real client; used in **unit tests** and for frontend development (`laserdesk_backend --rtc-demo` or equivalent).

### Frontend

- **Svelte Kit** + TypeScript under **`frontend/`**; **no direct RTC access** from the browser.
- **Workflow runtime**: Load definitions validated against `docs/workflows/workflow-schema.json`; render only allowed steps and components (see `frontend/src/lib/workflow/`, `frontend/static/workflows/`).
- **RTC window**: Route `/rtc` and cross-window messaging via `BroadcastChannel` `laserdesk-rtc-v1` (`frontend/src/lib/laser/rtcChannel.ts`).
- **E2E**: **Playwright** in `frontend/e2e/` against **laserdesk_backend** (mock RTC); optional env to point at real hardware in the lab.

### Repository layout (evolutionary)

- `docs/workflows/` – workflow schema and examples.
- `docs/prompts/<customer>/` – reproducible Cursor prompts.
- `backend/` – C++ server and RTC client.
- `frontend/` or Svelte root – UI (exact layout follows Svelte Kit conventions you choose).

---

## 4. Phased delivery

### Phase A – Grounding and decisions (Projektplan Phase 1)

**Status:** Complete (see artifacts below).

**Objectives**

- Extract **MVP** scope from `docs/requirements/mvp-feature-katalog.md` (create that file if the catalog lives elsewhere).
- Identify **authoritative** RTC6 Remote Interface material (manual appendix **RTC6 Ethernet**, software package deliverables, telegram definitions).
- Plan **version alignment**: RTC6 **firmware**, **BIOS-ETH**, and **host software package** must match; plan **detection and clear errors** on mismatch (same class of requirement as laserDESK 1.6.26.0 ↔ RTC6 firmware 1.22.1).

**Deliverables**

- Short **ADR**: MVP uses RTC6 **Ethernet Remote Interface** only (or explicit exception). → [`docs/adr/0001-rtc6-ethernet-remote-interface.md`](adr/0001-rtc6-ethernet-remote-interface.md)
- **MVP feature catalog** → [`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md)
- **Authoritative Remote Interface source index** → [`docs/rtc/remote-interface-sources.md`](rtc/remote-interface-sources.md)
- **Version alignment and mismatch handling** → [`docs/rtc/version-alignment.md`](rtc/version-alignment.md)
- **API sketch** (OpenAPI): resources, methods, error model. → [`docs/api/openapi-phase-a-sketch.yaml`](api/openapi-phase-a-sketch.yaml)
- **Mock RTC specification**: states, transitions, logical telegram placeholders. → [`docs/rtc/mock-rtc-specification.md`](rtc/mock-rtc-specification.md)

### Phase B – Backend skeleton and mock RTC (start of Projektplan Phase 2)

**Status:** Complete (see [`backend/README.md`](../backend/README.md)).

**Objectives**

- Runnable backend with REST and **injectable** RTC implementation.
- Unit tests for parsing, state machine, and API handlers.

**Deliverables**

- `laserdesk_backend` binary; `laserdesk_tests`; documented `--rtc-demo` using mock. → [`backend/`](../backend/) (CMake: `laserdesk_backend`, `laserdesk_tests`, library `laserdesk_rtc_core`).

### Phase C – Real RTC6 Remote Interface (core of Projektplan Phase 2)

**Status:** Complete — UDP **Command Telegram** client (RAW header §16.10.6), `EthernetRtcClient`, REST `ethernet` connect body, lab checklist, optional env-gated integration test.

**Objectives**

- Connect to **one** lab configuration (board IP, mode, network path).
- **Vertical slice**: connect, minimal job/list control path, start/stop, status and errors (exact commands per RTC6 manual §6 and Ethernet appendix).

**Deliverables**

- Bring-up checklist (network, BIOS-ETH version, firewall). → [`docs/rtc/bring-up-checklist-phase-c.md`](rtc/bring-up-checklist-phase-c.md)
- Optional integration tests behind an environment flag and real hardware. → `LASERDESK_RTC6_HOST` / `LASERDESK_RTC6_PORT` in `backend/tests/test_ethernet_integration.cpp`
- Code: `backend/src/rtc/ethernet_rtc_client.*`, `backend/src/rtc/rif/*` (telegram framing + Asio UDP).

### Phase D – Reference code study (Projektplan Phase 3)

**Status:** Complete — reference note: [`docs/reference-phase-d-amcf.md`](reference-phase-d-amcf.md).

**Objectives**

- Review **AMCF** for structure, error handling, and list-building patterns—not feature-for-feature copying.

**Deliverables**

- Short note in `docs/`: what was adopted, deferred, or rejected. → [`docs/reference-phase-d-amcf.md`](reference-phase-d-amcf.md)

### Phase E – Frontend and workflow model (Projektplan Phase 4)

**Status:** Complete — Svelte Kit app in [`frontend/`](../frontend/), workflow schema + reference JSON in [`docs/workflows/`](workflows/), Playwright in [`frontend/e2e/`](../frontend/e2e/).

**Objectives**

- Svelte Kit app with **workflow-driven** navigation and generic components in `src/lib/`.
- End-to-end slice: choose workflow → configure allowed parameters → trigger run → show status (mock backend first).

**Deliverables**

- At least one reference workflow JSON + matching UI flow; Playwright spec for the slice. → [`docs/workflows/workflow-schema.json`](workflows/workflow-schema.json), [`docs/workflows/reference-minimal-demo.json`](workflows/reference-minimal-demo.json), [`frontend/static/workflows/reference-minimal-demo.json`](../frontend/static/workflows/reference-minimal-demo.json), [`frontend/e2e/workflow-demo.spec.ts`](../frontend/e2e/workflow-demo.spec.ts)

### Phase F – GitHub distribution, Pages, releases, user documentation

**Status:** Implemented in-repo — workflows under [`.github/workflows/`](../.github/workflows/), hosted **Usage** route in the frontend, backend **CORS** for `github.io` clients.

**Objectives**

- Publish the **static frontend** to **GitHub Pages** under the user/org project URL (target: `https://holgerschl.github.io/laserDESK2026/` — repository name **`laserDESK2026`** under [github.com/holgerschl](https://github.com/holgerschl); use a single slash, not `//`).
- Provide **Windows `laserdesk_backend.exe`** and **macOS `laserdesk_backend-macos.tar.gz`** (arm64, Apple Silicon) via **GitHub Releases** (CI build).
- Ship **usage and installation** documentation as part of the deployed site (`/usage`) plus repository `README` links.
- *(Earlier Phase 5 scope)* CI for unit/E2E can be extended in the same workflow folder; not all jobs are required for Pages/releases.

**Deliverables**

| Item | Location / notes |
|------|------------------|
| Pages deploy workflow | [`.github/workflows/deploy-github-pages.yml`](../.github/workflows/deploy-github-pages.yml) — build with `SVELTEKIT_BASE_PATH=/laserDESK2026`, deploy via **GitHub Actions** Pages |
| Frontend CI | [`.github/workflows/frontend-ci.yml`](../.github/workflows/frontend-ci.yml) — `svelte-check` + static build |
| E2E CI | [`.github/workflows/e2e.yml`](../.github/workflows/e2e.yml) — build backend on Ubuntu, **Playwright** against mock RTC |
| Backend CI | [`.github/workflows/backend-ci.yml`](../.github/workflows/backend-ci.yml) — unit tests |
| Backend release workflow | [`.github/workflows/release-backend.yml`](../.github/workflows/release-backend.yml) — **Release** with Windows `.exe` + macOS `laserdesk_backend-macos.tar.gz` (arm64); `v*` tags or `main` / manual |
| User-facing HTML docs | [`frontend/src/routes/usage/+page.svelte`](../frontend/src/routes/usage/+page.svelte) → deployed at `…/laserDESK2026/usage/`; DXF demo [`frontend/src/routes/dxf/+page.svelte`](../frontend/src/routes/dxf/+page.svelte) |
| API base + CORS | [`frontend/src/lib/api/config.ts`](../frontend/src/lib/api/config.ts), [`backend/src/main.cpp`](../backend/src/main.cpp) (`LASERDESK_CORS_ORIGIN`) |

**Maintainer checklist (not automated here)**

1. Create public repo **`holgerschl/laserDESK2026`** (or rename local folder to match), `git remote add origin …`, push `main`.
2. **Settings → Pages → Build and deployment → Source: GitHub Actions** (first deploy may ask to approve the `github-pages` environment). If workflows stay red or the site is 404, see [`docs/github-pages-setup.md`](github-pages-setup.md).
3. Push to `main` (or run **Deploy GitHub Pages** manually) to publish the site.
4. Run **Release backend** once, or push tag `v0.1.0` to create a Release with `laserdesk_backend.exe` and `laserdesk_backend-macos.tar.gz`.
5. Users: download `.exe` or macOS `.tar.gz`, set `LASERDESK_CORS_ORIGIN` to `https://holgerschl.github.io`, open hosted **Usage** page and save API base `http://127.0.0.1:8080/api/v1`.

### Phase G – DXF demo asset and MVP scope (laser job from DXF)

**Status:** **In progress (MVP slice + G.4 mapper done):** ASCII DXF **LINE** parser + REST `/api/v1/jobs/dxf` + mock/ethernet **`load_dxf_job`**. **G.4:** `RtcJobPlan`, `dxf_rif_list_mapper`, optional ethernet list upload behind **`dxf_rif_list_upload`** on `POST /rtc/connect` (`R_DC_CONFIG_LIST` + `R_LC_*` IDs in [`backend/src/rtc/rif/remote_list_commands.hpp`](../backend/src/rtc/rif/remote_list_commands.hpp), sync with SCANLAB package `telegrams.h`). Frontend **`/dxf`** + Playwright **`dxf-demo.spec.ts`**. **Deferred:** richer list semantics (speeds, laser on/off); extend parser beyond LINE as needed.

**Objectives**

- Ship a **reference DXF** for development and demos.
- Extend MVP to cover: **load DXF**, **display** geometry + **entity/job list** UI, **parse → RTC Remote Interface commands**, **run** DXF-derived job (mock first, then ethernet).

**Deliverables**

| Item | Location |
|------|----------|
| Demo DXF (canonical) | [`demo/dxf/SCANLABLogo.dxf`](../demo/dxf/SCANLABLogo.dxf) |
| Static copy for frontend fetch | [`frontend/static/demo/dxf/SCANLABLogo.dxf`](../frontend/static/demo/dxf/SCANLABLogo.dxf) |
| MVP requirements | [`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md) — rows **B-07…B-09**, **F-05…F-07**, **X-03** |

**Implementation notes (for follow-up work)**

- Parser: start with **ASCII DXF** subset needed for `SCANLABLogo.dxf` (LINE, LWPOLYLINE, ARC, CIRCLE, etc. as present); add tests with the demo file.
- RTC mapping: align with **RTC6 software package** list / Remote List Command documentation; keep generation behind `IRtcClient` so mock and ethernet share the same job descriptor.

**Next-step plan (milestones, API sketch, risks):** [`docs/requirements/phase-g-dxf-implementation-plan.md`](requirements/phase-g-dxf-implementation-plan.md)

### Phase H – Vector scene editor (Konva / Fabric.js / similar)

**Status:** **In MVP catalog** ([`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md) §2: **B-10…B-12**, **F-08…F-13**, **X-04…X-05**; promotion note §5). **Implementation** of tasks H.1–H.9 is **in progress** until closed out with tests per catalog §4. **H.4 (partial):** **pan/zoom** and **mm SVG overlay** (fixed `width`/`height` = stage, inner `<g>` `matrix` matching Konva viewport — avoid CSS `transform` on root `<svg>`, which can hide the overlay) are implemented (**F-09**); **laser presets and per-entity laser** are implemented (**F-13** / **B-10** metadata): `scene_v1` carries `laser_groups`, `default_laser_group_id`, per-entity `laser_group_id` and optional `laser` override (`frontend/src/lib/scene/sceneV1.ts`); optional **`entity_label`** for stable job-tree naming; UI: **`EntityLaserPanel.svelte`** (parameters, **editable preset names**, preset CRUD, bulk laser apply when multi-selected), **`SceneJobTree.svelte`** (collapsible rows; **Shift+click** range select); **multi-select** shared logic in **`selection.ts`** (**F-11**). No separate `LaserGroupsPanel` — presets are edited in the parameter column. **Wide** editor shell on `/editor`. Remaining H.4 items include layer panel and polish. Delivers **interactive placement and manipulation** of geometric entities in the browser using a **canvas-oriented library** (decision: **Konva** or **Fabric.js** or equivalent; **MIT license**, bundle impact documented). The **canonical contract** remains a **versioned scene JSON** owned by laserDESK 2026; the library holds transient interaction state only and must **sync to/from** that model.

**Objectives**

- Let operators **author** marking geometry in-app (not only DXF import), aligned with the **workflow** model (dedicated step type or route).
- Reuse the **same backend job pipeline** as Phase G: scene → **`RtcJobPlan`** → mock / Ethernet `IRtcClient`.
- Keep **mock-first** CI: Playwright drives **place → submit → run** on mock RTC.

**Engineering tasks (order)**

| # | Task |
|---|------|
| H.1 | **Choose library** (Konva vs Fabric vs Paper): document in `docs/` (criteria: Svelte integration, transforms, licensing, bundle size); add dependency to `frontend/package.json`. |
| H.2 | Define **`scene` JSON schema** (layers, entities, transforms, IDs) — mirror or extend internal job representation used by DXF path; version field (`schemaVersion`). **Extended:** `laser_groups`, `default_laser_group_id`, per-entity `laser_group_id`, optional entity `laser`, optional **`entity_label`** (stable display name; see **F-11** / `sceneV1.ts`). |
| H.3 | **Svelte integration:** one editor shell component (e.g. `SceneEditor.svelte`) mounting the library in `onMount`, **destroy on teardown**; avoid duplicating scene state in the library—**export/import** from Konva/Fabric to scene JSON on each meaningful edit or via explicit sync. |
| H.4 | **Primitives (vertical slice):** place **line** and **rectangle**; **select**, **move**, **delete**; single **layer**; **pan/zoom** viewport. **Done:** `SceneEditor.svelte` — Konva `Group` (`viewPanX/Y`, `viewZoom`); wheel / Space+drag / middle-mouse pan; **mm SVG**: stage-sized `<svg>` + inner `<g matrix(…)>` matching Konva (not CSS on root `<svg>`); **Reset view**; `stageToWorldMm`; shapes **draggable when selected** (move + transformer). |
| H.5 | **Manipulators:** rotation / scale handles (library features or thin wrapper). **Multi-select (partial):** **Shift+click** range in job list + canvas (`selection.ts`); highlight multiple; transformer only for **single** selection — library-native multi-transform deferred. |
| H.6 | **Undo/redo:** command stack **or** library history API, applied to **canonical scene model** so serialization is stable. |
| H.7 | **Backend:** `POST` (or `PUT`) **scene job** — validate schema, map to **`RtcJobPlan`** (extend mapper alongside DXF); same start/stop as existing jobs. |
| H.8 | **Workflow:** extend [`docs/workflows/workflow-schema.json`](workflows/workflow-schema.json) with an optional step kind (e.g. `geometry-editor`) + reference workflow JSON under `frontend/static/workflows/`. |
| H.9 | **Tests:** unit tests for schema validation + mapper; **Playwright** E2E **editor → mock run** (extends DXF E2E pattern). |

**Deliverables**

| Item | Location / notes |
|------|------------------|
| Feature catalog rows | [`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md) — §2 **MVP** (**B-10…B-12**, **F-08…F-13**, **X-04…X-05**); §5 promotion note |
| Scene schema | TypeScript types + `buildSceneV1()` in [`frontend/src/lib/scene/sceneV1.ts`](../frontend/src/lib/scene/sceneV1.ts); optional JSON Schema / OpenAPI later |
| Editor UI | `SceneEditor.svelte` (Konva + pan/zoom + mm overlay); `EntityLaserPanel.svelte`, `SceneJobTree.svelte`, `scene/selection.ts`; route [`frontend/src/routes/editor/`](../frontend/src/routes/editor/) |
| API | `backend` — scene job endpoint + validation; mapper next to existing DXF → RIF path |

**Out of scope for Phase H (initial slice)**

- **laserDESK `.sld`** import/export.
- **Full** laserDESK 1.6 CAD parity (constraints, advanced trim, 3D, vision).
- Replacing Phase G DXF flow — **both** DXF import and editor feed the same scene/job abstraction where practical.

---

## 5. Dependencies and prerequisites

- **Hardware**: RTC6 **Ethernet** board on the lab network (or an approved simulator if available).
- **Software**: Matching **RTC6 software package** and **BIOS-ETH** for that package.
- **Optional**: laserDESK 1.6 + dongle only for **behavioural comparison** or migration scenarios—not required for greenfield Remote Interface MVP.
- **Legal**: SCANLAB PDFs and manuals are **SCANLAB copyright**; do not redistribute full manuals inside the repo; link to official downloads where appropriate.

---

## 6. Risks and mitigations

| Risk | Mitigation |
|------|------------|
| Remote Interface spec is large and cross-referenced | Time-box a **spike**; implement the **smallest** vertical path; keep **mock** aligned with the same interfaces. |
| Version skew (firmware, BIOS-ETH, DLL/package) | **Query and display** versions at connect; fail fast with actionable messages; document supported combinations. |
| Scope creep (“must match laserDESK 1.6 everywhere”) | Enforce **MVP catalog**; defer `.sld` import, **full** laserDESK 1.6 CAD parity, and Premium-only features unless listed. **Phase H** (canvas editor) is **in** the MVP catalog §2; **full** product parity remains out. |
| Confusion between laserDESK remote and RTC6 Remote Interface | ADR + this section; separate integration tracks if both are ever needed. |
| DXF scope / RTC list mapping underestimated | Follow [`phase-g-dxf-implementation-plan.md`](requirements/phase-g-dxf-implementation-plan.md) milestones; LINE-only first; time-box **B-08** spike on package examples. |

---

## 7. MVP success criteria

- Backend controls **RTC6 over Ethernet** in **Remote Interface Mode** for a **defined minimal process**, with **automated tests** against **mock** RTC.
- Frontend completes **at least one workflow** end-to-end against that API.
- **Playwright** passes against mock backend in CI (see `.github/workflows/e2e.yml`).
- Core backend code remains **portable** (no mandatory Windows RTC DLL in the long-term path).
- **Phase G:** Demo **DXF** loaded and displayed with **entity list** + preview; parsed output drives **mock** execution; **Ethernet** can optionally stream a **G.4** list image when connecting with `dxf_rif_list_upload: true` — see **Phase G** status above and [`phase-g-dxf-implementation-plan.md`](requirements/phase-g-dxf-implementation-plan.md).
- **Phase H:** **Vector scene editor** (Konva/Fabric per §4 Phase H) — place/manipulate geometry, submit scene job, run on mock; **pan/zoom** per **F-09** / H.4; **laser presets / per-entity laser** per **F-13**; **multi-select** per **F-11**; **X-04** / **X-05** when implemented.

---

## 8. Related files in this repository

These live in **`docs/`** (same folder as this file for the HTML/PDFs):

- `docs/laserDESK_2026_Projektplan.html` – original proposal and links.
- `docs/laserDESK_Installation_and_FirstSteps.pdf`, `docs/laserDESK_QuickGuide.pdf` – current laserDESK product context.
- `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` – RTC6 installation, programming, commands, Ethernet appendix.
- `docs/workflows/workflow-schema.json`, `docs/workflows/reference-minimal-demo.json` – workflow model (Phase E).
- `frontend/` – Svelte Kit app and Playwright specs (Phase E).
- `.github/workflows/` – Pages deploy, Windows backend release, backend CI (Phase F).
- `demo/dxf/SCANLABLogo.dxf` – Phase G reference DXF (copy also under `frontend/static/demo/dxf/`).
- `docs/requirements/phase-g-dxf-implementation-plan.md` – Phase G engineering breakdown.
- `backend/src/dxf/`, `frontend/src/routes/dxf/+page.svelte`, `frontend/e2e/dxf-demo.spec.ts` – Phase G implementation (LINE parser, REST, UI, E2E).
- **Phase H** – vector scene editor (Konva/Fabric/etc.): see **§4 Phase H** above; MVP rows in [`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md) **§2**, promotion note **§5**; editor [`SceneEditor.svelte`](../frontend/src/lib/components/SceneEditor.svelte), [`EntityLaserPanel.svelte`](../frontend/src/lib/components/EntityLaserPanel.svelte), [`SceneJobTree.svelte`](../frontend/src/lib/components/SceneJobTree.svelte), [`selection.ts`](../frontend/src/lib/scene/selection.ts), [`sceneV1.ts`](../frontend/src/lib/scene/sceneV1.ts), route [`frontend/src/routes/editor/`](../frontend/src/routes/editor/).

---

*Last aligned with the Projektplan and the documentation set available in this repo; revise phases when MVP requirements or SCANLAB package versions change.*
