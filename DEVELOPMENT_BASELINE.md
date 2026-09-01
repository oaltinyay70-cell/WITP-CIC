# DEVELOPMENT BASELINE - WITP CIC
> Locked 2026-09-01 | Version: 0.01 | Source: User instruction

## 1. Versioning
- **Current:** `VERSION:1` = `0.01`
- **Rule:** Version numbers start at `0.01`. Only increase when user explicitly says to.
- **Format:** `MAJOR.MINOR` e.g., 0.01, 0.02, 0.10, 1.00
- **Storage:** `VERSION` (root), `src/witp_ae_helper/__init__.py:__version__`, git tag `v0.01` on release
- **Process:** On user instruction `bump to X.Y`, update both files, commit `chore: bump to X.Y`, tag, push

## 2. Documentation - Vigorous (MANDATORY)
Every code artifact must be documented:

### Module level (`file_path:1`)
```python
"""
Module purpose, dependencies, key exports, version.
"""
```

### Function/Class
- Docstring with: Description, Args, Returns, Raises, Example, `file_path:line_number` cross-refs
- Inline comments for: regex, parsing heuristics, intent deduction logic, GUI layout

### Other
- `README.md` + `docs/rag/` for knowledge base
- `CHANGELOG.md` for version history
- `PRE_LAUNCH_CHECKLIST.md` for blockers

No code merged without docstrings.

## 3. Project Baseline (from prior turns)
- **Purpose V1:** Gather intel, deduce enemy intentions, increase battlefield awareness from turn outputs
- **Turn Files:** Awaiting user sample (logged in PRE_LAUNCH_CHECKLIST.md)
- **Platform:** Windows GUI executable + installer with splash screen (asset TBD) + WITP:AE folder auto-find (skippable for test)
- **Stack:** Python 3.14 + ChromaDB + SentenceTransformers (free/unlimited) + GUI (CustomTkinter/PySide6) + PyInstaller
- **Repo:** https://github.com/oaltinyay70-cell/WITP-CIC @ `master`
- **RAG:** `docs/rag/witp-ae/` + `rag/ingest.py` + `rag/query.py` + `.opencode/context.md`

## 4. Compliance
- All future edits must update docs and version adherence check before commit.
