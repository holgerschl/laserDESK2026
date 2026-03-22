# ADR 0001: RTC6 Ethernet Remote Interface as MVP integration path

## Status

Accepted

## Context

laserDESK 2026 targets a **workflow-centric** desktop/web UI with a **portable** backend. SCANLAB provides two different “remote” ideas:

1. **laserDESK remote control** – automates the **laserDESK application** (TCP/serial); documented separately from the Quick Guide.
2. **RTC6 Remote Interface** – **RTC6 Ethernet** boards accept **command telegrams** with **Remote Interface Mode** enabled; the manual describes architectures where the control PC needs **no platform-specific PCIe driver**.

The Projektplan and `docs/implementation-plan.md` assume long-term **OS independence** for RTC access. PCIe + Windows DLL is a common legacy path but conflicts with that goal if it becomes the only implementation.

## Decision

For **MVP**, the product **must** implement RTC access through **RTC6 Ethernet** using **Remote Interface Mode** and the **command telegram** protocol defined by SCANLAB documentation and the delivered **RTC6 software package**.

- The **portable** C++ core **must not** require Windows or a PCIe RTC DLL.
- A **temporary** PCIe/DLL-based adapter **may** exist only as an optional, isolated module for internal lab bring-up; it **must not** be the default or required for MVP demos or CI.

**laserDESK 1.6 remote control** integration is **out of scope** for MVP unless a separate ADR is accepted.

## Consequences

### Positive

- Backend can run on **Linux** or other OS once the Ethernet client is complete.
- Aligns with RTC6 manual **Appendix A (RTC6 Ethernet Board)** and Remote Interface architecture diagrams.
- Clear separation from legacy **SLLaserDesk.exe** automation.

### Negative / risks

- Requires **network** configuration, correct **BIOS-ETH** + **software package** pairing, and understanding of **telegrams** (see `docs/rtc/remote-interface-sources.md`).
- **Hardware**: Ethernet-capable RTC6 boards are mandatory for MVP validation; PCIe-only labs need network boards or an explicit exception ADR.

## References

- `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf` – Appendix A: RTC6 Ethernet Board; §6 Developing RTC6 user programs (context).
- `docs/rtc/version-alignment.md`
- `docs/requirements/mvp-feature-katalog.md`
