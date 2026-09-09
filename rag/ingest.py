"""
Free Unlimited RAG Ingest - ChromaDB + SentenceTransformers (local, no API)
Usage: py rag/ingest.py
Watches docs/rag/*.md and ingests into ./chroma_db
"""
import os
from pathlib import Path
import chromadb
from chromadb.utils import embedding_functions

# Config
DOCS_DIR = Path(__file__).parent.parent / "docs" / "rag"
DB_PATH = Path(__file__).parent.parent / "chroma_db"
COLLECTION_NAME = "code_rag"
EMBED_MODEL = "all-MiniLM-L6-v2"  # free, local, 384 dims, unlimited. Alternative: nomic-embed-text via Ollama
CHUNK_SIZE = 1200
CHUNK_OVERLAP = 200

def chunk_markdown(text, size=CHUNK_SIZE, overlap=CHUNK_OVERLAP):
    # Simple header-aware chunking: split by ## then by size
    chunks = []
    sections = text.split("\n## ")
    for sec in sections:
        sec = sec.strip()
        if not sec:
            continue
        # Ensure header marker
        if not sec.startswith("#"):
            sec = "## " + sec
        # Further split if too long
        for i in range(0, len(sec), size - overlap):
            chunk = sec[i:i+size]
            if len(chunk.strip()) > 100:
                chunks.append(chunk.strip())
    if not chunks:
        # fallback: fixed window
        for i in range(0, len(text), size - overlap):
            chunks.append(text[i:i+size])
    return chunks

def main():
    print(f"Ingesting from {DOCS_DIR} -> {DB_PATH} using {EMBED_MODEL}")
    if not DOCS_DIR.exists():
        print(f"Docs dir not found: {DOCS_DIR}")
        return

    client = chromadb.PersistentClient(path=str(DB_PATH))
    # Use local SentenceTransformer - fully free, unlimited, offline
    ef = embedding_functions.SentenceTransformerEmbeddingFunction(model_name=EMBED_MODEL)

    # Get or create collection (delete old for re-ingest)
    try:
        client.delete_collection(COLLECTION_NAME)
    except:
        pass
    collection = client.get_or_create_collection(name=COLLECTION_NAME, embedding_function=ef)

    # Support .md and .txt, recursive (docs/rag/witp-ae/*.md)
    md_files = list(DOCS_DIR.glob("*.md")) + list(DOCS_DIR.glob("**/*.md")) + list(DOCS_DIR.glob("**/*.txt"))
    # dedupe
    md_files = sorted(set(md_files))
    if not md_files:
        print("No .md files found")
        return

    total = 0
    BATCH = 64
    for md in md_files:
        # Skip huge files > 5MB to avoid OOM (the .md export with images is 41MB)
        if md.stat().st_size > 5_000_000:
            print(f"Skipping {md.name}: {md.stat().st_size} bytes > 5MB")
            continue
        try:
            text = md.read_text(encoding="utf-8")
        except:
            text = md.read_text(encoding="utf-8", errors="ignore")
        # strip BOM
        if text and text[0] == "\ufeff":
            text = text[1:]
        chunks = chunk_markdown(text)
        print(f"{md.name}: {len(chunks)} chunks ({md.stat().st_size} bytes)")
        # Batch add for speed + progress
        for i in range(0, len(chunks), BATCH):
            batch_chunks = chunks[i:i+BATCH]
            batch_ids = [f"{md.stem}_{j}" for j in range(i, i+len(batch_chunks))]
            batch_metas = [{"source": str(md.name), "chunk": j} for j in range(i, i+len(batch_chunks))]
            collection.add(ids=batch_ids, documents=batch_chunks, metadatas=batch_metas)
            total += len(batch_chunks)
            print(f"  -> {total} total, batch {i//BATCH+1}/{(len(chunks)+BATCH-1)//BATCH}")

    print(f"Done. Total chunks: {total}")
    print(f"Test query:")
    results = collection.query(query_texts=["project architecture"], n_results=2)
    for doc in results["documents"][0]:
        print(" -", doc[:200].replace("\n"," "))

if __name__ == "__main__":
    main()
