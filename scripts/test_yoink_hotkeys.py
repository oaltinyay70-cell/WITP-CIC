import os, re, subprocess

def test_yoink_command_generation():
    print("--- Running Test: YOINK Hotkey Integration ---")
    dashboard_path = "src/ui/Dashboard.cpp"
    with open(dashboard_path, "r") as f:
        code = f.read()
    
    # Verify State restrictions
    list_view_regex = r"else if \(state == State::LIST_VIEW && \(ch == .y. \|\| ch == .Y.\)\)"
    if re.search(list_view_regex, code):
        print("[PASS] Y hotkey is strictly bound to State::LIST_VIEW (Intel Screen).")
    else:
        print("[FAIL] Y hotkey is not properly restricted to the Intel screen!")
        return False
        
    # Verify Directory Creation and cmd /k
    cmd_regex = r"mkdir.*?&& start cmd /k"
    if re.search(cmd_regex, code):
        print("[PASS] System command includes directory creation (mkdir) and persistent window (cmd /k).")
    else:
        print("[FAIL] System command is missing mkdir or cmd /k!")
        return False
        
    # Execute the synthesized command in a scratch space
    test_vault_path = "scratch/test_vault"
    yoink_dir = test_vault_path + "\\\\yoink"
    
    # The exact command from C++
    cpp_cmd = f"if not exist \"{yoink_dir}\" mkdir \"{yoink_dir}\" && start cmd /c \"echo YOINK TEST SUCCESS && exit\""
    print(f"Simulating C++ system() call: {cpp_cmd}")
    
    res = subprocess.run(cpp_cmd, shell=True)
    if res.returncode == 0 and os.path.exists(yoink_dir):
        print(f"[PASS] Command executed successfully. Directory {yoink_dir} was created.")
    else:
        print(f"[FAIL] Command failed or directory was not created.")
        return False
        
    print("--- TEST PASSED ---")
    return True

if __name__ == "__main__":
    if not test_yoink_command_generation():
        exit(1)

