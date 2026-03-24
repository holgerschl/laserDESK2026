# Plan: Remote Interface Mode (Handbuch „Short Information“, ca. S. 1089)

Bezug: RTC6 Manual *Remote Interface Mode Usage Short Information* (vier Schritte). Dieses Dokument ordnet jeden Schritt dem **Ist-Stand in laserDESK 2026** zu und definiert **Ausbaustufen** (ohne RTC6-SDK-Code zu kopieren).

---

## Schritt (1) — RTC6 Ethernet Board auf Remote Interface Mode

**Handbuch:** Einmalige Aktion; z. B. **RTC6conf.exe** oder DLL-basiert; Listen/Konfiguration können vordefiniert werden.

| Aspekt | laserDESK |
|--------|-----------|
| **Automatisierung im Backend** | **Nicht vorgesehen.** Modus-Umschaltung ist **Betriebsaufgabe** am Board / mit SCANLAB-Tools. |
| **Doku / UI** | UI-Hinweis in `RtcConnectionPanel.svelte` (Remote Interface + RTCConf). Ergänzend: [`bring-up-checklist-phase-c.md`](bring-up-checklist-phase-c.md) (§16.10.3, BIOS-ETH, `tgm_format`, RTC6conf, Manual-PDF). |

**Geplante Ergänzungen (niedrig Aufwand)**

- [x] Checkliste um Verweis auf **RTC6conf.exe** / Paketpfad ergänzen (nur Doku).
- [x] Optional: kurzer Link/Verweis auf Manual-PDF im Repo (`docs/RTC6_Doc…`).

**Akzeptanz:** Betreiber kann anhand Doku+Checkliste Schritt (1) unabhängig vom laserDESK-Code ausführen.

---

## Schritt (2) — Netzwerk-Socket (betriebssystemspezifisch)

**Handbuch:** Windows-Beispiel mit **Winsock** (`example_1.cpp` im SCANLAB-Paket).

| Aspekt | laserDESK |
|--------|-----------|
| **Umsetzung** | **`UdpRifChannel`** (`backend/src/rtc/rif/udp_channel.cpp`) mit **Asio** (UDP IPv4, bind ephemeral local port, `send_to` / `receive_from`). |
| **Abweichung** | Kein Winsock im Quelltext — funktional gleiche Rolle wie Schritt (2), **plattformneutral** (Vorbereitung Linux-Port laut AGENTS.md). |

**Geplante Ergänzungen**

- [x] **Absender prüfen:** `UdpRifChannel` verwirft Antworten, wenn `sender` nicht der aufgelösten **Ziel-IP/Port** entspricht.
- [x] Optional: konfigurierbare **lokale Bind-Adresse** (Multi-NIC-Labs) — `udp_local_bind` in `RtcConnectConfig` / `POST /rtc/connect`.
- [x] Doku: kurze Zeile in `remote-interface-sources.md`, dass Schritt (2) durch Asio abgedeckt ist.

**Akzeptanz:** Verbindung zu bekannter Board-IP funktioniert; optional nur Antworten der konfigurierten Gegenstelle werden als gültig gewertet.

---

## Schritt (3) — RTC6-Befehle → Command Telegrams

**Handbuch:** Manuell (Struktur z. B. S. 1096) oder **C++ Remote Interface Wrapper** im SCANLAB-Paket.

| Aspekt | laserDESK |
|--------|-----------|
| **Umsetzung** | Eigenes **RAW-Framing** in `telegram_raw.*` (Header + little-endian `uint32_t`-Payload), IDs konsistent zu Repo-**`telegrams.h`** / Manual §16.10.9. |
| **Abweichung** | Kein Einbinden des vendor **RIF-Wrapper**-Quellcodes im Backend; Verhalten orientiert sich an **`rtc6_rif_wrapper.cpp`** (nur Referenz im Repo). |

**Geplante Ergänzungen**

- [x] **Command-Coverage:** Tabelle „Manual-Befehl → implementiert (Ja/Nein)“ für MVP vs. Roadmap → [`rif-command-coverage.md`](rif-command-coverage.md).
- [x] Bei neuen Befehlen: immer **OpenAPI/README** und ggf. **Mock** erweitern (Leitplanken in `rif-command-coverage.md`).

**Akzeptanz:** Jeder neu exponierte RIF-Befehl ist dokumentiert, getestet (Mock/Unit) und gegen Manual-ID geprüft.

---

## Schritt (4) — Ausführung wie mit DLL: senden, Answers prüfen, UDP-Fehler

**Handbuch:** Command senden, **Answer prüfen** (wichtig wegen **Paketverlust** und **vertauschter Reihenfolge**), Fehlerbehandlung.

| Aspekt | laserDESK (Ist) |
|--------|------------------|
| **Seqnum** | Pro Telegramm strikt steigende **`seq`** in `EthernetRtcClient::send_remote_control`; Sync mit `0x12345678` bei Header-`seqnum` **0** wie Wrapper. |
| **Answer-Check** | `parse_answer_telegram` prüft Version, Typ Answer, **Format**, **erwartete Seq**, Payload-Länge; `check_answer` prüft **LastError**, **Cmd-ID**, Mindestlänge. |
| **Timeouts** | Pro Request `recv_timeout_ms` (Default **3000 ms**); bei Fehler `RTC_TIMEOUT` / `RTC_INTERNAL`. |
| **Reordering** | `UdpRifChannel::request_response_matching` liest bis zur erwarteten Seq oder **Cap** (`rif_udp_max_extra_datagrams`, Default 8); nur Antworten der konfigurierten Gegenstelle. |
| **Connect** | Idempotente **`R_DC_GET_STATUS`**: mehrere Versuche (`rif_connect_status_attempts`, Default 3) mit optionaler Pause (`rif_retry_delay_ms`). |
| **Metriken** | `GET /rtc/status` (Ethernet): `rif_udp_timeout_count`, `rif_udp_spurious_datagrams`, `rif_connect_status_retries_used`. |

**Geplante Ausbaustufen (Priorität)**

1. **P1 — Robustheit „harmlos“**  
   - [x] Nach Timeout: begrenzte **Wiederholung** für **idempotentes** `R_DC_GET_STATUS` beim Connect (Limit + Delay).  
   - [x] **Absender-Filter** (`udp_channel.cpp`).

2. **P2 — Reordering (Handbuch-konform)**  
   - [x] Bei erwarteter Seq **N**: weitere Datagramme derselben Gegenstelle einlesen bis Treffer oder Cap (siehe `request_response_matching`).  
   - [x] Policy: Cap verhindert Endlosschleifen bei Kollisionen mit alten Paketen; Fehlermeldung `No matching answer telegram (datagram cap exceeded)`.

3. **P3 — Produktionsniveau**  
   - [x] Konfigurierbare Retry-/Backoff-Parameter (`rif_connect_status_attempts`, `rif_retry_delay_ms`, `rif_udp_max_extra_datagrams`); Metriken auf `RtcStatus`.  
   - [ ] Optional Abgleich mit SCANLAB **eth_set_com_timeouts_auto**-Semantik aus dem Wrapper (nur Konzept, keine DLL-Pflicht).

**Akzeptanz P1:** Flaky LAN: Connect gelingt häufiger ohne falsche Doppel-Ausführung von Zustandsbefehlen.  
**Akzeptanz P2:** Unter gelegentlichem Reordering bleibt eine Session stabil definiert (Tests mit injizierten Antworten / Simulator — später erweiterbar).  
**Akzeptanz P3:** Betrieb kann Timeouts/Retries/Cap über Connect-JSON steuern und Ausfälle an Status-Zählern ablesen.

---

## Teststrategie

- **Unit:** `test_telegram_raw.cpp` (Build/Parse, Seq, `GET_HEAD_PARA`-Double, `answer_raw_matches_seq_and_format`).  
- **Integration:** `LASERDESK_RTC6_HOST` gegen echte Karte; später: **UDP-Testdouble** (künstlich falsche Seq / Drop).  
- **E2E:** Weiter Mock-Backend; Hardware optional per Env.

---

## Kurzfassung

| Schritt | In laserDESK umgesetzt? |
|---------|-------------------------|
| (1) Board-Modus | **Nein (operativ)** — Hinweise/Checkliste + Manual-PDF. |
| (2) Socket | **Ja** — Asio-UDP, Absenderfilter, optional `udp_local_bind`. |
| (3) Telegramme | **Ja** — eigenes RAW-Mapping; Coverage in `rif-command-coverage.md`. |
| (4) Ausführung + UDP-Robustheit | **Ja** — Seq + Answer-Validierung, Matching-Recv mit Cap, Connect-Retry für GET_STATUS, Status-Metriken. |

*Nächster sinnvoller Schritt (optional):* UDP-Testdouble / Simulator für automatisierte Reordering-Tests; Konzept **eth_set_com_timeouts_auto** nur bei Bedarf dokumentieren.

---

*Stand: abgestimmt auf RTC6 Manual Rev. 1.1.3 en-US / Vier-Schritte-Kasten Remote Interface.*
