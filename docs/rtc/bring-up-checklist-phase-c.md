# RTC6 Ethernet Remote Interface – lab bring-up (Phase C)

Use with `laserdesk_backend` **ethernet** mode and the RTC6 manual **§16 Appendix A** (Rev. 1.1.3 en-US).

## Prerequisites

- RTC6 **Ethernet** board configured for **Standalone Full State** and **Remote Interface Mode** (manual §16.10.3).
- **Board mode (once per setup):** switch the card to **Remote Interface Mode** using SCANLAB tooling — typically **RTC6conf.exe** (or equivalent) from the **same RTC6 software package** as your host libraries and BIOS-ETH; exact path depends on package layout on disk.
- **BIOS-ETH** and host-side expectations match the **same RTC6 software package** (manual §16.10, Notes on p. 1092).
- `eth_set_remote_tgm_format` set in Boot Phase 1 as required for **RAW** telegrams (manual §16.8.2 / §16.10.1).
- **Manual PDF in repo:** `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` (primary reference for §16 / Remote Interface).

## Network

- [ ] Board and PC on the same L2 segment or routed path; **Cat 5e** or better (manual §16.2.7).
- [ ] Stable IP for the board (DHCP or static per site).
- [ ] **UDP port** matches the board (same as conventional DLL path — confirm with SCANLAB tools / `eth_get_port_numbers` in your package). Default in this backend: **5020** if unspecified; **override** in `POST /api/v1/rtc/connect` with `"port"`.
- [ ] Host firewall allows **UDP** to that port (outbound from PC; replies return to ephemeral local port).
- [ ] Multi-NIC labs: optional `POST /api/v1/rtc/connect` field **`udp_local_bind`** (IPv4) to bind the local UDP socket to a specific interface.

## Software

- [ ] If `tgm_format` ≠ `0`, set `"tgm_format"` in the connect JSON to match `eth_set_remote_tgm_format` on the board.
- [ ] Optional: set `"expected_package_tag"` / `"expected_bios_eth_tag"` for operator-visible status (no automatic version negotiation yet).

## Verification

1. Start backend: `laserdesk_backend --port 8080`.
2. `POST /api/v1/rtc/connect` with `{"mode":"ethernet","host":"<board-ip>","port":<udp>}`.
3. On success: `GET /api/v1/rtc/status` should show `rtc_mode: ethernet` and, after a successful `R_DC_GET_STATUS`, `remote_status` / `remote_pos` if present.

## Tests

- Default CI: unit tests only (no hardware).
- Hardware: set `LASERDESK_RTC6_HOST` (and optional `LASERDESK_RTC6_PORT`) and run `laserdesk_tests` — integration test runs; otherwise skipped.

## Safety

Laser and scan hardware safety rules from the RTC6 manual (e.g. §3.2) apply independently of this software.
