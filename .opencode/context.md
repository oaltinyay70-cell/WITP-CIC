# OpenCode Context - WITP CIC v0.01
> Auto-loaded every session. Baseline: `docs/DEVELOPMENT_BASELINE.md:1` LOCKED

Project: WITP:AE Player Help Utility (WITP CIC) - v0.01
Purpose: Gather intel, deduce enemy intentions, increase battlefield awareness from turn reports
Root: `C:\Users\ozgur\Documents\WITP CIC` | Repo: https://github.com/oaltinyay70-cell/WITP-CIC
Stack: Python 3.14 + ChromaDB 1.5.9 + sentence-transformers 6.0.1 (free/unlimited local) + GUI PySide6/CustomTkinter + PyInstaller exe
RAG: `rag/ingest.py:1` + `rag/query.py:1` -> `chroma_db/` | Embed: `all-MiniLM-L6-v2` | Docs: `docs/rag/witp-ae/*.md` + `docs/DEVELOPMENT_BASELINE.md:1`
Versioning: `VERSION:1` = 0.01 - ONLY increment on user instruction. Commit format `[v0.01] msg`. Tag `v0.01`
Documentation: VIGOROUS - Google docstrings for all funcs/classes, file headers, inline why-comments, CHANGELOG.md, `file_path:line_number` refs
Launch blockers: `PRE_LAUNCH_CHECKLIST.md:1` (turn files, splash screen, WITP:AE folder detection, manuals)
