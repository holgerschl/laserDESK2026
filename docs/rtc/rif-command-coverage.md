# RIF command coverage (laserDESK backend)

Maps **Remote Control** (`R_DC_*`) and **list** (`R_LC_*`) IDs used in this repository to manual / `telegrams.h` names. Update this file when adding UDP commands; extend **OpenAPI** (`docs/api/openapi-phase-a-sketch.yaml`) and **backend README** connect/status fields as needed.

## `R_DC_*` — implemented (Ethernet path)

| ID | Constant (`telegram_raw.hpp`) | Where used |
|----|------------------------------|------------|
| 1 | `kRdcConfigList` | `EthernetRtcClient::load_dxf_job` (optional, with `dxf_rif_list_upload`) |
| 4 | `kRdcGetInputPointer` | `load_dxf_job`, `get_status` flow |
| 15 | `kRdcExecuteListPos` | `start_execution` |
| 16 | `kRdcStopExecution` | `stop_execution` |
| 31 | `kRdcGetStatus` | `connect`, `get_status`, `rtc_discover` probe |
| 38 | `kRdcGetHeadPara` | `load_correction_file` (ParaNo 1 → K xy bit/mm) |
| 130 | `kRdcSelectCorTable` | `load_correction_file` |
| 154 | `kRdcLoadCorrectionFile` | `load_correction_file` |
| 155 | `kRdcNumberOfCorTables` | `load_correction_file` (optional) |

## `R_LC_*` — list upload (Phase G.4)

Emitted as RAW command telegrams when `dxf_rif_list_upload` is enabled (`remote_list_commands.hpp`):

| ID | Constant | Role |
|----|----------|------|
| 259 | `kLcEndOfList` | Terminates streamed list |
| 312 | `kLcMarkXyztAbs` | Segment end marks |
| 321 | `kLcJumpXyAbs` | Segment jumps |

## Roadmap / not wired in MVP Ethernet client

Many IDs exist in root `telegrams.h`; only the above are exercised by `EthernetRtcClient` and discover. Typical next candidates follow product workflow (laser parameters, list memory layout, additional queries) — always cross-check **manual §16.10.9** and package headers before adding.

## Related

- UDP robustness (seq matching, retries, metrics): [`handbuch-1089-remote-interface-plan.md`](handbuch-1089-remote-interface-plan.md)
- Source hierarchy: [`remote-interface-sources.md`](remote-interface-sources.md)
