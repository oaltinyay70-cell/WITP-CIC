"""
Free Unlimited RAG Query - ChromaDB local
Usage: py rag/query.py "your question"
"""
import sys
from pathlib import Path
import chromadb
from chromadb.utils import embedding_functions

DB_PATH = Path(__file__).parent.parent / "chroma_db"
COLLECTION_NAME = "code_rag"
EMBED_MODEL = "all-MiniLM-L6-v2"

def query(q, n=3):
    client = chromadb.PersistentClient(path=str(DB_PATH))
    ef = embedding_functions.SentenceTransformerEmbeddingFunction(model_name=EMBED_MODEL)
    col = client.get_collection(name=COLLECTION_NAME, embedding_function=ef)
    res = col.query(query_texts=[q], n_results=n)
    print(f"\nQuery: {q}\n")
    for i, (doc, meta) in enumerate(zip(res["documents"][0], res["metadatas"][0])):
        print(f"[{i+1}] Source: {meta['source']} (chunk {meta['chunk']})")
        print(doc[:800])
        print("-"*60)
    return res

if __name__ == "__main__":
    q = " ".join(sys.argv[1:]) if len(sys.argv) > 1 else "project overview"
    query(q)
