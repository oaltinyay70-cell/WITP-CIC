#pragma once
#include <string>
#include <vector>

namespace engine {

    enum class IntelCategory {
        CRITICAL,
        HVT,
        DISCOVERED
    };

    struct IntelItem {
        std::string title;
        std::string reasoning;
        int solidity;
        IntelCategory category;
    };

    struct DataStats {
        int ops_days = 0;
        int combat_days = 0;
        int sigint_days = 0;
        std::string turn_date = "Unknown";
    };

    class IntelligenceEngine {
    public:
        std::vector<IntelItem> items;
        DataStats stats;
        
        void processDirectory(const std::string& path);
        
        void clear();
    };
}