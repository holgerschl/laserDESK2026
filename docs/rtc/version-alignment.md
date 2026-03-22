# RTC6 version alignment and mismatch detection

## 1. Problem

SCANLAB documentation states that **RTC6 Ethernet** behaviour depends on consistent versions of:

- **RTC6 host software package** (DLL/API, tools, documentation revision),
- **Board firmware / BIOS-ETH** on the Ethernet board,
- (When applicable) **RTC6 firmware** expectations from consuming applications such as laserDESK 1.6.26.0 ↔ **RTC6 1.22.1** pairing.

Mismatch can cause **Remote Interface Mode** not to activate or **unexpected runtime behaviour** (see RTC6 manual Appendix A notes on BIOS-ETH and package version).

## 2. Policy for laserDESK 2026

1. **Pin** a supported triple for each release: `host_package_version`, `bios_eth_build` (or file id), and `manual_doc_rev` (e.g. 1.1.3 en-US).
2. **Document** the pin in release notes and in backend config example (`expected_rtc6_package_version` or similar).
3. **At connect** (Phase C implementation), **read back** whatever identity the board/API exposes (exact calls TBD from manual/package) and compare to the pin.
4. On mismatch: **fail closed** with HTTP **503** or **424** (dependency failed) and a body that states:
   - observed values,
   - expected values,
   - pointer to `docs/rtc/remote-interface-sources.md` and SCANLAB support.

## 3. API surface (planned)

Exposed to operators and UI (sketch; details in `docs/api/openapi-phase-a-sketch.yaml`):

| Element | Purpose |
|---------|---------|
| `GET /api/v1/health` | Process up; may include `rtc_mode: "mock" \| "ethernet"`. |
| `GET /api/v1/rtc/status` | Connection state, last error, and when implemented `versions: { package, bios_eth, ... }`. |

Mock RTC returns **synthetic** versions that always match the pin (for CI).

## 4. Operational checklist (lab)

Before enabling real hardware:

- [ ] Ethernet board IP / subnet documented.
- [ ] **BIOS-ETH** updated from the **same** RTC6 package as the host libraries.
- [ ] Firewall allows TCP to board (ports per manual/package).
- [ ] Cable and switch verified; no duplicate IP.
- [ ] laserDESK 1.6 on same subnet updated to compatible RTC6 firmware **only if** that stack is still used alongside tests.

---

*Phase A artifact – extend with exact API calls and version fields once Phase C spike is done.*
