# Mock RTC specification (Phase A)

Defines behaviour the **mock** RTC implementation must emulate so **unit tests**, **CI**, and **frontend** development do not require hardware.

The real **Ethernet Remote Interface** client (Phase C) **must implement the same abstract operations**; only transport and telegram encoding differ.

---

## 1. Abstract operations (backend internal interface)

Names are indicative; exact C++ signatures belong in `backend/` during Phase B.

| Operation | Description | Failure modes |
|-----------|-------------|----------------|
| `connect(config)` | Establish session to RTC (mock: no network). | `RTC_CONNECTION_REFUSED`, `RTC_TIMEOUT` |
| `disconnect()` | Tear down session. | — |
| `get_status()` | Return connection, run state, synthetic versions. | `RTC_NOT_CONNECTED` |
| `load_minimal_job(handle)` | Prepare internal “list” / job metaphor for MVP demo. | `RTC_INVALID_HANDLE`, `RTC_BUSY` |
| `start_execution()` | Start list execution (**RUNNING** until `stop_execution()` or, on Ethernet, board idle when polled). Mock matches Ethernet here so **Stop** stays available in the UI. | `RTC_NOT_READY`, `RTC_ALREADY_RUNNING` |
| `stop_execution()` | Halt; return to loaded or idle. | `RTC_NOT_RUNNING` |
| `reset_errors()` | Clear latched error (optional for MVP). | — |

---

## 2. State machine

States:

- **DISCONNECTED** – initial; no session.
- **CONNECTED_IDLE** – session up; nothing loaded.
- **LOADED** – minimal job prepared; ready to start.
- **RUNNING** – execution in progress (**Ethernet** client uses this while the list runs). **Mock** skips this state: `start_execution()` returns immediately to **LOADED** (simulates end-of-list) so the UI shows **Start** enabled and **Stop** disabled again without pressing Stop.
- **ERROR** – unrecoverable until `reset_errors` or `disconnect` (implementation choice: MVP mock may auto-clear on disconnect only).

Transitions:

```
DISCONNECTED --connect()--> CONNECTED_IDLE
CONNECTED_IDLE --load_minimal_job()--> LOADED
LOADED --start_execution()--> RUNNING  (Mock + Ethernet)
RUNNING --stop_execution()--> LOADED
LOADED --disconnect()--> DISCONNECTED
CONNECTED_IDLE --disconnect()--> DISCONNECTED
* --failure--> ERROR
ERROR --disconnect()--> DISCONNECTED
```

Rules:

- `load_minimal_job` from **CONNECTED_IDLE** replaces previous load (idempotent handle ok).
- `start_execution` only from **LOADED**.
- `stop_execution` only from **RUNNING** (mock returns `RTC_NOT_RUNNING` otherwise).

---

## 3. Synthetic version report (mock)

When `get_status()` (or equivalent) is called in mock mode, return:

| Field | Example value | Note |
|-------|---------------|------|
| `rtc_mode` | `"mock"` | |
| `package_version_reported` | `"1.22.0-mock"` | Matches CI expectation string |
| `bios_eth_reported` | `"mock-bios-eth"` | |
| `alignment_ok` | `true` | Always for mock |

Phase C real client fills the same fields from hardware.

---

## 4. Minimal “telegram” shapes (logical only)

Phase A does **not** fix binary layouts. For **mock**, use **internal structs** or JSON debug logs. The following **logical** telegram types are placeholders for documentation alignment with Appendix A terminology:

| Logical type | Direction | Purpose |
|--------------|-----------|---------|
| `SESSION_OPEN` | Host → Mock | Simulate auth/config (no-op in mock). |
| `JOB_DESCRIPTOR` | Host → Mock | Carries MVP placeholder handle + label. |
| `EXEC_START` | Host → Mock | Start run. |
| `EXEC_STOP` | Host → Mock | Stop run. |
| `STATUS_REPORT` | Mock → Host | State + versions + last error code. |

Real Ethernet implementation maps these to **SCANLAB command telegrams** in Phase C.

---

## 5. Error code enum (API-stable sketch)

Use stable string codes in JSON (examples):

- `RTC_NOT_CONNECTED`
- `RTC_NOT_READY`
- `RTC_NOT_RUNNING`
- `RTC_ALREADY_RUNNING`
- `RTC_BUSY`
- `RTC_INVALID_HANDLE`
- `RTC_TIMEOUT`
- `RTC_VERSION_MISMATCH` (real client only, when alignment check fails)
- `RTC_INTERNAL` (unexpected)

HTTP mapping: see OpenAPI sketch under `docs/api/`.

---

*Phase A artifact – revise after Phase C spike defines real telegram mapping.*
