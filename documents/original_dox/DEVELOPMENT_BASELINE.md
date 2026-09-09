# DEVELOPMENT BASELINE - WITP CIC
> Version: 0.01 | Effective: 2026-09-01 | Status: LOCKED
> All code must adhere to this baseline until superseded.

## 1. Versioning
- **Current:** `VERSION:1` = `0.01`
- **Policy:** Version starts at 0.01. **Never** auto-increment. Only increment when user explicitly instructs. Assistant must ask for confirmation if unsure.
- **Implementation:** `VERSION` file is source of truth, mirrored in `CHANGELOG.md:1` and `src/witp_ae_helper/__init__.py:1` `__version__`
- **Tagging:** `git tag v0.01 && git push origin v0.01` on every version bump
- **Commit msg format:** `[v0.01] description`

## 2. Documentation - Vigorous
Every code unit must be documented:
- **File header:** Purpose, version, author, dependencies (at top of every `*.py`)
- **Functions/Classes:** Google-style docstring with Args, Returns, Raises, Example; include `file_path:line_number` refs where relevant
- **Complex logic:** Inline comments for why, not what; link to manual section `docs/rag/witp-ae/*.md:line`
- **README per module:** `src/witp_ae_helper/README.md` explains architecture, `docs/rag/README.md` for RAG
- **Changelog:** Every change appended to `CHANGELOG.md:1`
- **No undocumented code** - PR/commit blocked if docstring missing

## 3. Project Standards
- **Project root:** `C:\Users\ozgur\Documents\WITP CIC`
- **Stack:** Python 3.14, ChromaDB 1.5.9, sentence-transformers 6.0.1, GUI: CustomTkinter/PySide6, Installer: PyInstaller/Inno Setup - all free/unlimited
- **Style:** `black`, `ruff`, type hints required, `py.typed`
- **Tests:** `tests/` mirror `src/`; must document test purpose

## 4. Process
- User feeds WITP:AE manuals -> ingest via `rag/ingest.py:1`
- All intel deductions must cite manual source
- Ask clarification for any ambiguity - never assume WITP:AE mechanics
- Keep `PRE_LAUNCH_CHECKLIST.md:1` updated; no launch without clearing BLOCKED
