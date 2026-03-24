# Authoritative sources: RTC6 Remote Interface

Phase A grounding: where the **Ethernet Remote Interface** contract is defined. Use these when implementing Phase C; do not rely on second-hand summaries alone.

## 1. In this repository

| Document | Relevance |
|----------|-----------|
| `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` | Primary English manual for **RTC6 Software Package 1.22.0** (title page date 2025-08-26). |
| `telegrams.h` (repo root, from SCANLAB package) | `TGM_HEADER`, `TGM_FORMAT::RAW` (=1), Remote IDs — reference for `telegram_raw.hpp` / list commands. |
| `rtc6_rif_wrapper.cpp` (repo root, from SCANLAB package) | **Seq sync:** ctor sends payload `{0x12345678}` with header `seqnum` **0**, then `seqnum = answ.payload.buffer[0] + 1` before further traffic. |

**PDF locations to study first (manual TOC):**

- **§6 Developing RTC6-User Programs** – DLL concept, list memory, initialization, list load/start/stop, multi-board notes, error handling. Establishes how the PC-side stack relates to board execution (baseline even when using Ethernet).
- **§16 Appendix A: RTC6 Ethernet Board** – Product overview vs PCIe, **Remote Interface Mode** ON/OFF behaviour, architecture figures (user PC with any OS, switch, board), **BIOS-ETH** + software package version matching notes, **command telegrams** vs loading jobs as RTC6 commands when mode is OFF.

Further chapters (scan/laser basics, command reference **§10**) apply when mapping telegrams and list semantics to concrete operations.

### 1.1 Correction files (`.ct5`) and calibration **bit/mm**

In **`docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf`** (Doc. Rev. 1.1.3 en-US), Chapter **7** (*Basic Functions for Scan Head Control and Laser Control*) defines how correction tables relate to the Image Field:

- **§7.3.5** *Image Field Correction and Correction Tables* — loading with `load_correction_file`, naming (`D2_*.ct5`, `D3_*.ct5`), assignment with `select_cor_table` / list variants.
- **§** *ct5 Correction File Header* (manual PDF **page 191**, same chapter) — the **`.ct5` header contains 16 parameters** (indexed **0…15** in the API table). Of these:
  - **Parameter 1** is **Calibration factor K xy [bit/mm]** (maps Image Field coordinates in mm to control bits). Cross-reference: **§7.3.2** *Image Field Size and Image Field Calibration* (manual PDF page **180**).
  - **Parameter 0**: 2D vs 3D table type; **parameters 3…7**: 3D-only; full table through parameter **15** (field-size limit, etc.) — see the PDF table on pages **191–192**.
- After tables are **loaded on the board**, the same header values can be read via **`get_table_para`** (loaded tables) or **`get_head_para`** (assigned tables), per the notes in that section.
- **1:1 demo file `Cor_1to1.ct5`**: the manual states it carries a **calibration factor of value 0**; for a “real” factor, **CorrectionFileConverter.exe** → *Show File Header* → field **Field Calibration [Bit/mm]** (see **§** *1to1 Correction Tables*, manual PDF page **190**).

**laserDESK note:** After `R_DC_LOAD_CORRECTION_FILE` + `R_DC_SELECT_COR_TABLE`, `EthernetRtcClient` calls **`R_DC_GET_HEAD_PARA`** (`get_head_para`, manual p. 465) with **ParaNo = 1** to read **K xy [bit/mm]** and stores it as the active scale for **all** G.4 DXF → list mapping (`DxfRifListMapParams::bits_per_mm` in `load_dxf_job`). If **K ≤ 0**, the telegram fails, or only **Mock** RTC is used, the scale stays at connect **`dxf_rif_bits_per_mm`** (default **128**). **`POST /api/v1/rtc/disconnect`** restores the connect default for the next session.

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
| AMCF (GitHub) | Reference implementation only; SCANLAB manual + package win on conflicts. Phase D note: [`docs/reference-phase-d-amcf.md`](../reference-phase-d-amcf.md). |

## 4. Implementation in this repository (Phase C)

| Area | Location |
|------|----------|
| TGM_HEADER + RAW payload (little-endian `uint32_t`) | `backend/src/rtc/rif/telegram_raw.*` |
| UDP send / receive (manual §16.10.8) | `backend/src/rtc/rif/udp_channel.*` (standalone **Asio**) |
| Remote Control IDs used for MVP slice | `R_DC_GET_STATUS` (31), `R_DC_GET_INPUT_POINTER` (4), `R_DC_EXECUTE_LIST_POS` (15, list 1 pos 0), `R_DC_STOP_EXECUTION` (16) — see `telegram_raw.hpp` |
| Correction load + **K xy** readback | `POST /api/v1/rtc/correction/load` → `R_DC_LOAD_CORRECTION_FILE` (154), `R_DC_SELECT_COR_TABLE` (130), then **`R_DC_GET_HEAD_PARA`** (38) ParaNo **1** — `backend/src/rtc/ethernet_rtc_client.cpp` |
| `IRtcClient` over Ethernet | `backend/src/rtc/ethernet_rtc_client.*` (connect: seq sync per wrapper, then `R_DC_GET_STATUS`) |
| Subnet discover | `backend/src/rtc/rtc_discover.cpp` (same seq sync + `GET_STATUS` per host) |

**Still vendor-dependent:** exact **UDP port**, **`tgm_format`** (default **1** = RAW, matching wrapper), and full **Remote List Command** sequences — keep repo `telegrams.h` in sync with your installed package revision.

## 5. Open work (beyond MVP slice)

- **Phase G.4 (repo):** DXF → **`R_LC_JUMP_XY_ABS` / `R_LC_MARK_XYZT_ABS`** / **`R_LC_END_OF_LIST`** (numeric IDs in `backend/src/rtc/rif/remote_list_commands.hpp`, sync with SCANLAB package **`telegrams.h`**), optional **`R_DC_CONFIG_LIST`**; enable with `dxf_rif_list_upload` on `POST /rtc/connect`.
- Automatic **version / BIOS-ETH** readback if a suitable Remote Command exists in your package revision.

---

*Phase A artifact – revise when the pinned RTC6 package version or manual revision changes.*
