import sys
import json
import os
import glob
import traceback

def send_response(data):
    try:
        sys.stdout.write(json.dumps(data) + '\n')
        sys.stdout.flush()
    except Exception:
        pass

try:
    from google import genai
except ImportError:
    send_response({"type": "error", "text": "google-genai package not found. Please run: pip install google-genai"})
    sys.exit(1)

def get_api_key():
    if "GEMINI_API_KEY" in os.environ:
        return os.environ["GEMINI_API_KEY"]
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    paths_to_check = [
        "ai/api_key.txt",
        os.path.join(script_dir, "api_key.txt")
    ]
    
    for path in paths_to_check:
        if os.path.exists(path):
            with open(path, 'r', encoding='utf-8') as f:
                key = f.read().strip()
                if key:
                    return key
    return None

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
        for f in glob.glob(os.path.join(vault_path, p)):
            content = safe_read(f)
            if content:
                context.append(f"File {os.path.basename(f)}:\n{content}")
                
    return "\n\n".join(context)

def main():
    api_key = get_api_key()
    if not api_key:
        send_response({"type": "error", "text": "GEMINI_API_KEY not found. Set environment variable or create ai/api_key.txt"})
        return
        
    client = genai.Client(api_key=api_key)
    
    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
                
            cmd = json.loads(line.strip())
            cmd_type = cmd.get("type")
            
            if cmd_type == "shutdown":
                break
                
            elif cmd_type == "chat":
                vault_path = cmd.get("vault_path", "")
                msg = cmd.get("message", "")
                
                context = load_context(vault_path)
                full_contents = f"{SYSTEM_PROMPT}\n\nCONTEXT:\n{context}\n\nCOMMANDER:\n{msg}"
                
                try:
                    response = client.models.generate_content_stream(
                        model='gemini-2.0-flash',
                        contents=full_contents
                    )
                    
                    for chunk in response:
                        if chunk.text:
                            send_response({"type": "response", "text": chunk.text, "done": False})
                    
                    send_response({"type": "response", "text": "", "done": True})
                except Exception as e:
                    send_response({"type": "error", "text": f"API Error: {str(e)}"})
                    
            elif cmd_type == "analyze":
                vault_path = cmd.get("vault_path", "")
                turn_data_path = cmd.get("turn_data_path", "")
                
                context = load_context(vault_path)
                turn_data = ""
                if turn_data_path and os.path.exists(turn_data_path):
                    with open(turn_data_path, 'r', encoding='utf-8') as f:
                        turn_data = f.read()
                        
                full_contents = f"{SYSTEM_PROMPT}\n\nCONTEXT:\n{context}\n\nTURN DATA:\n{turn_data}\n\nPlease analyze this turn data and provide 2-3 proactive suggestions. Return each suggestion on a new line."
                
                try:
                    response = client.models.generate_content(
                        model='gemini-2.0-flash',
                        contents=full_contents
                    )
                    
                    if response.text:
                        suggestions = response.text.split('\n')
                        for sugg in suggestions:
                            if sugg.strip():
                                send_response({"type": "suggestion", "text": sugg.strip()})
                except Exception as e:
                    send_response({"type": "error", "text": f"API Error: {str(e)}"})
                    
        except Exception as e:
            send_response({"type": "error", "text": f"System Error: {str(e)}"})

if __name__ == "__main__":
    main()
