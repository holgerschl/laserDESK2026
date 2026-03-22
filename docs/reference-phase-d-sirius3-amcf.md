# Phase D – Sirius3 & AMCF reference study

**Purpose:** Record what was taken from public **Sirius3** and **Autodesk Machine Control Framework (AMCF)** repositories versus what laserDESK 2026 **defers** or **rejects**, per the implementation plan (Projektplan Phase 3). This is a pattern and architecture note, not a license to copy proprietary NuGet binaries or vendor SDKs.

**Sources reviewed (public):**

- [Sirius3](https://github.com/labspiral/sirius3) (Spirallab) — README, repo layout (`demos/`, `doc/`, Visual Studio solution).
- [AMCF](https://github.com/Autodesk/AutodeskMachineControlFramework) — README, top-level layout, `Drivers/ScanLab/` (Headers, Interfaces, Implementation, ACT, Resources).

Authoritative RTC behaviour remains **SCANLAB RTC6 manual + software package**; see [`docs/rtc/remote-interface-sources.md`](rtc/remote-interface-sources.md).

---

## Sirius3

**What it is:** A **.NET** (net481 / net8–10–windows) laser-marking platform built on **NuGet packages** (`SpiralLab.Sirius3`, `.Dependencies`, `.UI`) that wrap **SCANLAB RTC** via installed **Windows runtimes/DLLs**, plus OpenTK, logging, and JSON.

**Useful ideas (conceptual):**

- **Device composition:** Factories for scanner, laser, powermeter, marker; attach to a single editor host (`SiriusEditorControl`). Aligns with our **injectable `IRtcClient`** (mock vs Ethernet) and keeping laser/RTC logic out of the UI layer.
- **Lifecycle:** `Core.Initialize` / `Cleanup`, device `Initialize`, marker `Ready(document, view, …)` — a clear staged setup we can mirror in REST (connect → configure → arm/run) without copying APIs.
- **Document / entity model:** Rich geometry and layer concepts — **inspiration only** for future job/domain modelling; MVP does not import Sirius formats.

**Deferred / not portable here:**

- **Direct RTC DLL usage** and **WinForms/OpenTK** stack — conflicts with **C++17 + Ethernet Remote Interface** and Linux portability goals in `AGENTS.md`.
- **Commercial NuGet licensing** — not a drop-in dependency for this repo.

**Rejected for core path:**

- Treating Sirius3 as the **RTC transport** or **telegram** reference — it is PCIe/driver-centric on Windows; our wire contract is **UDP command telegrams** (Phase C).

---

## AMCF

**What it is:** An **open** machine-control stack (C++ plugins, Go server, web client, CMake build scripts) with many **drivers** under `Drivers/`, including **`Drivers/ScanLab`** (plus ScanLabOIE, ScanLabSMC variants).

**Useful ideas (conceptual):**

- **Driver packaging:** Separate **Interfaces**, **Implementation**, **Headers**, **ACT** (machine-definition assets), **Resources** — a pattern for isolating vendor-specific code behind stable seams.
- **Simulation and testing:** README highlights **simulation drivers** and a **transactional / replayable** timeline — aligns with our **mock RTC** and env-gated integration tests.
- **Multi-language / server boundary:** Clear split between low-level drivers and higher-level orchestration — analogous to **REST API + `backend/src/rtc/`**.

**Deferred:**

- **Adopting AMCF as a runtime dependency** (full server, plugin ABI, Go/Node build) — too heavy for MVP; individual ideas may be revisited if we need a formal plugin model later.
- **Deep study of ScanLab driver internals** — useful when implementing **Remote List Commands** (job download) or error mapping; track against RTC6 manual, not AMCF alone.

**Rejected for core path:**

- **PCIe / vendor DLL assumptions** inside AMCF ScanLab driver code — same as Sirius: **Ethernet RIF** is the product baseline unless an explicit ADR adds a Windows-only bridge.

---

## Adopted (already reflected in this repository)

| Idea | Where it shows up |
|------|-------------------|
| Injectable RTC backend / test double | `IRtcClient`, `MockRtcClient`, `EthernetRtcClient` |
| RTC and HTTP concerns separated | `backend/src/rtc/*` vs `backend/src/http/*` |
| Lab / CI without hardware | Mock unit tests; optional `LASERDESK_RTC6_HOST` integration test |
| Manual + package as source of truth | ADR 0001, `remote-interface-sources.md`, `version-alignment.md` |

---

## Follow-up (optional)

- When implementing **full list download** and **rich errors**, skim **AMCF `Drivers/ScanLab/Implementation`** and **Sirius3 `demos/`** for *patterns* (ordering, retries, state checks), then re-derive against **telegrams.h** / Remote Interface Wrapper in the SCANLAB package.

*Phase D deliverable — implementation plan §Phase D.*
