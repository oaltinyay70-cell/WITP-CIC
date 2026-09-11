v0.01

# Changelog

## [0.01] - 2026-09-09
- Initial setup.
- Added `__main__.py` entry point for executable target.
- Added `scripts/build_exe.py` to compile using PyInstaller.
- Created `DEVELOPMENT_BASELINE.md` with core project rules.
- Added `requirements.txt` and installed specified free RAG stack (ChromaDB 1.5.9, sentence-transformers 6.0.1).
- Restored `WITPAE_Manual.txt` and added `scripts/rag_ingest.py` to ingest manual into ChromaDB.
- Added `assets/map.jpg` to be used for the SIGINT Map.
- Created comprehensive README.md summarizing project MVP, architecture, and repository structure.

## [Unreleased]
- **Retro TUI Overhaul**: Implemented a W.U.T.H.U.R style green-phosphor CRT dashboard built in native C++.
- **Layout & Rendering**: Locked console buffer size to eliminate standard Windows scrolling, replaced system("cls") with native Win32 API rendering for flicker-free interface.
- **Audio Feedback**: Added `Beep()` sound effects for cursor navigation, selection, and error states.
- **High-Contrast UI**: Redesigned all modal popups (Intel Assessment & Logout) with bright white text on black background enclosed by a solid green ASCII border.
- **Visual Alerting**: The 0-100% Solidity confidence rating now blinks using ANSI escape sequences (`\033[5m`) when confidence is >= 90%.
- **Intel Categorization**: Fully separated "Discovered Allied Units" from the "High Value Target" (HVT) tracker based on strict SIGINT movement rules.
- **Directory Configuration**: Added interactive "ADD/MANAGE CAMPAIGN VAULT" module allowing users to manage vaults, switch active games, import/export ZIP archives, and configure intelligence parsing directories dynamically.
- **State Management**: Refined `InteractiveTUI` states routing Japanese War Room and Wiki modules to a dedicated 'Under Development' page.

