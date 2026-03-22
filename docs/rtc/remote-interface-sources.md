# Authoritative sources: RTC6 Remote Interface

Phase A grounding: where the **Ethernet Remote Interface** contract is defined. Use these when implementing Phase C; do not rely on second-hand summaries alone.

## 1. In this repository

| Document | Relevance |
|----------|-----------|
| `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` | Primary English manual for **RTC6 Software Package 1.22.0** (title page date 2025-08-26). |

**PDF locations to study first (manual TOC):**

- **§6 Developing RTC6-User Programs** – DLL concept, list memory, initialization, list load/start/stop, multi-board notes, error handling. Establishes how the PC-side stack relates to board execution (baseline even when using Ethernet).
- **§16 Appendix A: RTC6 Ethernet Board** – Product overview vs PCIe, **Remote Interface Mode** ON/OFF behaviour, architecture figures (user PC with any OS, switch, board), **BIOS-ETH** + software package version matching notes, **command telegrams** vs loading jobs as RTC6 commands when mode is OFF.

Further chapters (scan/laser basics, command reference **§10**) apply when mapping telegrams and list semantics to concrete operations.

## 2. SCANLAB software package (not in repo)

The **RTC6 software package** ZIP from SCANLAB (e.g. linked from the Projektplan) typically contains:

- Host libraries / headers / examples (exact layout per package version).
- **RTC6BIOSETH_*.out** (or similarly named) firmware images for **BIOS-ETH** updates.
- Tools and demo programs referenced in §5 / §5.8 of the manual.

**Rule:** The **same version** must be used for **host API**, **board firmware/BIOS-ETH**, and documentation revision where possible. See `docs/rtc/version-alignment.md`.

## 3. Related but not authoritative for RTC6 telegrams

| Source | Use |
|--------|-----|
| `laserDESK_QuickGuide.pdf` §8.4 | Describes **laserDESK** remote control (laserDESK as server), **not** RTC6 Ethernet. |
| Sirius3 / AMCF (GitHub) | Reference implementations only; SCANLAB manual + package win on conflicts. Phase D synthesis: [`docs/reference-phase-d-sirius3-amcf.md`](../reference-phase-d-sirius3-amcf.md). |

## 4. Implementation in this repository (Phase C)

| Area | Location |
|------|----------|
| TGM_HEADER + RAW payload (little-endian `uint32_t`) | `backend/src/rtc/rif/telegram_raw.*` |
| UDP send / receive (manual §16.10.8) | `backend/src/rtc/rif/udp_channel.*` (standalone **Asio**) |
| Remote Control IDs used for MVP slice | `R_DC_GET_STATUS` (31), `R_DC_GET_INPUT_POINTER` (4), `R_DC_EXECUTE_LIST_POS` (15, list 1 pos 0), `R_DC_STOP_EXECUTION` (16) — see `telegram_raw.hpp` |
| `IRtcClient` over Ethernet | `backend/src/rtc/ethernet_rtc_client.*` |

**Still vendor-dependent:** exact **UDP port**, **`tgm_format`** value (must match `eth_set_remote_tgm_format`), and full **Remote List Command** sequences for real jobs — use SCANLAB **RTC6 Software Package** (`telegrams.h`, Remote Interface Wrapper, demo sources).

## 5. Open work (beyond MVP slice)

- Rich job download via **Remote List Commands** (IDs 256…).
- Automatic **version / BIOS-ETH** readback if a suitable Remote Command exists in your package revision.

---

*Phase A artifact – revise when the pinned RTC6 package version or manual revision changes.*
