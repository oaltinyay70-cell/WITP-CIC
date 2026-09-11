#include "StaffOfficer.hpp"
#include <windows.h>
#include <iostream>

namespace ai {

StaffOfficer::StaffOfficer() {}

StaffOfficer::~StaffOfficer() {
    shutdown();
}

bool StaffOfficer::start(const std::string& vault_path) {
    if (isRunning()) return true;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) return false;
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) return false;

    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) return false;
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) return false;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hChildStd_OUT_Wr;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.hStdInput = hChildStd_IN_Rd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    // Try starting python with various commands
    std::wstring cmd = L"python ai/staff_officer.py";
    
    // Command line needs to be mutable for CreateProcessW
    std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back(0);

    if (!CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::wstring cmd3 = L"python3 ai/staff_officer.py";
        std::vector<wchar_t> cmdBuffer3(cmd3.begin(), cmd3.end());
        cmdBuffer3.push_back(0);
        
        if (!CreateProcessW(NULL, cmdBuffer3.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            std::wstring pycmd = L"py ai/staff_officer.py";
            std::vector<wchar_t> cmdBufferPy(pycmd.begin(), pycmd.end());
            cmdBufferPy.push_back(0);
            
            if (!CreateProcessW(NULL, cmdBufferPy.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hChildStd_OUT_Rd);
                CloseHandle(hChildStd_OUT_Wr);
                CloseHandle(hChildStd_IN_Rd);
                CloseHandle(hChildStd_IN_Wr);
                return false;
            }
        }
    }

    hProcess = pi.hProcess;
    hThread = pi.hThread;
    hStdoutRead = hChildStd_OUT_Rd;
    hStdinWrite = hChildStd_IN_Wr;

    // Close the handles we don't need in the parent process
    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_IN_Rd);

    running = true;
    return true;
}

static std::string escapeJSON(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\n') output += "\\n";
        else if (c == '\r') output += "\\r";
        else if (c == '\t') output += "\\t";
        else output += c;
    }
    return output;
}

void StaffOfficer::sendMessage(const std::string& message, const std::string& vault_path) {
    if (!isRunning()) return;

    std::string jsonLine = "{\"type\":\"chat\",\"message\":\"" + escapeJSON(message) + "\",\"vault_path\":\"" + escapeJSON(vault_path) + "\"}\n";

    DWORD written;
    WriteFile((HANDLE)hStdinWrite, jsonLine.c_str(), jsonLine.length(), &written, NULL);
}

void StaffOfficer::requestAnalysis(const std::string& turn_data_path, const std::string& vault_path) {
    if (!isRunning()) return;
    
    std::string jsonLine = "{\"type\":\"analyze\",\"turn_data_path\":\"" + escapeJSON(turn_data_path) + "\",\"vault_path\":\"" + escapeJSON(vault_path) + "\"}\n";

    DWORD written;
    WriteFile((HANDLE)hStdinWrite, jsonLine.c_str(), jsonLine.length(), &written, NULL);
}

std::string StaffOfficer::readLine() {
    if (!isRunning()) return "";

    // Check if we already have a full line
    auto pos = readBuffer.find('\n');
    if (pos != std::string::npos) {
        std::string line = readBuffer.substr(0, pos);
        readBuffer.erase(0, pos + 1);
        return line;
    }

    // Check if there's any data to read
    DWORD bytesAvailable = 0;
    if (PeekNamedPipe((HANDLE)hStdoutRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
        char buf[4096];
        DWORD bytesToRead = (bytesAvailable < sizeof(buf) - 1) ? bytesAvailable : (sizeof(buf) - 1);
        DWORD bytesRead;
        
        if (ReadFile((HANDLE)hStdoutRead, buf, bytesToRead, &bytesRead, NULL) && bytesRead > 0) {
            buf[bytesRead] = '\0';
            readBuffer += buf;
            
            pos = readBuffer.find('\n');
            if (pos != std::string::npos) {
                std::string line = readBuffer.substr(0, pos);
                readBuffer.erase(0, pos + 1);
                return line;
            }
        }
    }
    return "";
}

bool StaffOfficer::isRunning() const {
    if (!running || hProcess == nullptr) return false;
    DWORD exitCode;
    if (GetExitCodeProcess((HANDLE)hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

void StaffOfficer::shutdown() {
    if (!isRunning()) {
        if (hStdinWrite) CloseHandle((HANDLE)hStdinWrite);
        if (hStdoutRead) CloseHandle((HANDLE)hStdoutRead);
        if (hThread) CloseHandle((HANDLE)hThread);
        if (hProcess) CloseHandle((HANDLE)hProcess);
        hStdinWrite = nullptr;
        hStdoutRead = nullptr;
        hThread = nullptr;
        hProcess = nullptr;
        running = false;
        return;
    }

    std::string jsonLine = "{\"type\":\"shutdown\"}\n";
    DWORD written;
    WriteFile((HANDLE)hStdinWrite, jsonLine.c_str(), jsonLine.length(), &written, NULL);

    if (WaitForSingleObject((HANDLE)hProcess, 3000) != WAIT_OBJECT_0) {
        TerminateProcess((HANDLE)hProcess, 1);
    }

    CloseHandle((HANDLE)hStdinWrite);
    CloseHandle((HANDLE)hStdoutRead);
    CloseHandle((HANDLE)hThread);
    CloseHandle((HANDLE)hProcess);
    
    hStdinWrite = nullptr;
    hStdoutRead = nullptr;
    hThread = nullptr;
    hProcess = nullptr;
    running = false;
}

} // namespace ai
