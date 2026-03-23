# MVP-Feature-Katalog – laserDESK 2026

Bounded scope for the first shippable increment. Anything not listed here is **out of MVP** unless explicitly promoted via change to this document.

**Sources:** `docs/laserDESK_2026_Projektplan.html`, `docs/implementation-plan.md`, laserDESK 1.6 Quick Guide (conceptual alignment only), RTC6 manual (integration path).

---

## 1. MVP principles

- **Workflow-centric UI** (frontend): Only steps and controls defined by the active workflow are shown.
- **RTC6 via Ethernet Remote Interface** (backend): No dependency on Windows PCIe RTC DLL for the **portable** MVP path; see ADR 0001.
- **Mock-first**: Full behaviour against a **mock RTC** before lab hardware is mandatory for CI and demos.

---

## 2. In scope (MVP)

### Backend

| ID | Feature | Notes |
|----|---------|--------|
| B-01 | HTTP **health** and API discovery | e.g. `GET /api/v1/health` |
| B-02 | **RTC connection** lifecycle (mock + real stub) | connect / disconnect, target host config |
| B-03 | **Version / capability reporting** | Surface package/firmware/BIOS-ETH identity when available; fail with clear message on known mismatch (see `docs/rtc/version-alignment.md`) |
| B-04 | **Minimal execution slice** | Load a minimal “job” or list metaphor, start, stop, query run state (exact mapping to RTC6 telegrams in Phase C) |
| B-05 | **Structured errors** | Stable error codes + human-readable messages for API and RTC layer |
| B-06 | **Unit tests** for RTC abstraction and API handlers | Against mock only in MVP CI |
| B-07 | **DXF job ingestion** | Accept user `.dxf` upload and/or reference demo file [`demo/dxf/SCANLABLogo.dxf`](../../demo/dxf/SCANLABLogo.dxf); parse to an internal vector / job representation (entities + metadata) |
| B-08 | **DXF → RTC Remote Interface command generation** | Map parsed geometry to RTC6 **Remote Interface** list / command sequence (per SCANLAB package: `telegrams.h`, list buffer semantics, Remote List Commands as required); **mock** RTC accepts the same logical job shape for CI |
| B-09 | **Run DXF-derived laser job** | REST (or workflow-driven API) to arm, start, and stop execution of the DXF-derived job on mock and ethernet `IRtcClient` implementations |
| B-10 | **Scene JSON schema + validation** | Versioned `schemaVersion`; layers, entities (IDs, type, geometry, transform); **laser metadata:** `laser_groups[]` (id, name, `laser` preset), `default_laser_group_id`, per-entity `laser_group_id`, optional per-entity `laser` override. Backend geometry path ignores laser fields; stored for UI / future RTC list segmentation. |
| B-11 | **REST: submit validated scene job** | e.g. `POST /api/v1/jobs/scene` (exact path in OpenAPI when implemented); reject invalid schema with structured errors (B-05) |
| B-12 | **Scene → `RtcJobPlan` / RIF mapping** | Extend or share code with DXF → list path; mock RTC accepts same execution shape for CI |

### Frontend

| ID | Feature | Notes |
|----|---------|--------|
| F-01 | Load **one reference workflow** from JSON validated against schema | Schema: [`docs/workflows/workflow-schema.json`](../workflows/workflow-schema.json); served copy under `frontend/static/workflows/` |
| F-02 | **Workflow-driven** navigation | Only allowed steps visible |
| F-03 | **RTC status** view (read-only in MVP) | Backed by backend; can live on `/rtc` or embedded panel |
| F-04 | Trigger **demo run** (against mock backend) | End-to-end button → API → mock RTC state change |
| F-05 | **Load a DXF file** | File picker; optional quick-load of bundled demo [`frontend/static/demo/dxf/SCANLABLogo.dxf`](../../frontend/static/demo/dxf/SCANLABLogo.dxf) (mirror of repo `demo/dxf/`) |
| F-06 | **Display DXF + entity / job item window** | Visualise geometry (e.g. 2D paths) and show a list panel of entities / job items mapped from the parser |
| F-07 | **DXF laser job flow** | Workflow steps (or equivalent) to parse via backend, review entity list, then **run** the DXF-derived job through the same RTC stack as B-09 |
| F-08 | **Canvas library integration** | Konva *or* Fabric.js (or equivalent); document choice in `docs/`; Svelte mount/teardown pattern |
| F-09 | **Editor shell** | **Pan/zoom (done):** wheel, **Space + drag** / middle-mouse pan, **Reset view**; Konva `Group` viewport. **mm SVG overlay:** explicit `width`/`height` = stage (px), same affine as Konva on an inner `<g>` (`matrix(zoom,0,0,zoom,panX,panY)` in user space — **no** CSS `transform` on the root `<svg>`, which can blank the overlay in some browsers). **Still open:** layer panel, workflow step kind (optional). **Route:** `/editor` — `SceneEditor.svelte`. |
| F-10 | **Place primitives** | Initial slice: at least **line** and **rectangle**; extend to polyline/arc in later promotions |
| F-11 | **Select / move / transform** | Selection, drag, scale/rotate using library capabilities; delete |
| F-12 | **Undo / redo** | Stable against exported scene model (command stack or library history + sync) |
| F-13 | **Laser groups & per-entity laser** | **Groups:** named presets (`LaserGroupV1`) editable in **Laser groups** panel; **default for new shapes** selector. **Grouping:** assign the same `laser_group_id` on multiple entities (job tree dropdown or **Selected entity**). **Per-entity:** optional **custom laser** (full override). Scene JSON built with `buildSceneV1()` in `frontend/src/lib/scene/sceneV1.ts`; components: `LaserGroupsPanel.svelte`, `EntityLaserPanel.svelte`, `SceneJobTree` group column. |

### Cross-cutting

| ID | Feature | Notes |
|----|---------|--------|
| X-01 | **Playwright** E2E: one path against **mock** backend | Required by project rules |
| X-02 | **AGENTS.md** / Cursor rules | Already or separately maintained; must reference this catalog |
| X-03 | **Tests for DXF path** | Unit tests for parser / mapper; E2E slice: load demo DXF → list visible → run on mock (extends X-01) |
| X-04 | **Playwright E2E: editor → mock run** | Place or load minimal scene → submit → run on mock backend (extends E2E strategy) |
| X-05 | **Tests for scene path** | Unit: schema validation, scene→plan mapper; optional round-trip JSON fixtures |

**Reference demo asset (Phase G):** `demo/dxf/SCANLABLogo.dxf` (copy of SCANLAB logo DXF from internal `ui/DemoFiles`; same bytes under `frontend/static/demo/dxf/` for static hosting).

**Implementation plan (next step):** [`phase-g-dxf-implementation-plan.md`](phase-g-dxf-implementation-plan.md). **Phase H (vector editor):** [`docs/implementation-plan.md`](../implementation-plan.md) §4 Phase H (tasks H.1–H.9).

---

## 3. Explicitly out of MVP

- Import or round-trip **laserDESK `.sld`** job files.
- **Full** geometry / CAD **parity** with laserDESK 1.6 (every tool and command). **Incremental** interactive editor is **in MVP** (B-10…X-05); wholesale parity remains out.
- **laserDESK application remote control** (serial/TCP server to `SLLaserDesk.exe`); separate product track.
- **PCIe** RTC6 board as the primary supported path (optional lab-only bridge only, behind interface).
- **Premium-only** laserDESK concepts (3D, vision, full tiling) unless a workflow-only stub is needed for demos.
- **Multi-board** / master-slave RTC topologies.
- **WebSocket** streaming (REST-only MVP; WebSocket may follow immediately after MVP).

---

## 4. Promotion criteria (into MVP)

A feature moves from “out” to “in” when:

1. This file is updated with a new row and version date.
2. `docs/implementation-plan.md` or release notes reference the change.
3. Tests (unit and/or E2E) cover the new behaviour.

---

## 5. Phase H — catalog promotion

Phase H (**vector scene editor**, Konva / Fabric.js / similar) was **promoted into §2 (MVP)** in catalog **v1.3** per §4. Feature IDs: **B-10…B-12**, **F-08…F-12**, **X-04…X-05**; **F-13** added in **v1.5** (laser groups / per-entity laser). Engineering tasks **H.1–H.9** and deliverables: [`docs/implementation-plan.md`](../implementation-plan.md) **§4 Phase H**.

**Approach (unchanged):** Browser **canvas library** (MIT license); **canonical** data is **versioned scene JSON** in laserDESK; backend uses the same **`RtcJobPlan`** / Remote Interface mapping as the DXF path.

**F-09 pan/zoom (March 2026):** Documented as implemented in **F-09** row and [`docs/implementation-plan.md`](../implementation-plan.md) §4 Phase H (H.4 progress note).

**F-13 laser groups (March 2026):** Per-entity / grouped laser parameters in scene JSON and editor UI — see **F-13** row and implementation plan §4 Phase H.

**F-09 / F-11 viewport & drag (March 2026):** mm overlay uses **fixed px** SVG size + inner `<g>` `matrix` (same as Konva); select-mode **move** for selected entities (see **F-09** / **F-11** and implementation plan §4 Phase H).

---

*Version: 1.6.1 · F-09 SVG overlay: inner-g matrix, not CSS transform on root svg · March 2026*
