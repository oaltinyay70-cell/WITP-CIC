# PRE-LAUNCH CHECKLIST - WITP:AE Player Help Utility
> Must be cleared before launch. Updated 2026-09-01

## BLOCKED / AWAITING INPUT
- [ ] **1. Turn Result Files** - User to provide specific file list + sample folder structure (combatReport.txt, operationsReport.txt, sigint etc.) - *status: AWAITING USER*
- [ ] **2. WITP:AE Folder Detection** - Implement auto-find + manual browse for WITP:AE install path; test skip allowed for dev
- [ ] **3. Splash Screen Asset** - User to provide image file (PNG/BMP) for installer + app startup
- [ ] **4. Game Manuals / Documentation** - Awaiting feed from user - will ingest to `docs/rag/witp-ae-*.md`

## REQUIREMENTS - LOCKED
- [ ] **Purpose V1:** Gather intel, deduce enemy intentions, increase battlefield awareness (derived from turn reports)
- [ ] **Platform:** Windows executable with GUI
- [ ] **Installer:** Windows installer with splash screen, WITP:AE folder discovery pre-start
- [ ] **Tech:** Python 3.14 + GUI (CustomTkinter/PySide6) + PyInstaller (free, unlimited) - fits existing stack

## RAG / INTEL DESIGN - PENDING DOCS
- [ ] Parse manual to define intel sources (SIGINT, combat reports, ops reports, recon)
- [ ] Define "battlefield awareness" metrics (e.g., enemy TF movements, airframe losses, base build-up)
- [ ] Define "intent deduction" heuristics (e.g., mass shipping at X -> invasion likely)

## NOTES
- Do not launch without clearing BLOCKED items above.
- User explicitly allows skipping WITP:AE folder check during testing.
