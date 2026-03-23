# Phase D – AMCF reference study

**Purpose:** Record what was taken from the public **Autodesk Machine Control Framework (AMCF)** repository versus what laserDESK 2026 **defers** or **rejects**, per the implementation plan (Projektplan Phase 3). This is a pattern and architecture note, not a license to copy vendor SDKs wholesale.

**Source reviewed (public):**

- [AMCF](https://github.com/Autodesk/AutodeskMachineControlFramework) — README, top-level layout, `Drivers/ScanLab/` (Headers, Interfaces, Implementation, ACT, Resources).

Authoritative RTC behaviour remains **SCANLAB RTC6 manual + software package**; see [`docs/rtc/remote-interface-sources.md`](rtc/remote-interface-sources.md).

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

- **PCIe / vendor DLL assumptions** inside AMCF ScanLab driver code — **Ethernet RIF** is the product baseline unless an explicit ADR adds a Windows-only bridge.

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

- When implementing **full list download** and **rich errors**, skim **AMCF `Drivers/ScanLab/Implementation`** for *patterns* (ordering, retries, state checks), then re-derive against **telegrams.h** / Remote Interface Wrapper in the SCANLAB package.

*Phase D deliverable — implementation plan §Phase D.*
