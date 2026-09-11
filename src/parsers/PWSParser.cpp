#include "PWSParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace parsers {

    static std::string extractJsonField(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\": \"";
        size_t start = json.find(pattern);
        if (start == std::string::npos) return "";
        start += pattern.length();
        
        std::string result;
        for (size_t i = start; i < json.length(); ++i) {
            if (json[i] == '\\' && i + 1 < json.length()) {
                char next = json[i + 1];
                if (next == 'n') { result += '\n'; ++i; continue; }
                else if (next == 'r') { result += '\r'; ++i; continue; }
                else if (next == 't') { result += '\t'; ++i; continue; }
                else if (next == '"') { result += '"'; ++i; continue; }
                else if (next == '\\') { result += '\\'; ++i; continue; }
            }
            if (json[i] == '"') break;
            result += json[i];
        }
        return result;
    }

    PWSExtract PWSParser::parse(const std::string& filepath) {
        PWSExtract ext;
        if (!fs::exists(filepath)) return ext;

        // Create temporary output json path
        fs::create_directories("scratch");
        std::string temp_json = "scratch/temp_pws_parsed.json";

        // Invoke python extractor
        std::string cmd = "python ai/pws_extractor.py \"" + filepath + "\" \"" + temp_json + "\" > nul 2>&1";
        int res = std::system(cmd.c_str());
        if (res != 0) {
            // Try py launcher fallback
            cmd = "py ai/pws_extractor.py \"" + filepath + "\" \"" + temp_json + "\" > nul 2>&1";
            std::system(cmd.c_str());
        }

        if (!fs::exists(temp_json)) return ext;

        std::ifstream file(temp_json);
        if (!file) return ext;

        std::stringstream ss;
        ss << file.rdbuf();
        std::string json = ss.str();
        file.close();

        ext.header.format = extractJsonField(json, "format");
        ext.header.recipient = extractJsonField(json, "recipient");
        ext.header.game_date = extractJsonField(json, "game_date");
        ext.header.timestamp = extractJsonField(json, "timestamp");
        ext.header.game_version = extractJsonField(json, "game_version");
        ext.header.scenario = extractJsonField(json, "scenario");
        ext.after_action_report = extractJsonField(json, "after_action_report");
        ext.sigint_report = extractJsonField(json, "sigint_report");

        // Extract ship names
        size_t ships_start = json.find("\"ship_names\": [");
        if (ships_start != std::string::npos) {
            size_t ships_end = json.find("]", ships_start);
            if (ships_end != std::string::npos) {
                std::string list_str = json.substr(ships_start, ships_end - ships_start);
                size_t p = 0;
                while ((p = list_str.find("\"", p)) != std::string::npos) {
                    size_t next_q = list_str.find("\"", p + 1);
                    if (next_q == std::string::npos) break;
                    std::string item = list_str.substr(p + 1, next_q - p - 1);
                    if (item != "ship_names" && item.length() >= 3) {
                        ext.ship_names.push_back(item);
                    }
                    p = next_q + 1;
                }
            }
        }

        return ext;
    }
}
