#pragma once

#include <string>
#include <vector>

namespace parsers {
    struct PWSHeader {
        std::string format;      // e.g. "PBEM0" or "PBEM1"
        std::string recipient;   // e.g. "Incoming to Dave"
        std::string game_date;   // e.g. "12/08/41"
        std::string timestamp;   // e.g. "Fri, September 11 2026..."
        std::string game_version;
        std::string scenario;    // e.g. "War in the Pacific: Hakko Ichiu"
    };
    
    struct PWSExtract {
        PWSHeader header;
        std::string after_action_report;  // Full AAR text
        std::string sigint_report;        // Full SIGINT text
        std::vector<std::string> ship_names;  // From chunk 6
    };
    
    class PWSParser {
    public:
        // Parse a .pws file and extract all available data
        static PWSExtract parse(const std::string& filepath);
    };
}
