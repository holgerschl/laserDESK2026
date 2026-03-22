# laserDESK 2026 – Implementation Plan

This document turns the project proposal in `docs/laserDESK_2026_Projektplan.html` into a concrete implementation path, aligned with SCANLAB **laserDESK** (1.6.x) user documentation and the **RTC6** manual (Rev. 1.1.3 en-US, software package 1.22.x line).

---

## 1. Product goals (from the Projektplan)

- **Workflow-centric UI**: Show only steps and functions belonging to the active customer workflow; avoid a single overloaded menu surface.
- **Stack**: **Svelte Kit** + TypeScript frontend; **backend** encapsulates laser and RTC logic; communication via **REST** (MVP) with a path to **WebSocket** for live status and logs.
- **RTC6**: Use the **Remote Interface** model (Ethernet board, command telegrams) so the control PC is **not** tied to a Windows-only PCIe driver for the core product vision.
- **Customization**: Structured **workflow definitions** (JSON/YAML or DSL) plus versioned **Cursor prompts** under `docs/prompts/` (per customer folder).
- **References** for patterns and parity discussions: **Sirius3** (Spirallab), **Autodesk Machine Control Framework** (AMCF), and behaviour of current **laserDESK** where relevant.

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

**Status:** Complete — reference note: [`docs/reference-phase-d-sirius3-amcf.md`](reference-phase-d-sirius3-amcf.md).

**Objectives**

- Review **Sirius3** and **AMCF** for structure, error handling, and list-building patterns—not feature-for-feature copying.

**Deliverables**

- Short note in `docs/`: what was adopted, deferred, or rejected. → [`docs/reference-phase-d-sirius3-amcf.md`](reference-phase-d-sirius3-amcf.md)

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
- Provide a **Windows `laserdesk_backend.exe`** via **GitHub Releases** (CI build).
- Ship **usage and installation** documentation as part of the deployed site (`/usage`) plus repository `README` links.
- *(Earlier Phase 5 scope)* CI for unit/E2E can be extended in the same workflow folder; not all jobs are required for Pages/releases.

**Deliverables**

| Item | Location / notes |
|------|------------------|
| Pages deploy workflow | [`.github/workflows/deploy-github-pages.yml`](../.github/workflows/deploy-github-pages.yml) — build with `SVELTEKIT_BASE_PATH=/laserDESK2026`, deploy via **GitHub Actions** Pages |
| Frontend CI | [`.github/workflows/frontend-ci.yml`](../.github/workflows/frontend-ci.yml) — `svelte-check` + static build |
| E2E CI | [`.github/workflows/e2e.yml`](../.github/workflows/e2e.yml) — build backend on Ubuntu, **Playwright** against mock RTC |
| Backend CI | [`.github/workflows/backend-ci.yml`](../.github/workflows/backend-ci.yml) — unit tests |
| Windows release workflow | [`.github/workflows/release-backend-windows.yml`](../.github/workflows/release-backend-windows.yml) — artifact on manual run; **Release + `.exe`** on `v*` tags |
| User-facing HTML docs | [`frontend/src/routes/usage/+page.svelte`](../frontend/src/routes/usage/+page.svelte) → deployed at `…/laserDESK2026/usage/` |
| API base + CORS | [`frontend/src/lib/api/config.ts`](../frontend/src/lib/api/config.ts), [`backend/src/main.cpp`](../backend/src/main.cpp) (`LASERDESK_CORS_ORIGIN`) |

**Maintainer checklist (not automated here)**

1. Create public repo **`holgerschl/laserDESK2026`** (or rename local folder to match), `git remote add origin …`, push `main`.
2. **Settings → Pages → Build and deployment → Source: GitHub Actions** (first deploy may ask to approve the `github-pages` environment). If workflows stay red or the site is 404, see [`docs/github-pages-setup.md`](github-pages-setup.md).
3. Push to `main` (or run **Deploy GitHub Pages** manually) to publish the site.
4. Run **Release Windows backend** once, or push tag `v0.1.0` to create a Release with `laserdesk_backend.exe`.
5. Users: download `.exe`, set `LASERDESK_CORS_ORIGIN` to `https://holgerschl.github.io`, open hosted **Usage** page and save API base `http://127.0.0.1:8080/api/v1`.

### Phase G – DXF demo asset and MVP scope (laser job from DXF)

**Status:** Catalog + demo file in repo — **implementation** of B-07…B-09, F-05…F-07, X-03 is **not** done in this increment (tracked in [`docs/requirements/mvp-feature-katalog.md`](requirements/mvp-feature-katalog.md)).

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
| Scope creep (“must match laserDESK 1.6 everywhere”) | Enforce **MVP catalog**; defer `.sld` import, full geometry editor, and Premium-only features unless listed. |
| Confusion between laserDESK remote and RTC6 Remote Interface | ADR + this section; separate integration tracks if both are ever needed. |
| DXF scope / RTC list mapping underestimated | Follow [`phase-g-dxf-implementation-plan.md`](requirements/phase-g-dxf-implementation-plan.md) milestones; LINE-only first; time-box **B-08** spike on package examples. |

---

## 7. MVP success criteria

- Backend controls **RTC6 over Ethernet** in **Remote Interface Mode** for a **defined minimal process**, with **automated tests** against **mock** RTC.
- Frontend completes **at least one workflow** end-to-end against that API.
- **Playwright** passes against mock backend in CI (see `.github/workflows/e2e.yml`).
- Core backend code remains **portable** (no mandatory Windows RTC DLL in the long-term path).
- **Phase G (when implemented):** Demo **DXF** loaded and displayed with **entity/job list**; parsed output drives **mock** (then ethernet) execution per catalog **B-07…B-09**, **F-05…F-07**, **X-03**.

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

---

*Last aligned with the Projektplan and the documentation set available in this repo; revise phases when MVP requirements or SCANLAB package versions change.*
