# WITP:AE Helper Architecture

Purpose: Explains the internal architecture of the `witp_ae_helper` module.
Version: 0.01

## Overview
This module provides the core functionality for the War in the Pacific: Admiral's Edition (WITP:AE) PBEM Game Helper. 

## Technical Stack
- **Python:** 3.14
- **ChromaDB:** 1.5.9
- **Sentence-Transformers:** 6.0.1
- **GUI:** CustomTkinter / PySide6
- **Installer:** PyInstaller / Inno Setup

## Standards
- Strict type hinting using `py.typed`.
- Checked via `black` and `ruff`.
- Tests must be placed in the `tests/` root directory mirroring the structure here.
