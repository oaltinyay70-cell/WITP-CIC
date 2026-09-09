"""
Purpose: Ingests markdown and text files into ChromaDB for RAG.
Version: 0.01
Author: Assistant / Antigravity
Dependencies: chromadb, sentence-transformers

This script reads documents from docs/rag/ and ingests them into a local
ChromaDB instance using sentence-transformers for local, free embeddings.
"""

import os
from pathlib import Path
import chromadb
from chromadb.utils import embedding_functions

# Config
DOCS_DIR = Path(__file__).resolve().parent.parent / "docs" / "rag"
DB_PATH = Path(__file__).resolve().parent.parent / "chroma_db"
COLLECTION_NAME = "witp_ae_rag"
EMBED_MODEL = "all-MiniLM-L6-v2"
CHUNK_SIZE = 1200
CHUNK_OVERLAP = 200

def chunk_text(text: str, size: int = CHUNK_SIZE, overlap: int = CHUNK_OVERLAP) -> list[str]:
    """
    Chunks text by major headings and length.

    Args:
        text: The text to chunk.
        size: Maximum length of a chunk.
        overlap: Overlap between fallback chunks.

    Returns:
        A list of string chunks.

    Raises:
        None

    Example:
        >>> chunks = chunk_text("Some long text...", 1200, 200)
    """
    chunks = []
    # Split by section headers (e.g. 1.0, 2.0 or ##)
    # The manual uses "1.0 ", "2.0 ", etc., but fallback to fixed window is safest for unstructured text
    for i in range(0, len(text), size - overlap):
        chunk = text[i:i+size].strip()
        if len(chunk) > 100:
            chunks.append(chunk)
    return chunks

def main() -> None:
    """
    Main function to execute the ingestion process.

    Args:
        None

    Returns:
        None

    Raises:
        Exception: If ChromaDB or file reading fails.

    Example:
        >>> main()
    """
    print(f"Ingesting from {DOCS_DIR} -> {DB_PATH} using {EMBED_MODEL}")
    if not DOCS_DIR.exists():
        print(f"Docs dir not found: {DOCS_DIR}")
        return

    client = chromadb.PersistentClient(path=str(DB_PATH))
    ef = embedding_functions.SentenceTransformerEmbeddingFunction(model_name=EMBED_MODEL)

    try:
        client.delete_collection(COLLECTION_NAME)
    except Exception:
        pass
    
    collection = client.get_or_create_collection(name=COLLECTION_NAME, embedding_function=ef)

    docs = list(DOCS_DIR.glob("**/*.md")) + list(DOCS_DIR.glob("**/*.txt"))
    docs = sorted(set(docs))
    
    if not docs:
        print("No documents found to ingest.")
        return

    total = 0
    BATCH = 64
    for md in docs:
        try:
            text = md.read_text(encoding="utf-8", errors="ignore")
        except Exception as e:
            print(f"Failed to read {md.name}: {e}")
            continue
            
        if text and text[0] == "\ufeff":
            text = text[1:]
            
        chunks = chunk_text(text)
        print(f"{md.name}: {len(chunks)} chunks ({md.stat().st_size} bytes)")
        
        for i in range(0, len(chunks), BATCH):
            batch_chunks = chunks[i:i+BATCH]
            batch_ids = [f"{md.stem}_{j}" for j in range(i, i+len(batch_chunks))]
            batch_metas = [{"source": str(md.name), "chunk": j} for j in range(i, i+len(batch_chunks))]
            collection.add(ids=batch_ids, documents=batch_chunks, metadatas=batch_metas)
            total += len(batch_chunks)
            print(f"  -> {total} total, batch {i//BATCH+1}/{(len(chunks)+BATCH-1)//BATCH}")

    print(f"Done. Total chunks ingested: {total}")

if __name__ == "__main__":
    main()
