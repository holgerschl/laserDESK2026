# laserDESK 2026 – backend

C++17 HTTP service with an **injectable RTC layer**:

- **Mock** `IRtcClient` (CI / `--rtc-demo`)
- **Ethernet Remote Interface** `IRtcClient`: **UDP** Command Telegrams per RTC6 manual **§16.10.6–16.10.8** (RAW format, `TGM_VERSION` `0x01000000`)

## Requirements

- **CMake** ≥ 3.16  
- C++17 compiler (MSVC, Clang, GCC)  
- Git (for CMake `FetchContent`: cpp-httplib, nlohmann/json, **Asio** standalone, GoogleTest)  
- Network access on first configure (dependency download)

## Build

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build --parallel
ctest --test-dir backend/build --output-on-failure
```

Windows (Visual Studio example):

```powershell
cmake -S backend -B backend/build -G "Visual Studio 17 2022" -A x64
cmake --build backend/build --config Release
ctest -C Release --test-dir backend/build --output-on-failure
```

## Targets

| Target | Description |
|--------|-------------|
| `laserdesk_backend` | REST server on `0.0.0.0:8080` by default |
| `laserdesk_tests` | Unit + optional env-gated RTC6 integration test |
| `laserdesk_rtc_core` | Static library: `http/` + `rtc/` (mock, ethernet, `rif/`) |

## Run

```bash
./backend/build/laserdesk_backend --port 8080
./backend/build/laserdesk_backend --rtc-demo --port 8080
```

`--rtc-demo` pre-connects the **mock** RTC. Use `POST /api/v1/rtc/connect` for **ethernet** (real board).

Environment: `LASERDESK_PORT` if `--port` omitted.

### Windows Defender / SmartScreen (false “Trojan” warnings)

Unsigned executables that open **TCP** (HTTP) and **UDP** (RTC) are often flagged heuristically. **Renaming or “changing the signature” of the binary is not a reliable or appropriate fix** and does not replace proper trust.

What actually helps:

1. **Authenticode code signing** — sign `laserdesk_backend.exe` with a valid code-signing certificate (commercial or OSS CI options). This is what Microsoft expects for distributed binaries.
2. **False-positive report** — submit the file to Microsoft as a false positive (Defender / “Submit a file”) so the cloud rules can be updated for this build hash.
3. **Embedded metadata** — the Windows build includes a **VERSIONINFO** resource and an **application manifest** (`asInvoker`) under `backend/win/` so the file identifies itself as the laserDESK backend (may reduce some generic detections; not guaranteed).

**CORS (hosted UI on GitHub Pages):** if the browser loads the app from another origin (e.g. `https://holgerschl.github.io`) and the API runs on `localhost`, set:

`LASERDESK_CORS_ORIGIN=https://holgerschl.github.io`

(Use your exact Pages origin, no path.) The server then sends `Access-Control-Allow-Origin`, handles `OPTIONS` preflight, and **`Access-Control-Allow-Private-Network: true`** so Chromium can allow an HTTPS page to call `http://127.0.0.1` on your PC. Omit `LASERDESK_CORS_ORIGIN` for same-origin or local dev with Vite proxy. If the browser shows a local-network permission prompt, allow it.

## API (`/api/v1/`)

- `GET /health` — `rtc_mode`: `mock` | `ethernet` | `disconnected`
- `GET /rtc/status` — for ethernet, may include `remote_status` / `remote_pos` after `R_DC_GET_STATUS`
- `POST /rtc/connect` — see below
- `POST /rtc/disconnect`
- `POST /jobs/minimal-demo` (+ optional `label`), `/run`, `/stop`
- `POST /jobs/dxf` — parse ASCII DXF (**LINE** entities): JSON `{"source":"demo"}` (needs repo `demo/dxf/` or `LASERDESK_DEMO_DXF`), or `{"dxf_text":"…","source_name":"…"}`, or multipart field `file`
- `GET /jobs/dxf/{job_id}` — parsed job + entities
- `POST /jobs/dxf/{job_id}/load`, `/run`, `/stop` — load into RTC session (same run/stop as minimal-demo)

### `POST /rtc/connect` body

**Mock**

```json
{ "mode": "mock" }
```

**Ethernet** (UDP Remote Interface; board must be in **Remote Interface Mode**)

```json
{
  "mode": "ethernet",
  "host": "192.168.1.50",
  "port": 5020,
  "tgm_format": 1,
  "recv_timeout_ms": 3000,
  "udp_local_bind": "",
  "rif_connect_status_attempts": 3,
  "rif_udp_max_extra_datagrams": 8,
  "rif_retry_delay_ms": 50,
  "expected_package_tag": "1.22.0",
  "expected_bios_eth_tag": "from-lab-notes",
  "dxf_rif_list_upload": false,
  "dxf_rif_bits_per_mm": 128,
  "rif_config_list_mem1": 1,
  "rif_config_list_mem2": 2
}
```

- **`dxf_rif_list_upload`**: optional, default **false**. If **true**, load sends **`R_DC_CONFIG_LIST`** (1), **`R_DC_GET_INPUT_POINTER`** (4), then one UDP telegram per list command: **`R_LC_JUMP_XY_ABS`** / **`R_LC_MARK_XYZT_ABS`** per DXF LINE and **`R_LC_END_OF_LIST`** (IDs from SCANLAB package `telegrams.h`, mirrored in `src/rtc/rif/remote_list_commands.hpp`).
- **`dxf_rif_bits_per_mm`**: default scale **mm → scanner bits** for G.4 list upload (`src/rtc/job/dxf_rif_list_mapper.cpp`). On **Ethernet**, after a successful **`POST /api/v1/rtc/correction/load`**, the backend reads **K xy [bit/mm]** from the assigned correction table via **`R_DC_GET_HEAD_PARA`** (ParaNo **1**, manual Ch. 10 p. 465) and **replaces** this internal scale until disconnect. If **K ≤ 0** (e.g. **Cor_1to1.ct5**), the query fails, or **Mock** RTC is used, the connect-time value (default **128**) remains in effect.
- **`rif_config_list_mem1` / `rif_config_list_mem2`**: passed to **`R_DC_CONFIG_LIST`** (same as SCANLAB RIF `config_list`).

- **`port`**: defaults to **5020** if omitted — **confirm** with your RTC6 package / `eth_get_port_numbers` (see `docs/rtc/bring-up-checklist-phase-c.md`).
- **`tgm_format`**: must match `eth_set_remote_tgm_format` on the board. Default **`1`** = `TGM_FORMAT::RAW` in SCANLAB `telegrams.h` (same as `rtc6_rif_wrapper.cpp`). Use **`0`** only if the board is configured for `NONE`.
- **`udp_local_bind`**: optional IPv4; bind the local UDP socket to that address (multi-NIC). Omit or empty = any interface.
- **`rif_connect_status_attempts`**, **`rif_retry_delay_ms`**: retry idempotent **`R_DC_GET_STATUS`** during connect after UDP timeout (defaults **3** / **50 ms**).
- **`rif_udp_max_extra_datagrams`**: cap on extra inbound datagrams while waiting for the matching answer **seq** (UDP reordering; default **8**). Same key is accepted on **subnet discover** JSON.

On connect, the client sends the **seq sync** payload **`0x12345678`** (header `seqnum` **0**, as in the SCANLAB wrapper), then **`R_DC_GET_STATUS` (31)** with the next strictly increasing `seqnum`.  
**Load / run / stop** use **`R_DC_GET_INPUT_POINTER` (4)**, **`R_DC_EXECUTE_LIST_POS` (15)** with list `1` pos `0`, and **`R_DC_STOP_EXECUTION` (16)** — minimal vertical slice. Optional **DXF list build** (`dxf_rif_list_upload`) adds **`R_DC_CONFIG_LIST` (1)** and **`R_LC_*`** jump/mark telegrams per package `telegrams.h`.

## Layout

```
backend/
  CMakeLists.txt
  src/
    main.cpp
    rtc/
      job_id.cpp
      job/              # Phase G.4: RtcJobPlan + DXF → RIF list sequence
        rtc_job_plan.*
        dxf_rif_list_mapper.*
      mock_rtc_client.*
      ethernet_rtc_client.*
      rif/              # Remote Interface framing + UDP (Asio)
        telegram_raw.*
        remote_list_commands.hpp
        udp_channel.*
    http/
      api_router.*
  tests/
    test_mock_rtc.cpp
    test_backend_session.cpp
    test_telegram_raw.cpp
    test_ethernet_integration.cpp   # skipped unless LASERDESK_RTC6_HOST is set
```

## References

- Mock behaviour: `docs/rtc/mock-rtc-specification.md`  
- Lab checklist: `docs/rtc/bring-up-checklist-phase-c.md`  
- Manual: `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` (Appendix A §16.10)
