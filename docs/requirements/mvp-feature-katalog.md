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

### Cross-cutting

| ID | Feature | Notes |
|----|---------|--------|
| X-01 | **Playwright** E2E: one path against **mock** backend | Required by project rules |
| X-02 | **AGENTS.md** / Cursor rules | Already or separately maintained; must reference this catalog |
| X-03 | **Tests for DXF path** | Unit tests for parser / mapper; E2E slice: load demo DXF → list visible → run on mock (extends X-01) |

**Reference demo asset (Phase G):** `demo/dxf/SCANLABLogo.dxf` (copy of SCANLAB logo DXF from internal `ui/DemoFiles`; same bytes under `frontend/static/demo/dxf/` for static hosting).

---

## 3. Explicitly out of MVP

- Import or round-trip **laserDESK `.sld`** job files.
- Full **geometry editor** parity with laserDESK 1.6 (CAD authoring beyond DXF **load/display/run** above remains out).
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

*Version: 1.1 · Phase G catalog extension · March 2026*
