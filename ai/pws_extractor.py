# -*- coding: utf-8 -*-
"""
M.U.T.H.U.R PWS Binary Extractor CLI
Extracts header, AAR, SIGINT, and ships from a .pws file and writes JSON output.
Usage: python pws_extractor.py <input.pws> <output.json>
"""
import sys
import os
import gzip
import zlib
import json

def extract_pws(pws_path, output_json_path):
    result = {
        "format": "",
        "recipient": "",
        "game_date": "",
        "timestamp": "",
        "game_version": "",
        "scenario": "",
        "after_action_report": "",
        "sigint_report": "",
        "ship_names": []
    }
    
    if not os.path.exists(pws_path):
        with open(output_json_path, "w", encoding="utf-8") as f:
            json.dump(result, f)
        return

    try:
        with gzip.open(pws_path, 'rb') as f:
            data = f.read()
    except Exception as e:
        with open(output_json_path, "w", encoding="utf-8") as f:
            json.dump(result, f)
        return

    # Decompress zlib chunks
    chunks = []
    i = 0
    while i < len(data) - 1:
        if data[i] == 0x78 and (data[i+1] == 0xDA or data[i+1] == 0x9C):
            try:
                dobj = zlib.decompressobj()
                dec = dobj.decompress(data[i:])
                consumed = len(data[i:]) - len(dobj.unused_data)
                chunks.append(dec)
                i += consumed
                continue
            except zlib.error:
                pass
        i += 1

    def extract_strings(raw_bytes, min_len=4):
        strs = []
        curr = bytearray()
        for b in raw_bytes:
            if 32 <= b <= 126:
                curr.append(b)
            else:
                if len(curr) >= min_len:
                    strs.append(curr.decode('ascii', errors='replace'))
                curr = bytearray()
        if len(curr) >= min_len:
            strs.append(curr.decode('ascii', errors='replace'))
        return strs

    # Chunk 0: Header
    if len(chunks) > 0:
        h_strs = extract_strings(chunks[0], 3)
        for s in h_strs:
            if s.startswith("(PBEM"):
                result["format"] = s
            elif "incoming" in s.lower() or "incoming to" in s.lower():
                result["recipient"] = s
            elif "/" in s and len(s) <= 10:
                result["game_date"] = s
            elif any(day in s for day in ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]):
                result["timestamp"] = s
            elif s.startswith("1.8."):
                result["game_version"] = s

    # Chunk 1: Scenario
    if len(chunks) > 1:
        scen_strs = extract_strings(chunks[1], 4)
        if scen_strs:
            result["scenario"] = scen_strs[0]

    # Chunk 6: Ships
    if len(chunks) > 6:
        raw_ships = extract_strings(chunks[6], 4)
        # Filter valid ship names (alphabetical, reasonable length)
        clean_ships = [s for s in raw_ships if len(s) >= 3 and any(c.isalpha() for c in s)]
        result["ship_names"] = clean_ships[:500]  # Store top 500 capital ships / units

    # Last large chunk (> 10KB): AAR and SIGINT
    last_chunk = None
    for c in reversed(chunks):
        if len(c) > 10000:
            last_chunk = c
            break

    if last_chunk:
        # Extract full printable text with line breaks
        text = ""
        curr = bytearray()
        for b in last_chunk:
            if 32 <= b <= 126 or b in (10, 13):
                curr.append(b)
            else:
                if curr:
                    text += curr.decode('ascii', errors='replace')
                    if b == 0:
                        text += "\n"
                    curr = bytearray()
        if curr:
            text += curr.decode('ascii', errors='replace')

        sigint_idx = text.find("SIG INT REPORT")
        if sigint_idx == -1:
            sigint_idx = text.find("SIGINT REPORT")

        if sigint_idx != -1:
            result["after_action_report"] = text[:sigint_idx].strip()
            result["sigint_report"] = text[sigint_idx:].strip()
        else:
            result["after_action_report"] = text.strip()

    with open(output_json_path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)

if __name__ == "__main__":
    if len(sys.argv) >= 3:
        extract_pws(sys.argv[1], sys.argv[2])
    else:
        print("Usage: python pws_extractor.py <input.pws> <output.json>")
