# -*- coding: utf-8 -*-
"""
M.U.T.H.U.R AI Staff Officer Sidecar
Supports:
1. Google Gemini (Cloud API via google-genai)
2. Local Models (Ollama, LM Studio via standard OpenAI-compatible HTTP API)
3. Custom OpenAI-compatible endpoints
"""
import sys
import json
import os
import glob
import urllib.request
import urllib.error

def send_response(data):
    try:
        sys.stdout.write(json.dumps(data) + '\n')
        sys.stdout.flush()
    except Exception:
        pass

CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ai_config.json")

def load_config():
    defaults = {
        "provider": "gemini",
        "gemini_api_key": "",
        "gemini_model": "gemini-2.0-flash",
        "local_endpoint": "http://localhost:11434/v1",
        "local_model": "llama3.2",
        "openai_api_key": "",
        "openai_endpoint": "https://api.openai.com/v1",
        "openai_model": "gpt-4o-mini"
    }
    if os.path.exists(CONFIG_PATH):
        try:
            with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
                data = json.load(f)
                defaults.update(data)
        except Exception:
            pass
    return defaults

def save_config(cfg):
    try:
        with open(CONFIG_PATH, 'w', encoding='utf-8') as f:
            json.dump(cfg, f, indent=2)
    except Exception:
        pass

def get_gemini_api_key(cfg):
    if cfg.get("gemini_api_key"):
        return cfg["gemini_api_key"].strip()
    if "GEMINI_API_KEY" in os.environ:
        return os.environ["GEMINI_API_KEY"].strip()
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    paths_to_check = [
        "ai/api_key.txt",
        os.path.join(script_dir, "api_key.txt")
    ]
    for path in paths_to_check:
        if os.path.exists(path):
            try:
                with open(path, 'r', encoding='utf-8') as f:
                    k = f.read().strip()
                    if k:
                        return k
            except Exception:
                pass
    return ""

SYSTEM_PROMPT = """You are MUTHR (Military Unit Tactical Helper & Resource), a World War 2 Pacific Theater 
intelligence staff officer assigned to the Allied Combined Intelligence Center.

You have access to operational reports, SIGINT intercepts, after action reports, and ship 
tracking data for this campaign. Your role:
- Analyze incoming intelligence and highlight threats
- Suggest strategic responses to Japanese movements  
- Track Japanese carrier and battleship movements across turns
- Warn about potential invasion targets based on SIGINT patterns
- Answer questions about unit positions, ship statuses, and battle outcomes

Always ground your analysis in the actual data provided. Be concise and military in tone.
Use proper naval terminology. Address the user as "Commander"."""

def load_context(vault_path):
    if not vault_path:
        return ""
    context = []
    def safe_read(file_path):
        if os.path.exists(file_path):
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    return f.read()
            except Exception:
                return ""
        return ""

    camp_json = os.path.join(vault_path, "campaign.json")
    if os.path.exists(camp_json):
        context.append(f"Campaign Data:\n{safe_read(camp_json)}")
        
    for p in ["intel/*.json", "sigint/*.json", "reports/*.txt"]:
        for f in sorted(glob.glob(os.path.join(vault_path, p)), reverse=True)[:5]:
            content = safe_read(f)
            if content:
                # Limit each file to 4000 chars to fit context window
                if len(content) > 4000:
                    content = content[:4000] + "\n... [truncated]"
                context.append(f"File {os.path.basename(f)}:\n{content}")
                
    return "\n\n".join(context)

def call_local_model(endpoint, model_name, messages):
    """Calls a local OpenAI-compatible endpoint (Ollama / LM Studio) using stdlib urllib."""
    url = endpoint.rstrip('/')
    if not url.endswith('/chat/completions'):
        url += '/chat/completions'
        
    payload = {
        "model": model_name,
        "messages": messages,
        "stream": False,
        "temperature": 0.3
    }
    
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"}
    )
    
    try:
        with urllib.request.urlopen(req, timeout=45) as resp:
            body = resp.read().decode('utf-8')
            res = json.loads(body)
            choices = res.get("choices", [])
            if choices and "message" in choices[0]:
                return choices[0]["message"].get("content", "")
            return body
    except urllib.error.URLError as e:
        raise RuntimeError(f"Cannot reach local model at {endpoint}. Ensure Ollama or LM Studio is running. (e.g. 'ollama run {model_name}')")

def main():
    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
                
            cmd = json.loads(line.strip())
            cmd_type = cmd.get("type")
            cfg = load_config()
            
            if cmd_type == "shutdown":
                break
                
            elif cmd_type == "test":
                provider = cfg.get("provider", "gemini")
                if provider == "local":
                    endpoint = cfg.get("local_endpoint", "http://localhost:11434/v1")
                    model = cfg.get("local_model", "llama3.2")
                    try:
                        resp = call_local_model(endpoint, model, [{"role": "user", "content": "Ping"}])
                        send_response({"type": "response", "text": f"SUCCESS: Connected to local model '{model}' at {endpoint}.", "done": True})
                    except Exception as e:
                        send_response({"type": "error", "text": f"Local Connection Error: {str(e)}"})
                else:
                    key = get_gemini_api_key(cfg)
                    if not key:
                        send_response({"type": "error", "text": "Gemini API key is not configured. Open SETTINGS to enter it."})
                    else:
                        try:
                            from google import genai
                            client = genai.Client(api_key=key)
                            model = cfg.get("gemini_model", "gemini-2.0-flash")
                            r = client.models.generate_content(model=model, contents="Say 'Ready, Commander.'")
                            send_response({"type": "response", "text": f"SUCCESS: Connected to Gemini ({model}). Response: {r.text.strip()}", "done": True})
                        except Exception as e:
                            send_response({"type": "error", "text": f"Gemini API Error: {str(e)}"})

            elif cmd_type == "chat":
                vault_path = cmd.get("vault_path", "")
                msg = cmd.get("message", "")
                context = load_context(vault_path)
                provider = cfg.get("provider", "gemini")
                
                if provider == "local":
                    endpoint = cfg.get("local_endpoint", "http://localhost:11434/v1")
                    model = cfg.get("local_model", "llama3.2")
                    messages = [
                        {"role": "system", "content": SYSTEM_PROMPT},
                        {"role": "user", "content": f"CAMPAIGN CONTEXT:\n{context}\n\nCOMMANDER'S QUERY:\n{msg}"}
                    ]
                    try:
                        reply = call_local_model(endpoint, model, messages)
                        send_response({"type": "response", "text": reply, "done": True})
                    except Exception as e:
                        send_response({"type": "error", "text": str(e)})
                else:
                    # Google Gemini
                    key = get_gemini_api_key(cfg)
                    if not key:
                        send_response({
                            "type": "error",
                            "text": "Gemini API key is not configured.\nGo to SETTINGS -> AI MODEL CONFIGURATION in M.U.T.H.U.R to enter your key or switch to a Local Model (Ollama).\n(Get a free key from https://aistudio.google.com)"
                        })
                        continue
                        
                    try:
                        from google import genai
                    except ImportError:
                        send_response({"type": "error", "text": "google-genai library missing. Run: pip install google-genai"})
                        continue
                        
                    try:
                        client = genai.Client(api_key=key)
                        model = cfg.get("gemini_model", "gemini-2.0-flash")
                        full_contents = f"{SYSTEM_PROMPT}\n\nCAMPAIGN CONTEXT:\n{context}\n\nCOMMANDER'S QUERY:\n{msg}"
                        
                        response = client.models.generate_content_stream(
                            model=model,
                            contents=full_contents
                        )
                        for chunk in response:
                            if chunk.text:
                                send_response({"type": "response", "text": chunk.text, "done": False})
                        send_response({"type": "response", "text": "", "done": True})
                    except Exception as e:
                        send_response({"type": "error", "text": f"Gemini API Error: {str(e)}"})

            elif cmd_type == "analyze":
                vault_path = cmd.get("vault_path", "")
                context = load_context(vault_path)
                provider = cfg.get("provider", "gemini")
                
                prompt = f"{SYSTEM_PROMPT}\n\nCAMPAIGN CONTEXT:\n{context}\n\nPlease analyze the latest intelligence and give 2-3 concise strategic recommendations."
                
                if provider == "local":
                    endpoint = cfg.get("local_endpoint", "http://localhost:11434/v1")
                    model = cfg.get("local_model", "llama3.2")
                    try:
                        reply = call_local_model(endpoint, model, [{"role": "user", "content": prompt}])
                        for line in reply.split('\n'):
                            if line.strip():
                                send_response({"type": "suggestion", "text": line.strip()})
                    except Exception as e:
                        send_response({"type": "error", "text": str(e)})
                else:
                    key = get_gemini_api_key(cfg)
                    if not key:
                        continue
                    try:
                        from google import genai
                        client = genai.Client(api_key=key)
                        model = cfg.get("gemini_model", "gemini-2.0-flash")
                        r = client.models.generate_content(model=model, contents=prompt)
                        if r.text:
                            for line in r.text.split('\n'):
                                if line.strip():
                                    send_response({"type": "suggestion", "text": line.strip()})
                    except Exception as e:
                        send_response({"type": "error", "text": str(e)})

        except Exception as e:
            send_response({"type": "error", "text": f"Staff Officer Error: {str(e)}"})

if __name__ == "__main__":
    main()
