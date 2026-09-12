#pragma once

#include <string>
#include <vector>
#include <utility>
#include "../engine/IntelligenceEngine.hpp"

namespace storage {
    struct CampaignMeta {
        std::string name;           // User-given campaign name
        std::string scenario;       // From PWS header (e.g. "Hakko Ichiu")
        std::string game_version;   // From PWS header
        std::string created_date;   // ISO timestamp
        std::string last_played;    // ISO timestamp
        std::vector<std::string> source_dirs; // Archive directories
    };

    class CampaignVault {
    public:
        CampaignMeta meta;
        std::string vault_root;  // e.g. "<project_dir>/data/muthr_vaults"
        std::string vault_path;  // vault_root + "/" + campaign_name
        
        // List all existing campaigns in vault_root
        static std::vector<std::string> listCampaigns(const std::string& vault_root);
        
        // Create a new campaign vault
        void create(const std::string& vault_root, const std::string& name);
        
        // Load an existing campaign
        void load(const std::string& vault_path);
        
        // Clear vault from memory and disk (scratch)
        void clear();
        
        // Package the active campaign into its zip file
        void package(bool clear_after = true);
        
        // Save metadata
        void saveMeta();
        
        // Save intel items for a given turn date
        void saveIntelItems(const std::string& turn_date, const std::vector<engine::IntelItem>& items);
        
        // Load all intel items from vault
        std::vector<engine::IntelItem> loadAllIntel();
        
        // Save raw text report
        void saveReport(const std::string& filename, const std::string& content);
        
        // Save SIGINT entries as JSON
        void saveSIGINT(const std::string& turn_date, const std::vector<std::string>& entries);
        
        // Append chat message
        void appendChat(const std::string& role, const std::string& text);
        
        // Load chat history
        std::vector<std::pair<std::string,std::string>> loadChatHistory();
        
        // Export vault to ZIP (returns path to zip)
        std::string exportZip();
        
        // Import vault from ZIP
        static void importZip(const std::string& vault_root, const std::string& zip_path);
    };
}
