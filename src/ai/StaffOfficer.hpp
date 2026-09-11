#pragma once
#include <string>
#include <vector>
#include <functional>

namespace ai {
    class StaffOfficer {
    public:
        StaffOfficer();
        ~StaffOfficer();
        
        // Start the Python sidecar process
        bool start(const std::string& vault_path);
        
        // Send a chat message to the AI
        void sendMessage(const std::string& message, const std::string& vault_path);
        
        // Read one line of response (blocking with timeout)
        // Returns empty string if no data available
        std::string readLine();
        
        // Check if the sidecar is running
        bool isRunning() const;
        
        // Shutdown the sidecar
        void shutdown();
        
        // Request proactive analysis of current turn
        void requestAnalysis(const std::string& turn_data_path, const std::string& vault_path);
        
    private:
        void* hProcess = nullptr;  // HANDLE
        void* hThread = nullptr;   // HANDLE  
        void* hStdinWrite = nullptr;  // HANDLE - write end of stdin pipe
        void* hStdoutRead = nullptr;  // HANDLE - read end of stdout pipe
        bool running = false;
        
        std::string readBuffer;
    };
}
