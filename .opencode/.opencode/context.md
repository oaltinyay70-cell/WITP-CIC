# OpenCode Context - Auto-loaded as RAG

This file is automatically injected into every Muse Spark session.

Include high-level project knowledge here. Keep under 8000 tokens.

See `docs/rag/README.md` for full knowledge base.
See `docs/rag/*.md` for detailed indexed docs.

Project: [YOUR_PROJECT]
Stack: Python 3.14 + ChromaDB + SentenceTransformers (free, unlimited, local)
RAG Engine: `rag/ingest.py` + `rag/query.py` -> ChromaDB at `./chroma_db`
Embeddings: `all-MiniLM-L6-v2` (local, free, offline) - upgrade to `nomic-embed-text` via Ollama if installed
