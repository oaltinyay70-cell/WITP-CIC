#include "IntelligenceEngine.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

namespace engine {

    int parseDateToDays(const std::string& dateStr) {
        std::map<std::string, int> months = {
            {"Jan", 0}, {"Feb", 31}, {"Mar", 59}, {"Apr", 90}, {"May", 120}, {"Jun", 151},
            {"Jul", 181}, {"Aug", 212}, {"Sep", 243}, {"Oct", 273}, {"Nov", 304}, {"Dec", 334}
        };
        
        if (dateStr.length() < 10) return 0;
        std::string mon = dateStr.substr(0, 3);
        int day = 0, year = 0;
        try {
            day = std::stoi(dateStr.substr(4, 2));
            year = std::stoi(dateStr.substr(8, 2));
        } catch(...) { return 0; }
        
        int m_days = months.count(mon) ? months[mon] : 0;
        return (year * 365) + m_days + day;
    }

    bool containsIgnoreCase(std::string str, std::string sub) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        std::transform(sub.begin(), sub.end(), sub.begin(), ::tolower);
        return str.find(sub) != std::string::npos;
    }

    void IntelligenceEngine::processDirectory(const std::string& path) {
        int ops_min = 999999, ops_max = 0;
        int sigint_min = 999999, sigint_max = 0;
        int combat_min = 999999, combat_max = 0;

        if (!fs::exists(path)) return;

        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() != ".txt") continue;

            std::ifstream file(entry.path());
            std::string line;
            
            bool is_valid_report = false;
            int file_type = 0; 
            int days = 0;
            std::string dateStr = "Unknown";

            // Check first 5 lines for a valid header
            for(int i=0; i<5; ++i) {
                if (!std::getline(file, line)) break;
                if (containsIgnoreCase(line, "ERATIONAL REPORT FOR")) {
                    file_type = 1; is_valid_report = true;
                    size_t pos = line.find("FOR ");
                    if (pos != std::string::npos && pos + 4 < line.length()) {
                        dateStr = line.substr(pos + 4);
                        days = parseDateToDays(dateStr);
                        if (days > 0) {
                            if (days < ops_min) ops_min = days;
                            if (days > ops_max) ops_max = days;
                            stats.turn_date = dateStr;
                        }
                    }
                    break;
                } else if (containsIgnoreCase(line, "G INT REPORT FOR") || containsIgnoreCase(line, "SIGINT REPORT FOR")) {
                    file_type = 2; is_valid_report = true;
                    size_t pos = line.find("FOR ");
                    if (pos != std::string::npos && pos + 4 < line.length()) {
                        dateStr = line.substr(pos + 4);
                        days = parseDateToDays(dateStr);
                        if (days > 0) {
                            if (days < sigint_min) sigint_min = days;
                            if (days > sigint_max) sigint_max = days;
                            stats.turn_date = dateStr;
                        }
                    }
                    break;
                } else if (containsIgnoreCase(line, "COMBAT REPORT") || containsIgnoreCase(line, "AFTER ACTION")) {
                    file_type = 3; is_valid_report = true;
                    break;
                }
            }

            if (!is_valid_report) continue;

            while (std::getline(file, line)) {
                if (line.length() < 5) continue;
                
                if (containsIgnoreCase(line, "loaded on") || 
                    containsIgnoreCase(line, "moving to") || 
                    containsIgnoreCase(line, "Invasion") ||
                    containsIgnoreCase(line, "Amphibious")) {
                    
                    IntelItem item;
                    item.title = "> [CRITICAL] " + line.substr(0, std::min(line.length(), (size_t)60)) + (line.length()>60?"...":"");
                    item.reasoning = (file_type == 1 ? "OPS REPORT: " : (file_type == 2 ? "SIGINT REPORT: " : "COMBAT REPORT: ")) + line;
                    item.solidity = (file_type == 2) ? 80 : 95; 
                    item.category = IntelCategory::CRITICAL;
                    items.push_back(item);
                    continue; 
                }

                // Discovered Allied Units
                if (containsIgnoreCase(line, "sighted over") || 
                    containsIgnoreCase(line, "shadowed by") || 
                    containsIgnoreCase(line, "sighted by") ||
                    containsIgnoreCase(line, "observes Japanese")) {
                    
                    IntelItem item;
                    item.title = "> [DISCOVERED] " + line.substr(0, std::min(line.length(), (size_t)65)) + (line.length()>65?"...":"");
                    item.reasoning = (file_type == 1 ? "OPS REPORT: " : (file_type == 2 ? "SIGINT REPORT: " : "COMBAT REPORT: ")) + line;
                    item.solidity = (file_type == 2) ? 60 : 100; 
                    item.category = IntelCategory::DISCOVERED;
                    items.push_back(item);
                    continue;
                }

                if (containsIgnoreCase(line, " CV ") || containsIgnoreCase(line, " BB ") || 
                    containsIgnoreCase(line, " CA ") || containsIgnoreCase(line, " CVL ") ||
                    containsIgnoreCase(line, " CVE ") || containsIgnoreCase(line, "Tanker") ||
                    containsIgnoreCase(line, "Division") || containsIgnoreCase(line, "Brigade") ||
                    containsIgnoreCase(line, "Regiment") || containsIgnoreCase(line, " TF ") || 
                    containsIgnoreCase(line, "Command") || containsIgnoreCase(line, "Corps")) {
                    
                    IntelItem item;
                    item.title = "> [HVT] " + line.substr(0, std::min(line.length(), (size_t)65)) + (line.length()>65?"...":"");
                    item.reasoning = (file_type == 1 ? "OPS REPORT: " : (file_type == 2 ? "SIGINT REPORT: " : "COMBAT REPORT: ")) + line;
                    item.solidity = (file_type == 2) ? 60 : 90; 
                    item.category = IntelCategory::HVT;
                    items.push_back(item);
                }
            }
        }

        stats.ops_days = (ops_max >= ops_min) ? (ops_max - ops_min + 1) : 0;
        stats.sigint_days = (sigint_max >= sigint_min) ? (sigint_max - sigint_min + 1) : 0;
        stats.combat_days = (combat_max >= combat_min) ? (combat_max - combat_min + 1) : 0;
    }
}