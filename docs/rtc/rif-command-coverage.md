# RIF command coverage (laserDESK backend)

Maps **Remote Control** (`R_DC_*`) and **list** (`R_LC_*`) IDs used in this repository to manual / `telegrams.h` names. Update this file when adding UDP commands; extend **OpenAPI** (`docs/api/openapi-phase-a-sketch.yaml`) and **backend README** connect/status fields as needed.

## `R_DC_*` — implemented (Ethernet path)

| ID | Constant (`telegram_raw.hpp`) | Where used |
|----|------------------------------|------------|
| 1 | `kRdcConfigList` | `EthernetRtcClient::load_dxf_job` (optional, with `dxf_rif_list_upload`) |
| 3 | `kRdcSetStartListPos` | `load_dxf_job`: before `kRdcLoadListPos` (same `ListNo`, `Pos` as `get_input_pointer`) — wrapper §6.4.1 |
| 4 | `kRdcGetInputPointer` | `load_dxf_job` (before `kRdcLoadListPos` and list stream), `get_status` flow |
| 6 | `kRdcLoadListPos` | `load_dxf_job`: `load_list(ListNo, Pos)` after input pointer — manual §6.4.1 / Appendix A ID 6 |
| 11 | `kRdcSetMaxCount` | `start_execution` before execute; payload from API `repeat_count` (RTC6 Ch.10 `set_max_counts` is **External Starts**; not the same as “repeat this list N times” — see backend `rtc_client.hpp`) |
| 15 | `kRdcExecuteListPos` | `start_execution` (list no from `POST /rtc/connect` **`rif_execute_list_no`**, default = **`rif_config_list_mem1`**) |
| 16 | `kRdcStopExecution` | `stop_execution` |
| 31 | `kRdcGetStatus` | `connect`, `get_status`, `rtc_discover` probe. **`get_status`** Status word bits (BUSY / PAUSED / …): `rif/get_status_bits.hpp`. Ethernet: if session is **`running`** and **`repeat_count` was 1**, `get_status` may set internal FSM back to **`loaded`** when all list-execution busy bits are clear; for **`repeat_count` > 1**, session stays **`running`** until **`POST /rtc/stop`** (or disconnect). |
| 142 | `kRdcGetRtcVersion` | `connect`: best-effort probe; fills `package_version_reported` if `expected_package_tag` empty (SCANLAB `RIF_test` RI path) |
| 143 | `kRdcGetBiosVersion` | `connect`: best-effort; fills `bios_eth_reported` if `expected_bios_eth_tag` empty |
| 38 | `kRdcGetHeadPara` | `load_correction_file` (ParaNo 1 → K xy bit/mm) |
| 130 | `kRdcSelectCorTable` | `load_correction_file` |
| 154 | `kRdcLoadCorrectionFile` | `load_correction_file` |
| 155 | `kRdcNumberOfCorTables` | `load_correction_file` (optional) |

## `R_LC_*` — list upload (Phase G.4)

Emitted as RAW command telegrams when `dxf_rif_list_upload` is enabled (`remote_list_commands.hpp`):

| ID | Constant | Role |
|----|----------|------|
| 259 | `kLcEndOfList` | Terminates streamed list |
| 312 | `kLcMarkXyztAbs` | Segment end marks; planar DXF uses Z = **`0x7FFFFFFF`** (`NO_Z_MOVE`, vendor `rtc6_rif_wrapper`) for 2D segments |
| 321 | `kLcJumpXyAbs` | Segment jumps |

## Roadmap / not wired in MVP Ethernet client

Many IDs exist in root `telegrams.h`; only the above are exercised by `EthernetRtcClient` and discover. Typical next candidates follow product workflow (laser parameters, list memory layout, additional queries) — always cross-check **manual §16.10.9** and package headers before adding.

## Related

- UDP robustness (seq matching, retries, metrics): [`handbuch-1089-remote-interface-plan.md`](handbuch-1089-remote-interface-plan.md)
- Source hierarchy: [`remote-interface-sources.md`](remote-interface-sources.md)
