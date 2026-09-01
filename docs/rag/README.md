# RAG Knowledge Base - Code Development

> Feed this file(s) directly to Muse Spark / OpenCode. All content is chunked at 500-800 tokens with H1/H2 boundaries for optimal retrieval.

## How to use with Muse Spark
1. Paste this file content here in chat, OR
2. Keep in `docs/rag/*.md` - automatically indexed by `ingest.py` into local ChromaDB (free, unlimited, offline)
3. Reference via `.opencode/context.md` for auto-loading

---

## 1. Project Overview

**Project Name:** [YOUR_PROJECT]
**Tech Stack:** [e.g., Python 3.14, React, Postgres, FastAPI]
**Repository:** [path]
**Purpose:** [1-2 lines what it does]

**Architecture:**
```mermaid
[Frontend] -> [API] -> [DB]
```

---

## 2. File Map & Conventions

| Path | Purpose | Key Exports |
|------|---------|-------------|
| `src/api/main.py:1` | FastAPI entry | `app`, `lifespan` |
| `src/core/utils.py:1` | Helpers | `parseConfig()` |
| `docs/rag/README.md:1` | RAG source | - |

**Naming:** `snake_case` for python, `kebab-case` for files
**Style:** Black formatter, ruff lint

---

## 3. API Contracts

### Endpoint: `POST /api/v1/...`
Request:
```json
{}
```
Response:
```json
{}
```

---

## 4. Database Schema

```sql
CREATE TABLE users (id UUID PRIMARY KEY, ...);
```

---

## 5. Key Workflows

### Workflow: User Registration
1. `src/api/routes/auth.py:10` - `register()`
2. `src/core/auth.py:25` - `hashPassword()`
3. `src/db/models.py:40` - `User.create()`

Error handling: [describe]

---

## 6. Common Pitfalls & Rules

- NEVER do X, always do Y
- Env vars required: `DATABASE_URL`, `SECRET_KEY`
- Tests: `py -m pytest`

---

## 7. Glossary

| Term | Definition |
|------|------------|
| RAG | Retrieval Augmented Generation |

---

## 8. Code Snippets (Reusable)

```python
# src/core/example.py:1
def example():
    pass
```
