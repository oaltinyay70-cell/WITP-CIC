#include "CampaignVault.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace fs = std::filesystem;

namespace storage {




    
    static std::string extractJsonField(const std::string& json, const std::string& key) {
        std::string pattern = "\"" + key + "\": \"";
        size_t start = json.find(pattern);
        if (start == std::string::npos) return "";
        start += pattern.length();
        std::string result;
        for (size_t i = start; i < json.length(); ++i) {
            if (json[i] == '"') break;
            result += json[i];
        }
        return result;
    }

    static std::string escapeJsonString(const std::string& input) {
        std::ostringstream ss;
        for (char c : input) {
            switch (c) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        ss << "\\u"
                           << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        ss << c;
                    }
            }
        }
        return ss.str();
    }
    
    


    static std::string currentIsoTime() {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", tm);
        return std::string(buffer);
    }

    std::vector<std::string> CampaignVault::listCampaigns(const std::string& vault_root) {
        std::vector<std::string> campaigns;
        if (!fs::exists(vault_root)) return campaigns;
        for (const auto& entry : fs::directory_iterator(vault_root)) {
            if (entry.is_regular_file() && entry.path().extension() == ".zip") {
                campaigns.push_back(entry.path().stem().string());
            }
        }
        return campaigns;
    }

    void CampaignVault::create(const std::string& vault_root_arg, const std::string& name) {
        vault_root = vault_root_arg;
        meta.name = name;
        meta.created_date = currentIsoTime();
        meta.last_played = meta.created_date;
        
        vault_path = (fs::path("scratch") / "active_campaign").string();
        if (fs::exists(vault_path)) {
            try { fs::remove_all(vault_path); } catch (...) {}
        }
        
        fs::create_directories(vault_path);
        fs::create_directories(fs::path(vault_path) / "intel");
        fs::create_directories(fs::path(vault_path) / "reports");
        fs::create_directories(fs::path(vault_path) / "sigint");
        fs::create_directories(fs::path(vault_path) / "pws");
        fs::create_directories(fs::path(vault_path) / "ships");
        
        saveMeta();
    }

    
    static std::string escapePsPath(const std::string& path) {
        std::string res;
        for (char ch : path) {
            if (ch == '\'') res += "''";
            else res += ch;
        }
        return res;
    }

    void CampaignVault::load(const std::string& vault_path_arg) { // vault_path_arg is the zip file path or name
        meta = CampaignMeta();
        fs::path zip_p = fs::path(vault_path_arg);
        if (zip_p.extension() != ".zip") zip_p = fs::path(vault_root) / (vault_path_arg + ".zip");
        
        vault_path = (fs::path("scratch") / "active_campaign").string();
        fs::create_directories(vault_path);
        
        if (fs::exists(zip_p)) {
            std::string command = "powershell -Command \"Expand-Archive -Path '" + escapePsPath(zip_p.string()) + "' -DestinationPath '" + escapePsPath(vault_path) + "' -Force\"";
            std::system(command.c_str());
        }

        fs::path meta_file = fs::path(vault_path) / "campaign.json";
        if (fs::exists(meta_file)) {
            std::ifstream ifs(meta_file);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            meta.name = extractJsonField(content, "name");
            meta.scenario = extractJsonField(content, "scenario");
            meta.game_version = extractJsonField(content, "game_version");
            meta.created_date = extractJsonField(content, "created_date");
            meta.last_played = extractJsonField(content, "last_played");
            
            // Parse source_dirs array
            size_t arr_start = content.find("\"source_dirs\": [");
            if (arr_start != std::string::npos) {
                size_t arr_end = content.find("]", arr_start);
                if (arr_end != std::string::npos) {
                    std::string arr_str = content.substr(arr_start, arr_end - arr_start);
                    size_t pos = 0;
                    while ((pos = arr_str.find("\"", pos)) != std::string::npos) {
                        size_t end_pos = arr_str.find("\"", pos + 1);
                        if (end_pos != std::string::npos) {
                            meta.source_dirs.push_back(arr_str.substr(pos + 1, end_pos - pos - 1));
                            pos = end_pos + 1;
                        } else {
                            break;
                        }
                    }
                }
            }

        }
    }

    void CampaignVault::saveMeta() {
        fs::path meta_file = fs::path(vault_path) / "campaign.json";
        std::ofstream ofs(meta_file);
        ofs << "{\n"
            << "  \"name\": \"" << escapeJsonString(meta.name) << "\",\n"
            << "  \"scenario\": \"" << escapeJsonString(meta.scenario) << "\",\n"
            << "  \"game_version\": \"" << escapeJsonString(meta.game_version) << "\",\n"
            << "  \"created_date\": \"" << escapeJsonString(meta.created_date) << "\",\n"
            << "  \"last_played\": \"" << escapeJsonString(meta.last_played) << "\",\n"
            << "  \"source_dirs\": [";
        for (size_t i = 0; i < meta.source_dirs.size(); ++i) {
            ofs << "\"" << escapeJsonString(meta.source_dirs[i]) << "\"";
            if (i < meta.source_dirs.size() - 1) ofs << ", ";
        }
        ofs << "]\n}\n";
    }

    void CampaignVault::saveIntelItems(const std::string& turn_date, const std::vector<engine::IntelItem>& items) {
        fs::path intel_dir = fs::path(vault_path) / "intel";
        fs::create_directories(intel_dir);
        fs::path intel_file = intel_dir / ("turn_" + turn_date + ".json");
        std::ofstream ofs(intel_file);
        ofs << "[\n";
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            ofs << "  {\n"
                << "    \"title\": \"" << escapeJsonString(item.title) << "\",\n"
                << "    \"reasoning\": \"" << escapeJsonString(item.reasoning) << "\",\n"
                << "    \"solidity\": " << item.solidity << ",\n"
                << "    \"category\": " << static_cast<int>(item.category) << "\n"
                << "  }";
            if (i < items.size() - 1) ofs << ",";
            ofs << "\n";
        }
        ofs << "]\n";
    }

    std::vector<engine::IntelItem> CampaignVault::loadAllIntel() {
        std::vector<engine::IntelItem> all_items;
        fs::path intel_dir = fs::path(vault_path) / "intel";
        if (!fs::exists(intel_dir)) return all_items;

        for (const auto& entry : fs::directory_iterator(intel_dir)) {
            if (entry.path().extension() == ".json") {
                std::ifstream ifs(entry.path());
                std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                
                size_t pos = 0;
                while ((pos = content.find("{", pos)) != std::string::npos) {
                    size_t end_pos = content.find("}", pos);
                    if (end_pos == std::string::npos) break;
                    
                    std::string obj = content.substr(pos, end_pos - pos + 1);
                    engine::IntelItem item;
                    
                    size_t p = obj.find("\"title\"");
                    if (p != std::string::npos) {
                        size_t q1 = obj.find("\"", p + 7);
                        if(q1 != std::string::npos) {
                            size_t q2 = obj.find("\"", q1 + 1);
                            if (q1 != std::string::npos && q2 != std::string::npos) item.title = obj.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }
                    
                    p = obj.find("\"reasoning\"");
                    if (p != std::string::npos) {
                        size_t q1 = obj.find("\"", p + 11);
                        if(q1 != std::string::npos) {
                            size_t q2 = obj.find("\"", q1 + 1);
                            if (q1 != std::string::npos && q2 != std::string::npos) item.reasoning = obj.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }
                    
                    p = obj.find("\"solidity\"");
                    if (p != std::string::npos) {
                        size_t colon = obj.find(":", p);
                        size_t comma = obj.find(",", p);
                        if (colon != std::string::npos && comma != std::string::npos) {
                            item.solidity = std::stoi(obj.substr(colon + 1, comma - colon - 1));
                        }
                    }
                    
                    p = obj.find("\"category\"");
                    if (p != std::string::npos) {
                        size_t colon = obj.find(":", p);
                        size_t comma = obj.find_first_of(",\n\r}", p);
                        if (colon != std::string::npos && comma != std::string::npos) {
                            item.category = static_cast<engine::IntelCategory>(std::stoi(obj.substr(colon + 1, comma - colon - 1)));
                        }
                    }
                    
                    all_items.push_back(item);
                    pos = end_pos + 1;
                }
            }
        }
        return all_items;
    }

    void CampaignVault::saveReport(const std::string& filename, const std::string& content) {
        fs::path report_file = fs::path(vault_path) / "reports" / filename;
        std::ofstream ofs(report_file);
        ofs << content;
    }

    void CampaignVault::saveSIGINT(const std::string& turn_date, const std::vector<std::string>& entries) {
        fs::path sigint_dir = fs::path(vault_path) / "sigint";
        fs::path sigint_file = sigint_dir / ("sigint_" + turn_date + ".json");
        std::ofstream ofs(sigint_file);
        ofs << "[\n";
        for (size_t i = 0; i < entries.size(); ++i) {
            ofs << "  \"" << escapeJsonString(entries[i]) << "\"";
            if (i < entries.size() - 1) ofs << ",";
            ofs << "\n";
        }
        ofs << "]\n";
    }

    void CampaignVault::appendChat(const std::string& role, const std::string& text) {
        fs::path chat_file = fs::path(vault_path) / "chat_history.json";
        std::string new_entry = "  {\n    \"role\": \"" + escapeJsonString(role) + "\",\n    \"text\": \"" + escapeJsonString(text) + "\"\n  }";
        
        if (!fs::exists(chat_file)) {
            std::ofstream ofs(chat_file);
            ofs << "[\n" << new_entry << "\n]\n";
        } else {
            std::ifstream ifs(chat_file);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            
            size_t insert_pos = content.find_last_of("]");
            if (insert_pos != std::string::npos) {
                content.insert(insert_pos, ",\n" + new_entry + "\n");
                std::ofstream ofs(chat_file);
                ofs << content;
            }
        }
    }

    std::vector<std::pair<std::string,std::string>> CampaignVault::loadChatHistory() {
        std::vector<std::pair<std::string,std::string>> history;
        fs::path chat_file = fs::path(vault_path) / "chat_history.json";
        if (fs::exists(chat_file)) {
            std::ifstream ifs(chat_file);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            
            size_t pos = 0;
            while ((pos = content.find("{", pos)) != std::string::npos) {
                size_t end_pos = content.find("}", pos);
                if (end_pos == std::string::npos) break;
                
                std::string obj = content.substr(pos, end_pos - pos + 1);
                std::string role, text;
                
                size_t p = obj.find("\"role\"");
                if (p != std::string::npos) {
                    size_t q1 = obj.find("\"", p + 6);
                    if(q1 != std::string::npos) {
                        size_t q2 = obj.find("\"", q1 + 1);
                        if (q1 != std::string::npos && q2 != std::string::npos) role = obj.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
                
                p = obj.find("\"text\"");
                if (p != std::string::npos) {
                    size_t q1 = obj.find("\"", p + 6);
                    if(q1 != std::string::npos) {
                        size_t q2 = obj.find("\"", q1 + 1);
                        if (q1 != std::string::npos && q2 != std::string::npos) text = obj.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
                
                history.push_back({role, text});
                pos = end_pos + 1;
            }
        }
        return history;
    }

    std::string CampaignVault::exportZip() {
        std::string zip_name = meta.name + ".zip";
        fs::path zip_path = fs::path(vault_root) / zip_name;
        
        std::string command = "powershell -Command \"Compress-Archive -Path '" + escapePsPath(vault_path) + "\\*' -DestinationPath '" + escapePsPath(zip_path.string()) + "' -Force\"";
        std::system(command.c_str());
        
        return zip_path.string();
    }

    void CampaignVault::importZip(const std::string& vault_root, const std::string& zip_path) {
        std::string command = "powershell -Command \"Expand-Archive -Path '" + escapePsPath(zip_path) + "' -DestinationPath '" + escapePsPath(vault_root) + "' -Force\"";
        std::system(command.c_str());
    }

    void CampaignVault::clear() {
        meta = CampaignMeta();
        if (fs::exists(vault_path)) {
            try { fs::remove_all(vault_path); } catch (...) {}
        }
        vault_path = "";
    }

    void CampaignVault::package() {
        if (vault_path.empty() || meta.name.empty()) return;
        saveMeta();
        std::string zip_name = meta.name + ".zip";
        fs::path zip_path = fs::path(vault_root) / zip_name;
        std::string command = "powershell -Command \"Compress-Archive -Path '" + escapePsPath(vault_path) + "\\*' -DestinationPath '" + escapePsPath(zip_path.string()) + "' -Force\"";
        std::system(command.c_str());
        clear();
    }
}
