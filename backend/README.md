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

**CORS (hosted UI on GitHub Pages):** if the browser loads the app from another origin (e.g. `https://holgerschl.github.io`) and the API runs on `localhost`, set:

`LASERDESK_CORS_ORIGIN=https://holgerschl.github.io`

(Use your exact Pages origin, no path.) The server then sends `Access-Control-Allow-Origin` and handles `OPTIONS` preflight. Omit for same-origin or local dev with Vite proxy.

## API (`/api/v1/`)

- `GET /health` — `rtc_mode`: `mock` | `ethernet` | `disconnected`
- `GET /rtc/status` — for ethernet, may include `remote_status` / `remote_pos` after `R_DC_GET_STATUS`
- `POST /rtc/connect` — see below
- `POST /rtc/disconnect`
- `POST /jobs/minimal-demo` (+ optional `label`), `/run`, `/stop`

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
  "tgm_format": 0,
  "recv_timeout_ms": 800,
  "expected_package_tag": "1.22.0",
  "expected_bios_eth_tag": "from-lab-notes"
}
```

- **`port`**: defaults to **5020** if omitted — **confirm** with your RTC6 package / `eth_get_port_numbers` (see `docs/rtc/bring-up-checklist-phase-c.md`).
- **`tgm_format`**: must match `eth_set_remote_tgm_format` on the board (RAW is typical; default `0` may need changing per site).

On connect, the client sends **`R_DC_GET_STATUS` (31)** as a handshake.  
**Load / run / stop** use **`R_DC_GET_INPUT_POINTER` (4)**, **`R_DC_EXECUTE_LIST_POS` (15)** with list `1` pos `0`, and **`R_DC_STOP_EXECUTION` (16)** — minimal vertical slice only; real jobs need Remote **List** commands from the SCANLAB package.

## Layout

```
backend/
  CMakeLists.txt
  src/
    main.cpp
    rtc/
      job_id.cpp
      mock_rtc_client.*
      ethernet_rtc_client.*
      rif/              # Remote Interface framing + UDP (Asio)
        telegram_raw.*
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
