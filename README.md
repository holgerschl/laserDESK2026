# laserDESK 2026

Workflow-centric laser processing UI (Svelte Kit) and backend (RTC6 via Ethernet Remote Interface). The **backend** supports **mock** RTC and **UDP Remote Interface** (Phase C); see [`backend/README.md`](backend/README.md).

**Public demo (after you enable GitHub Pages + Releases):**

- Web app: [https://holgerschl.github.io/laserDESK2026/](https://holgerschl.github.io/laserDESK2026/) (project site; repo should be **`laserDESK2026`** under [holgerschl](https://github.com/holgerschl))
- Usage / installation (same site): […/laserDESK2026/usage/](https://holgerschl.github.io/laserDESK2026/usage/)
- Windows backend binary: **Releases** on [github.com/holgerschl/laserDESK2026](https://github.com/holgerschl/laserDESK2026) (workflow builds `laserdesk_backend.exe`)

Until the repo is published, build and run locally (see below and Phase F in [`docs/implementation-plan.md`](docs/implementation-plan.md)).

## Documentation

| Document | Description |
|----------|-------------|
| [Frontend README](frontend/README.md) | Svelte Kit dev, proxy, Playwright E2E |
| [Implementation plan](docs/implementation-plan.md) | Phased technical plan and Phase A artifacts |
| [Projektplan (HTML)](docs/laserDESK_2026_Projektplan.html) | SCANLAB project proposal (workflow centricity) |
| [MVP feature catalog](docs/requirements/mvp-feature-katalog.md) | In/out scope for the first increment |
| [ADR 0001 – RTC6 Ethernet](docs/adr/0001-rtc6-ethernet-remote-interface.md) | MVP integration decision |

### SCANLAB manuals (in `docs/`)

- `docs/laserDESK_Installation_and_FirstSteps.pdf`
- `docs/laserDESK_QuickGuide.pdf`
- `docs/RTC6_Doc.Rev_.1.1.3_en-US.pdf`

## Repository layout

- **`docs/`** – plans, ADRs, API sketches, RTC notes, and the PDF/HTML references above
- **`backend/`** – C++17 REST server (`laserdesk_backend`), mock RTC (`src/rtc/`), API routes (`src/http/`), unit tests
- **`.github/workflows/`** – GitHub Pages deploy; Windows backend Releases (Phase F)

**Frontend:** [`frontend/`](frontend/) — Svelte Kit, workflow-driven UI, `/rtc` monitor, `/usage` docs, Playwright E2E. See [`frontend/README.md`](frontend/README.md).
