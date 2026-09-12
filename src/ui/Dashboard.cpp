#include "Dashboard.hpp"
#include "../storage/CampaignVault.hpp"
#include "../parsers/PWSParser.hpp"
#include "../ai/StaffOfficer.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <ctime>

namespace ui {

    enum class State { HUB_MENU, MAIN, MODAL_INFO, MODAL_EXIT, LIST_VIEW, WIKI_VIEW, DIR_CONFIG, DIR_BROWSER, CAMPAIGN_MGR, AI_CHAT, SETTINGS_MENU, SETTINGS_AI, VAULT_MENU };
    enum class ItemType { EXIT, BACK, INTEL, MORE_CRITICAL, MORE_HVT, MORE_DISCOVERED };

    struct MenuItem {
        ItemType type;
        std::string title;
        std::string reasoning;
        int solidity;
        int y_pos;
    };

    struct AIConfig {
        std::string provider = "gemini"; // "gemini", "local"
        std::string gemini_api_key = "";
        std::string gemini_model = "gemini-2.0-flash";
        std::string local_endpoint = "http://localhost:11434/v1";
        std::string local_model = "llama3.2";
    };

    class InteractiveTUI {
        HANDLE hOut, hIn;
        State state = State::HUB_MENU;
        int selected_item = 1; 
        int hub_selected_item = 0;
        int esc_count = 0;
        DWORD last_esc_time = 0;
        
        std::vector<MenuItem> current_menu;
        engine::IntelligenceEngine& engine;
        engine::DataStats stats;
        
        engine::IntelCategory list_view_category = engine::IntelCategory::CRITICAL;
        int list_scroll_offset = 0;
        const int MAX_VISIBLE_LIST = 15;
        
        int first_c_idx = -1;
        int first_h_idx = -1;
        int first_d_idx = -1;
        
        std::string active_war_room = "ALLIED";
        
        // Directory config
        std::vector<std::string> data_dirs;
        int dir_selected = -1; // -1 = input field, 0+ = listed dir index
        std::string dir_error_msg = "";
        const int HUB_ITEMS = 5;
        int vault_menu_selected = 0;
        
        // Directory browser
        std::filesystem::path browser_path;
        std::vector<std::string> browser_items;
        int browser_selected = 0;
        int browser_scroll = 0;
        // Intro screen arts
        std::vector<std::string> intro_arts;
        int current_art_index = 0;
        
        // Wiki state
        int wiki_scroll = 0;
        struct WikiPage {
            std::string title;
            std::vector<std::string> content;
        };
        std::vector<WikiPage> wiki_pages;
        int wiki_selected_page = -1;
        int wiki_menu_selected = 0;


        // Storage & Campaign Vault
        storage::CampaignVault vault;
        std::string vault_root;
        std::vector<std::string> campaign_list;
        int campaign_selected = 0;
        std::string campaign_input_name = "";
        std::string vault_status_msg = "";
        bool campaign_input_mode = false;
        bool zip_import_mode = false;
        std::string zip_import_path = "";

        // AI Staff Officer & Settings
        ai::StaffOfficer ai_officer;
        AIConfig ai_config;
        std::string chat_input = "";
        std::vector<std::pair<std::string, std::string>> chat_history;
        int chat_scroll = 0;
        std::string ai_status_msg = "";

        // Settings Menu navigation
        int settings_selected = 0;
        int ai_settings_selected = 0;
        bool ai_input_mode = false;
        int ai_input_field = 0; // 0=key, 1=model, 2=endpoint, 3=local_model
        std::string ai_input_buffer = "";
        std::string settings_status_msg = "";
        int modal_solidity_y = 0;

    public:
        InteractiveTUI(engine::IntelligenceEngine& eng) : engine(eng) {
            stats = engine.stats;
            
            // Auto-discover game directory across all drives
            std::string default_dir = "data/samples"; // fallback
            for (char c = 'C'; c <= 'Z'; ++c) {
                std::string test_path = std::string(1, c) + ":\\Matrix Games\\War in the Pacific Admiral's Edition\\SAVE\\archive";
                if (std::filesystem::exists(test_path) && std::filesystem::is_directory(test_path)) {
                    default_dir = test_path;
                    break;
                }
            }
            data_dirs.push_back(default_dir);
            
            srand((unsigned)time(NULL));
            
            // Aircraft Carrier
            intro_arts.push_back(R"(
                               ___/_____
                         _____|_________|_____
                        /  ___________________\___
                   ____/  /                       \____
     _____________/      /                             \______________
     \___________       /   ___   ___   ___   ___       ____________/
                 \     /   |   | |   | |   | |   |     /
     _____________\___/___________________________\___/______________
     \______________________________________________________________/
)");

            // Battleship
            intro_arts.push_back(R"(
                                      |
                                     -+-
                                    __|__
                             _____|_______|_____
     _______________________/                   \_____________________
     \__________________         | | | | |        ___________________/
                        \        | | | | |       /
     ____________________\_______|_|_|_|_|______/____________________
     \______________________________________________________________/
)");

            // YAMATO
            intro_arts.push_back(R"(
                                      |
                                     _|_
                                   _|___|_
                           ____ |           | ____
     _____________________/ _  \|___||_||___|/  _ \___________________
     \__________________   |_|  |           |  |_|  _________________/
                        \  |_|  |           |  |_| /
     ____________________\______I___________I_____/__________________
     \______________________________________________________________/
)");

            // Wickes Class DD
            intro_arts.push_back(R"(


                                |  |  |  |
                                |  |  |  |
                            ____|__|__|__|____
     ______________________/                  \_______________________
     \__________________                         ____________________/
                        \                       /
     ____________________\_____________________/_____________________
     \______________________________________________________________/
)");

            // A6M Zero
            intro_arts.push_back(R"(


                                        _
                                      -=\`\
                                  |\ ____\_\__
                                -=\c`""""""" "`)
                                   `~~~~~/ /~~`
                                     -==/ /
                                       '-'
)");

            // F4F Wildcat
            intro_arts.push_back(R"(


                                       _
                                     _|=\_
                                 |\ /_____\_
                               -=\c`"""""""`)
                                  `~~~~~| |~`
                                    -==/ /
                                      '-'
)");

            // SBD Dauntless
            intro_arts.push_back(R"(


                                        __
                                      _/_/\_
                                  |\_"      "-.
                                -=\c`"""""""""`)
                                   `~~~~~/ /~~`
                                     -==/ /
                                       '-'
)");

            current_art_index = rand() % intro_arts.size();

            // Populate WIKI
wiki_pages = {
                {
                    "1. HOW IT WORKS",
                    {
                        " W.U.T.H.U.R processes your daily output text files from War in the Pacific:",
                        " Admiral's Edition (Operations Reports, SIGINT, Combat Reports). It aggregates",
                        " historical data from the 'SAVE\\\\archive' folder to extract vital intelligence."
                    }
                },
                {
                    "2. SETUP & DIRECTORIES",
                    {
                        " Use the [ADD/MANAGE CAMPAIGN VAULT] menu to map your game directory.",
                        " The system requires files to be located in the 'SAVE\\\\archive' subdirectory.",
                        " Only text files (.txt) are processed."
                    }
                },
                {
                    "3. INTELLIGENCE KEYWORDS",
                    {
                        " The intelligence engine categorizes parsed reports using specific keywords:",
                        " ",
                        "  > [CRITICAL] : Tactical early warnings.",
                        "      Triggers: 'loaded on', 'moving to', 'Invasion', 'Amphibious'.",
                        "      Meaning : Imminent enemy operations or major movements.",
                        " ",
                        "  > [DISCOVERED] : Allied unit exposure.",
                        "      Triggers: 'sighted over', 'shadowed by', 'observes Japanese'.",
                        "      Meaning : The enemy has spotted your forces, task forces, or bases.",
                        " ",
                        "  > [HVT] : High Value Target Tracker.",
                        "      Triggers: Mentions of massive strategic assets."
                    }
                },
                {
                    "4. ERROR REGISTRY",
                    {
                        " This section catalogs all application errors and their resolutions:",
                        " ",
                        "  > [001] No Log Files Found",
                        "      Cause : The selected directory does not contain any .txt log files.",
                        "      Fix   : Ensure you are selecting a directory that contains .txt logs",
                        "              such as the SAVE/archive folder."
                    }
                }
            };

            // Setup Vault Root strictly inside the project directory
            std::filesystem::path proj_vault = std::filesystem::current_path() / "Campaigns";
            std::filesystem::create_directories(proj_vault);
            vault_root = proj_vault.string();
            
            // Discover existing campaigns
            campaign_list = storage::CampaignVault::listCampaigns(vault_root);
            if (campaign_list.empty()) {
                vault.create(vault_root, "Pacific 1941");
                campaign_list.push_back("Pacific 1941");
            } else {
                vault.load((std::filesystem::path(vault_root) / campaign_list[0]).string());
                                      engine.clear();
                                      data_dirs = vault.meta.source_dirs;
                                      for (const auto& dir : vault.meta.source_dirs) {
                                          if (std::filesystem::exists(dir)) {
                                              engine.processDirectory(dir);
                                          }
                                      }
                                      if (!engine.items.empty()) {
                                          vault.saveIntelItems(engine.stats.turn_date, engine.items);
                                      } else {
                                          auto historical = vault.loadAllIntel();
                                          if (!historical.empty()) {
                                              engine.items = historical;
                                              engine.stats.turn_date = vault.meta.last_played;
                                          }
                                      }
            }
            chat_history = vault.loadChatHistory();
            loadAIConfig();
        }

    private:
        void loadAIConfig() {
            std::string path = "ai/ai_config.json";
            if (!std::filesystem::exists(path)) return;
            std::ifstream ifs(path);
            if (!ifs) return;
            std::stringstream ss;
            ss << ifs.rdbuf();
            std::string json = ss.str();
            
            auto getVal = [&](const std::string& key) -> std::string {
                std::string pattern = "\"" + key + "\": \"";
                size_t p = json.find(pattern);
                if (p == std::string::npos) return "";
                p += pattern.length();
                size_t end = json.find("\"", p);
                if (end == std::string::npos) return "";
                return json.substr(p, end - p);
            };

            std::string prov = getVal("provider");
            if (!prov.empty()) ai_config.provider = prov;
            std::string key = getVal("gemini_api_key");
            if (!key.empty()) ai_config.gemini_api_key = key;
            std::string gm = getVal("gemini_model");
            if (!gm.empty()) ai_config.gemini_model = gm;
            std::string ep = getVal("local_endpoint");
            if (!ep.empty()) ai_config.local_endpoint = ep;
            std::string lm = getVal("local_model");
            if (!lm.empty()) ai_config.local_model = lm;
        }

        void saveAIConfig() {
            std::filesystem::create_directories("ai");
            std::ofstream ofs("ai/ai_config.json");
            ofs << "{\n";
            ofs << "  \"provider\": \"" << ai_config.provider << "\",\n";
            ofs << "  \"gemini_api_key\": \"" << ai_config.gemini_api_key << "\",\n";
            ofs << "  \"gemini_model\": \"" << ai_config.gemini_model << "\",\n";
            ofs << "  \"local_endpoint\": \"" << ai_config.local_endpoint << "\",\n";
            ofs << "  \"local_model\": \"" << ai_config.local_model << "\"\n";
            ofs << "}\n";
            ofs.close();

            // Also write key to api_key.txt if present
            if (!ai_config.gemini_api_key.empty()) {
                std::ofstream kfs("ai/api_key.txt");
                kfs << ai_config.gemini_api_key;
                kfs.close();
            }
        }

        void playNavSound() { Beep(400, 20); }
        void playSelectSound() { Beep(800, 40); }
        void playErrorSound() { Beep(200, 100); }

        void initConsole() {
            // Enable ANSI escape sequences (blink, etc.)
            DWORD mode = 0;
            GetConsoleMode(hOut, &mode);
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);

            // Lock the screen buffer to the visible window size — no scrollbar
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            COORD bufSize;
            bufSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            bufSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
            SetConsoleScreenBufferSize(hOut, bufSize);
        }

        void clear() {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            DWORD len = csbi.dwSize.X * csbi.dwSize.Y;
            DWORD written;
            COORD origin = {0, 0};
            FillConsoleOutputCharacterA(hOut, ' ', len, origin, &written);
            FillConsoleOutputAttribute(hOut, csbi.wAttributes, len, origin, &written);
            SetConsoleCursorPosition(hOut, origin);
        }

        void setColor(int color) { SetConsoleTextAttribute(hOut, color); }
        void gotoxy(int x, int y) {
            COORD c; c.X = x; c.Y = y;
            SetConsoleCursorPosition(hOut, c);
        }

        std::vector<std::string> wordWrap(const std::string& text, size_t max_width) {
            std::vector<std::string> lines;
            size_t start = 0;
            while (start < text.length()) {
                if (text.length() - start <= max_width) {
                    lines.push_back(text.substr(start));
                    break;
                }
                size_t end = start + max_width;
                size_t last_space = text.rfind(' ', end);
                if (last_space != std::string::npos && last_space > start) {
                    end = last_space;
                }
                lines.push_back(text.substr(start, end - start));
                start = end + 1;
            }
            return lines;
        }

        void loadMainMenu() {
            current_menu.clear();
            selected_item = 1;
            first_c_idx = first_h_idx = first_d_idx = -1;
            
            current_menu.push_back({ItemType::EXIT, "[X] LOGOUT", "", 0, 1});
            
            int y = 10;
            
            // Critical
            int c_count = 0, c_total = 0;
            for (const auto& intel : engine.items) if (intel.category == engine::IntelCategory::CRITICAL) c_total++;
            for (const auto& intel : engine.items) {
                if (intel.category == engine::IntelCategory::CRITICAL) {
                    if (c_count < 4) {
                        if (c_count == 0) first_c_idx = current_menu.size();
                        current_menu.push_back({ItemType::INTEL, intel.title, intel.reasoning, intel.solidity, y++});
                        c_count++;
                    }
                }
            }
            if (c_count == 0) {
                first_c_idx = current_menu.size();
                current_menu.push_back({ItemType::INTEL, "> No critical alerts at this time.", "", 0, y++});
            } else if (c_total > 4) {
                current_menu.push_back({ItemType::MORE_CRITICAL, "> ... more (" + std::to_string(c_total - 4) + " additional alerts)", "", 0, y++});
            }
            
            y += 2; 
            
            // HVT Tracker
            int h_count = 0, h_total = 0;
            for (const auto& intel : engine.items) if (intel.category == engine::IntelCategory::HVT) h_total++;
            for (const auto& intel : engine.items) {
                if (intel.category == engine::IntelCategory::HVT) {
                    if (h_count < 4) {
                        if (h_count == 0) first_h_idx = current_menu.size();
                        current_menu.push_back({ItemType::INTEL, intel.title, intel.reasoning, intel.solidity, y++});
                        h_count++;
                    }
                }
            }
            if (h_count == 0) {
                first_h_idx = current_menu.size();
                current_menu.push_back({ItemType::INTEL, "> No high value targets detected.", "", 0, y++});
            } else if (h_total > 4) {
                current_menu.push_back({ItemType::MORE_HVT, "> ... more (" + std::to_string(h_total - 4) + " additional targets)", "", 0, y++});
            }
            
            y += 2;
            
            // Discovered Units
            int d_count = 0, d_total = 0;
            for (const auto& intel : engine.items) if (intel.category == engine::IntelCategory::DISCOVERED) d_total++;
            for (const auto& intel : engine.items) {
                if (intel.category == engine::IntelCategory::DISCOVERED) {
                    if (d_count < 4) {
                        if (d_count == 0) first_d_idx = current_menu.size();
                        current_menu.push_back({ItemType::INTEL, intel.title, intel.reasoning, intel.solidity, y++});
                        d_count++;
                    }
                }
            }
            if (d_count == 0) {
                first_d_idx = current_menu.size();
                current_menu.push_back({ItemType::INTEL, "> No units discovered.", "", 0, y++});
            } else if (d_total > 4) {
                current_menu.push_back({ItemType::MORE_DISCOVERED, "> ... more (" + std::to_string(d_total - 4) + " additional discoveries)", "", 0, y++});
            }
        }

        void loadListView(engine::IntelCategory category) {
            list_view_category = category;
            current_menu.clear();
            selected_item = 1;
            list_scroll_offset = 0;
            current_menu.push_back({ItemType::BACK, "[<] BACK", "", 0, 1});
            
            for (const auto& intel : engine.items) {
                if (intel.category == category) {
                    current_menu.push_back({ItemType::INTEL, intel.title, intel.reasoning, intel.solidity, 0});
                }
            }
        }

        void returnToHub() {
            vault.package();
            engine.clear();
            { vault.package(); engine.clear(); state = State::HUB_MENU; }
            drawHub();
        }

        void drawHub() {
            clear();
            setColor(10);
            std::cout << intro_arts[current_art_index];
            std::cout << R"(
     __  __   _   _   _____   _   _   _   _   ____
     |  \/  | | | | | |_   _| | | | | | | | | |  _ \
     | |\/| | | | | |   | |   | |_| | | | | | | |_) |
     | |  | | | |_| |   | |   |  _  | | |_| | |  _ <
     |_|  |_|  \___/    |_|   |_| |_|  \___/  |_| \_\
)" << "\n";
            std::cout << "     INTERFACE 1943.12 INITIALIZED\n";
            std::cout << "     ============================================================\n";
            std::string hub_items[5] = {
                "ALLIED WAR ROOM", 
                "JAPANESE WAR ROOM", 
                "ADD/MANAGE CAMPAIGN VAULT", 
                "SETTINGS",
                "WIKI & MANUAL"
            };
            for(int i=0; i<HUB_ITEMS; i++) {
                if (i == hub_selected_item) {
                    setColor(160);
                    std::cout << "     " << hub_items[i];
                    for(size_t j=hub_items[i].length(); j<35; j++) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(2);
                    std::cout << "     " << hub_items[i] << "\n";
                }
            }
            
            // Draw What's New Box
            int box_left = 72;
            int box_top = 2; // Top right of the screen
            
            setColor(2);
            gotoxy(box_left, box_top); std::cout << "+---------------------------------+";
            gotoxy(box_left, box_top+1); std::cout << "| "; setColor(10); std::cout << "WHAT'S NEW - v0.02             "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+2); std::cout << "|---------------------------------|";
            gotoxy(box_left, box_top+3); std::cout << "| "; setColor(10); std::cout << "- Campaign Vault Storage       "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+4); std::cout << "| "; setColor(10); std::cout << "- ZIP Export & Import          "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+5); std::cout << "| "; setColor(10); std::cout << "- PWS Binary Auto-Extraction   "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+6); std::cout << "| "; setColor(10); std::cout << "- AI Staff Officer Chat        "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+7); std::cout << "+---------------------------------+";
            
            // Get window height and position the version at the bottom
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            int bottom = csbi.srWindow.Bottom - csbi.srWindow.Top;
            gotoxy(64, bottom);
            setColor(10);
            std::cout << "v0.02";
            setColor(2);
        }

        void drawMain() {
            clear();
            setColor(2); // Dark Green border
            std::cout << "===============================================================================\n";
            setColor(10); // Light green title
            std::cout << " [ " << active_war_room << " COMBAT INFORMATION CENTER ]                     Turn: " << stats.turn_date << "\n";
            setColor(2);
            std::cout << "===============================================================================\n";
            
            setColor(10);
            std::cout << " [ DATA SOURCES SYNCED ]\n";
            setColor(2);
            std::cout << " > Operations Reports : Last " << (stats.ops_days == 0 ? 1 : stats.ops_days) << " Days\n";
            std::cout << " > Combat Reports     : Last " << (stats.combat_days == 0 ? (stats.ops_days == 0 ? 1 : stats.ops_days) : stats.combat_days) << " Days\n";
            std::cout << " > SIGINT             : Last " << (stats.sigint_days == 0 ? 1 : stats.sigint_days) << " Days\n";
            setColor(2);
            std::cout << "-------------------------------------------------------------------------------\n\n";

            setColor(10);
            std::cout << " [C]ritical alerts\n";
            setColor(2);
            std::cout << "-------------------------------------------------------------------------------\n";
            
            if (first_h_idx != -1) {
                gotoxy(0, current_menu[first_h_idx].y_pos - 2);
                setColor(10);
                std::cout << " [H]igh Value Target Tracker\n";
                setColor(2);
                std::cout << "-------------------------------------------------------------------------------\n";
            }
            
            if (first_d_idx != -1) {
                gotoxy(0, current_menu[first_d_idx].y_pos - 2);
                setColor(10); 
                std::cout << " [D]iscovered " << (active_war_room == "ALLIED" ? "Allied" : "Japanese") << " Units\n";
                setColor(2);
                std::cout << "-------------------------------------------------------------------------------\n";
            }

            for(int i=0; i < current_menu.size(); ++i) {
                gotoxy(i==0 ? 68 : 2, current_menu[i].y_pos);
                if(i == selected_item) {
                    setColor(160); // Black on Light Green
                } else {
                    setColor(10); // Everything Light Green in the matrix
                }
                std::cout << current_menu[i].title;
                setColor(2); // reset to dark green
            }

            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            int b_line = csbi.srWindow.Bottom - csbi.srWindow.Top;
            gotoxy(2, b_line);
            setColor(2);
            std::cout << "[A] AI Chat  |  [Y] YOINK  |  [C/H/D] Jump  |  [X] Logout  |  [ENTER] Inspect";
        }

        void drawListView() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ " << active_war_room << " COMBAT INFORMATION CENTER ]                     Turn: " << stats.turn_date << "\n";
            setColor(2);
            std::cout << "  ===============================================================================\n";
            
            setColor(10);
            if (list_view_category == engine::IntelCategory::CRITICAL) std::cout << "   [ FULL LIST ] CRITICAL ALERTS\n";
            else if (list_view_category == engine::IntelCategory::HVT) std::cout << "   [ FULL LIST ] HIGH VALUE TARGETS\n";
            else if (list_view_category == engine::IntelCategory::DISCOVERED) std::cout << "   [ FULL LIST ] DISCOVERED UNITS\n";
            
            setColor(2);
            std::cout << "  -------------------------------------------------------------------------------\n";
            
            // Render Back button
            gotoxy(70, 1);
            if (selected_item == 0) setColor(160); else setColor(10);
            std::cout << current_menu[0].title;
            
            if (selected_item > 0) {
                if (selected_item < list_scroll_offset + 1) {
                    list_scroll_offset = selected_item - 1;
                } else if (selected_item >= list_scroll_offset + 1 + MAX_VISIBLE_LIST) {
                    list_scroll_offset = selected_item - MAX_VISIBLE_LIST;
                }
            }
            
            int draw_y = 5;
            for (int i = 1; i < current_menu.size(); ++i) {
                if (i >= list_scroll_offset + 1 && i < list_scroll_offset + 1 + MAX_VISIBLE_LIST) {
                    gotoxy(4, draw_y);
                    if (i == selected_item) setColor(160); else setColor(10);
                    std::cout << current_menu[i].title;
                    current_menu[i].y_pos = draw_y; 
                    draw_y++;
                } else {
                    current_menu[i].y_pos = -1; // offscreen
                }
            }
            setColor(2);
            
            if (list_scroll_offset > 0) {
                gotoxy(75, 5); setColor(10); std::cout << "^";
            }
            if (list_scroll_offset + 1 + MAX_VISIBLE_LIST <= current_menu.size()) {
                gotoxy(75, draw_y - 1); setColor(10); std::cout << "v";
            }
            setColor(2);
        }

                void drawWiki() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ W.U.T.H.U.R USER MANUAL & WIKI DATABASE ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            if (wiki_selected_page == -1) {
                for (size_t i = 0; i < wiki_pages.size(); ++i) {
                    if ((int)i == wiki_menu_selected) {
                        setColor(160);
                        std::cout << "     " << wiki_pages[i].title;
                        for (size_t j = wiki_pages[i].title.length(); j < 35; j++) std::cout << " ";
                        std::cout << "\n";
                        setColor(10);
                    } else {
                        std::cout << "     " << wiki_pages[i].title << "\n";
                    }
                    std::cout << "\n";
                }
                setColor(2);
                std::cout << "\n  -------------------------------------------------------------------------------\n";
                std::cout << " UP/DOWN: Select Topic  |  ENTER: Read Topic  |  ESC: Return\n";
            } else {
                const auto& page = wiki_pages[wiki_selected_page];
                setColor(14);
                std::cout << "   " << page.title << "\n\n";
                setColor(10);
                
                int max_lines = 14;
                if (wiki_scroll > (int)page.content.size() - max_lines && page.content.size() > max_lines) {
                    wiki_scroll = (int)page.content.size() - max_lines;
                }
                
                for (int i = 0; i < max_lines; ++i) {
                    int idx = wiki_scroll + i;
                    if (idx < (int)page.content.size()) {
                        std::cout << page.content[idx] << "\n";
                    } else {
                        std::cout << "\n";
                    }
                }
                
                setColor(2);
                std::cout << "  -------------------------------------------------------------------------------\n";
                std::cout << " UP/DOWN: Scroll  |  ESC: Back to Wiki Menu\n";
            }
        }
        
        void drawCampaignMgr() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ CAMPAIGN VAULT MANAGER & PORTABILITY ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            if (!vault_status_msg.empty()) {
                setColor(12);
                std::cout << " " << vault_status_msg << "\n\n";
                setColor(2);
            }
            if (!vault.meta.scenario.empty()) {
                std::cout << "     Scenario : " << vault.meta.scenario << "\n";
            }
            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n\n";

            setColor(10);
            std::cout << " AVAILABLE CAMPAIGN VAULTS (in " << vault_root << "):\n\n";

            campaign_list = storage::CampaignVault::listCampaigns(vault_root);
            if (campaign_list.empty()) {
                setColor(2);
                std::cout << "     (No campaigns found)\n\n";
            } else {
                for (int i = 0; i < (int)campaign_list.size(); ++i) {
                    bool isActive = (campaign_list[i] == vault.meta.name);
                    if (i == campaign_selected) {
                        setColor(160);
                        std::cout << "     [" << (i + 1) << "] " << campaign_list[i];
                        if (isActive) std::cout << "    (ACTIVE)";
                        for (size_t p = campaign_list[i].length() + (isActive ? 10 : 0); p < 45; ++p) std::cout << " ";
                    } else {
                        setColor(isActive ? 10 : 2);
                        std::cout << "     [" << (i + 1) << "] " << campaign_list[i];
                        if (isActive) std::cout << "    (ACTIVE)";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";
            }

            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n";
            if (campaign_input_mode) {
                setColor(10);
                std::cout << " ENTER NEW CAMPAIGN NAME: ";
                setColor(15);
                std::cout << campaign_input_name << "_\n\n";
                setColor(2);
                std::cout << "   [ENTER] Create Vault  |  [ESC] Cancel\n";
            } else if (zip_import_mode) {
                setColor(10);
                std::cout << " ENTER PATH TO IMPORT .ZIP: ";
                setColor(15);
                std::cout << zip_import_path << "_\n\n";
                setColor(2);
                std::cout << "   [ENTER] Import Archive  |  [ESC] Cancel\n";
            } else {
                setColor(10);
                std::cout << "   [ENTER] Switch Vault  |  [N] New Vault  |  [E] Export ZIP  |  [I] Import ZIP\n";
                setColor(2);
                std::cout << "   [ESC] Return to Vault Menu\n";
            }
        }

        void drawAIChat() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ W.U.T.H.U.R TACTICAL STAFF OFFICER ]              Campaign: " << vault.meta.name << "\n";
            setColor(2);
            std::cout << "  ===============================================================================\n";

            if (!ai_status_msg.empty()) {
                setColor(14);
                std::cout << "   >>> " << ai_status_msg << "\n";
                setColor(2);
                std::cout << "  -------------------------------------------------------------------------------\n";
            }

            int start_idx = 0;
            if (chat_history.size() > 4) {
                start_idx = (int)chat_history.size() - 4;
            }

            if (chat_history.empty()) {
                setColor(10);
                std::cout << "\n STAFF OFFICER > Commander, I am standing by to analyze operational intelligence,\n";
                std::cout << "                   track enemy fleet dispositions, or assess high-value targets.\n";
                std::cout << "                   Ask any question regarding your current campaign.\n\n";
            } else {
                for (size_t i = start_idx; i < chat_history.size(); ++i) {
                    const auto& item = chat_history[i];
                    if (item.first == "user" || item.first == "COMMANDER") {
                        setColor(15);
                        std::cout << " COMMANDER > ";
                        setColor(10);
                        std::cout << item.second << "\n\n";
                    } else {
                        setColor(14);
                        std::cout << " STAFF OFFICER > ";
                        setColor(10);
                        auto wrapped = wordWrap(item.second, 74);
                        for (size_t wi = 0; wi < wrapped.size(); ++wi) {
                            if (wi > 0) std::cout << "                   ";
                            std::cout << wrapped[wi] << "\n";
                        }
                        std::cout << "\n";
                    }
                }
            }

            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            int b_line = csbi.srWindow.Bottom - csbi.srWindow.Top;

            gotoxy(2, b_line - 3);
            setColor(2);
            std::cout << "  -------------------------------------------------------------------------------\n";
            setColor(10);
            std::cout << " ASK STAFF OFFICER > ";
            setColor(15);
            std::cout << chat_input << "_\n";
            setColor(2);
            std::cout << "\n [ENTER] Transmit Query  |  [ESC] Return";
        }

        void drawSettingsMenu() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ W.U.T.H.U.R SYSTEM SETTINGS & PREFERENCES ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            setColor(10);
            std::cout << " SELECT CONFIGURATION MODULE:\n";
            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n\n";

            std::string s_items[3] = {
                "AI MENTOR & MODEL CONFIGURATION (Cloud & Local)",
                "FILE ARCHIVE DIRECTORIES",
                "[<] RETURN TO W.U.T.H.U.R HUB"
            };

            for (int i = 0; i < 3; ++i) {
                if (i == settings_selected) {
                    setColor(160);
                    std::cout << "     " << s_items[i];
                    for (size_t p = s_items[i].length(); p < 60; ++p) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(10);
                    std::cout << "     " << s_items[i] << "\n";
                }
            }

            setColor(2);
            std::cout << "\n -------------------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Select  |  ESC: Return to Hub\n";
        }

        void drawSettingsAI() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ AI MENTOR & MODEL CONFIGURATION ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            if (!settings_status_msg.empty()) {
                setColor(14);
                std::cout << "   >>> " << settings_status_msg << "\n";
                setColor(2);
                std::cout << " -------------------------------------------------------------------------------\n\n";
            }

            setColor(10);
            std::cout << " ACTIVE MODEL PROVIDER: ";
            if (ai_config.provider == "local") {
                setColor(14);
                std::cout << "[ LOCAL MODEL (OLLAMA / LM STUDIO) - 100% PRIVATE & OFFLINE ]\n";
            } else {
                setColor(11);
                std::cout << "[ GOOGLE GEMINI (CLOUD API) - HIGH INTELLIGENCE ]\n";
            }
            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n\n";

            std::vector<std::string> labels = {
                "AI Engine Provider   : " + (ai_config.provider == "local" ? std::string("LOCAL (Ollama/LM Studio)") : std::string("GOOGLE GEMINI (Cloud)")),
                "Gemini API Key       : " + (ai_config.gemini_api_key.empty() ? std::string("(NOT SET - REQUIRED FOR CLOUD)") : (ai_config.gemini_api_key.substr(0, std::min<size_t>(8, ai_config.gemini_api_key.length())) + "****************")),
                "Gemini Model Name    : " + ai_config.gemini_model,
                "Local Server Endpoint: " + ai_config.local_endpoint,
                "Local Model Name     : " + ai_config.local_model,
                "[ TEST AI CONNECTION NOW ]"
            };

            for (int i = 0; i < (int)labels.size(); ++i) {
                if (i == ai_settings_selected) {
                    setColor(160);
                    std::cout << "    " << (i + 1) << ". " << labels[i];
                    for (size_t p = labels[i].length() + 5; p < 74; ++p) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(10);
                    std::cout << "    " << (i + 1) << ". " << labels[i] << "\n";
                }
            }

            setColor(2);
            std::cout << "\n -------------------------------------------------------------------------------\n";
            setColor(10);
            std::cout << " EXPLANATIONS & INSTRUCTIONS:\n";
            setColor(2);

            if (ai_config.provider == "gemini") {
                std::cout << "    - GOOGLE GEMINI: Fast, high-reasoning tactical AI.\n";
                std::cout << "    - FREE API KEY: Obtain a 100% free key at: https://aistudio.google.com\n";
                std::cout << "      (No credit card required. Free tier supports up to 15 requests/min).\n";
                std::cout << "    - Press [ENTER] on Item 2 to enter or update your API key.\n";
            } else {
                std::cout << "    - LOCAL MODEL: Runs 100% locally on your PC with complete privacy.\n";
                std::cout << "    - RECOMMENDED OFFLINE AI: Google Gemma 2 (2 Billion Parameter)\n";
                std::cout << "      1. Install Ollama (https://ollama.com)\n";
                std::cout << "      2. Run in a standard Windows command prompt: ollama run gemma2:2b\n";
                std::cout << "      3. Change 'Local Model Name' above to: gemma2:2b\n";
                std::cout << "    - Default endpoint for Ollama: http://localhost:11434/v1\n";
            }

            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n";
            if (ai_input_mode) {
                setColor(10);
                std::cout << " ENTER NEW VALUE: ";
                setColor(15);
                std::cout << ai_input_buffer << "_\n";
                setColor(2);
                std::cout << "   [ENTER] Confirm  |  [ESC] Cancel\n";
            } else {
                std::cout << " UP/DOWN: Select  |  ENTER: Toggle/Edit  |  [T] Test Connection  |  ESC: Save & Back\n";
            }
        }

        void drawDirConfig() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ FILE DIRECTORY CONFIGURATION ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";
            
            if (!dir_error_msg.empty()) {
                if (dir_error_msg.find("SUCCESS") != std::string::npos || dir_error_msg.find("LOG FILES ARE PROCESSED") != std::string::npos) {
                    setColor(10); // Light green for success
                } else {
                    setColor(12); // Light red for errors
                }
                std::cout << "   " << dir_error_msg << "\n\n";
                setColor(2);
            }

            setColor(10);
            std::cout << " Active data directories:\n";
            setColor(2);
            std::cout << " -----------------------------------------------------------------------\n";
            
            if (data_dirs.empty()) {
                setColor(2);
                std::cout << "     (none configured)\n";
            } else {
                for (int i = 0; i < (int)data_dirs.size(); i++) {
                    if (dir_selected == i) {
                        setColor(160);
                        std::cout << "     [" << (i+1) << "] " << data_dirs[i];
                        // Pad
                        for (size_t p = data_dirs[i].length(); p < 55; p++) std::cout << " ";
                        std::cout << "[DEL]";
                        std::cout << "\n";
                    } else {
                        setColor(10);
                        std::cout << "     [" << (i+1) << "] " << data_dirs[i] << "\n";
                    }
                }
            }
            
            setColor(2);
            std::cout << " -----------------------------------------------------------------------\n\n";
            
            setColor(10);
            std::cout << " Add new directory path:\n";
            
            // Input field changed to a button
            if (dir_selected == -1) {
                setColor(160);
                std::cout << "   > [ BROWSE FOR DIRECTORY ]                                      \n";
            } else {
                setColor(2);
                std::cout << "   > [ BROWSE FOR DIRECTORY ]                                      \n";
            }
            
            setColor(2);
            std::cout << "\n -----------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Select/Delete  |  ESC: Back to W.U.T.H.U.R\n";
        }

        void loadBrowserItems() {
            browser_items.clear();
            if (browser_path.has_parent_path() && browser_path != browser_path.parent_path()) {
                browser_items.push_back("..");
            }
            try {
                for (const auto& entry : std::filesystem::directory_iterator(browser_path)) {
                    if (entry.is_directory()) {
                        browser_items.push_back(entry.path().filename().string());
                    }
                }
            } catch (...) {}
            browser_selected = 0;
            browser_scroll = 0;
        }

        void drawDirBrowser() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ SELECT DIRECTORY ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            setColor(15);
            std::string p_str = browser_path.string();
            if (p_str.length() > 60) p_str = "..." + p_str.substr(p_str.length() - 57);
            std::cout << " CURRENT: " << p_str << "\n\n";

            // The 'Select this directory' button is index 0 visually, but let's make it part of the scroll list.
            // Actually, let's keep it fixed at the top.
            if (browser_selected == -1) {
                setColor(160);
                std::cout << "   > [ CHOOSE THIS DIRECTORY ]                                     \n\n";
            } else {
                setColor(10);
                std::cout << "   > [ CHOOSE THIS DIRECTORY ]                                     \n\n";
            }

            setColor(2);
            std::cout << " Subdirectories:\n";
            std::cout << " -----------------------------------------------------------------------\n";

            int max_visible = 12;
            if (browser_selected >= 0) {
                if (browser_selected < browser_scroll) browser_scroll = browser_selected;
                if (browser_selected >= browser_scroll + max_visible) browser_scroll = browser_selected - max_visible + 1;
            }

            for (int i = 0; i < max_visible; i++) {
                int idx = browser_scroll + i;
                if (idx >= (int)browser_items.size()) {
                    std::cout << "\n";
                    continue;
                }

                if (browser_selected == idx) {
                    setColor(160);
                    std::cout << "     " << browser_items[idx];
                    for(size_t p=browser_items[idx].length(); p<65; p++) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(10);
                    std::cout << "     " << browser_items[idx] << "\n";
                }
            }

            setColor(2);
            std::cout << " -----------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Enter/Select  |  ESC: Cancel\n";
        }

        void drawVaultMenu() {
            clear();
            setColor(2);
            std::cout << "  ===============================================================================\n";
            setColor(10);
            std::cout << "   [ W.U.T.H.U.R CAMPAIGN VAULT & FILE MANAGER ]\n";
            setColor(2);
            std::cout << "  ===============================================================================\n\n";

            setColor(10);
            std::cout << " CURRENT ACTIVE CAMPAIGN:\n";
            setColor(15);
            std::cout << "     Name     : " << vault.meta.name << "\n";
            std::cout << "     Storage  : " << vault.vault_path << "\n";
            if (!vault.meta.scenario.empty()) {
                std::cout << "     Scenario : " << vault.meta.scenario << "\n";
            }
            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n\n";

            setColor(10);
            std::cout << " SELECT VAULT MANAGEMENT OPTION:\n";
            setColor(2);
            std::cout << " -------------------------------------------------------------------------------\n\n";

            std::string v_items[3] = {
                "CAMPAIGN VAULT (MANAGE / EXPORT)",
                "FILE ARCHIVE DIRECTORIES",
                "[<] RETURN TO W.U.T.H.U.R HUB"
            };

            for (int i = 0; i < 3; ++i) {
                if (i == vault_menu_selected) {
                    setColor(160);
                    std::cout << "     " << v_items[i];
                    for (size_t p = v_items[i].length(); p < 60; ++p) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(10);
                    std::cout << "     " << v_items[i] << "\n";
                }
            }

            setColor(2);
            std::cout << "\n -------------------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Select  |  ESC: Return to Hub\n";
        }

        void drawModalInfo() {
            auto& item = current_menu[selected_item];
            if (item.solidity == 0) return;

            int box_top = 4, box_bot = 20, box_left = 6, box_right = 74;
            int inner_width = box_right - box_left - 1;

            // Draw border in bright green on black
            setColor(10);
            gotoxy(box_left, box_top);
            std::cout << "+";
            for(int x = box_left+1; x < box_right; x++) std::cout << "-";
            std::cout << "+";

            gotoxy(box_left, box_bot);
            std::cout << "+";
            for(int x = box_left+1; x < box_right; x++) std::cout << "-";
            std::cout << "+";

            for(int y = box_top+1; y < box_bot; y++) {
                gotoxy(box_left, y);
                std::cout << "|";
                // Fill interior with black background, white text
                setColor(15); // Bright white on black
                for(int x = box_left+1; x < box_right; x++) std::cout << " ";
                setColor(10);
                std::cout << "|";
            }

            // Title bar
            setColor(160); // Green BG, black text - just for the title line
            gotoxy(box_left+1, box_top+1);
            std::string title_bar = " INTEL ASSESSMENT ";
            std::cout << title_bar;
            for(int x = title_bar.length(); x < inner_width; x++) std::cout << " ";

            // Separator under title
            setColor(10);
            gotoxy(box_left+1, box_top+2);
            for(int x = 0; x < inner_width; x++) std::cout << "-";

            // Reasoning text - bright white on black for max contrast
            setColor(15);
            std::vector<std::string> wrapped_reasoning = wordWrap("REASONING: " + item.reasoning, inner_width - 4);
            int current_y = box_top + 4;
            for(const auto& line : wrapped_reasoning) {
                if (current_y >= box_bot - 4) break; // don't overflow
                gotoxy(box_left + 3, current_y++);
                std::cout << line;
            }

            // Confidence Level
            current_y++;
            modal_solidity_y = current_y;
            gotoxy(box_left + 3, current_y);
            if (item.solidity > 90) {
                // Blink for confidence level above 90% (once every second = 1.0 Hz)
                setColor(10);
                std::cout << "CONFIDENCE LVL: " << item.solidity << "%  [CONFIRMED INTEL]";
            } else {
                setColor(10);
                std::cout << "CONFIDENCE LVL: ";
                setColor(15);
                std::cout << item.solidity << "%";
            }
            current_y++;

            // Progress bar
            int meter_len = item.solidity / 5;
            gotoxy(box_left + 3, current_y);
            setColor(10);
            std::cout << "[";
            for(int i=0; i<20; ++i) {
                if (i < meter_len) { setColor(160); std::cout << " "; } // filled = green block
                else { setColor(10); std::cout << "."; }
            }
            setColor(10);
            std::cout << "]";

            // Close hint
            setColor(2);
            gotoxy(box_left + 3, box_bot - 1);
            std::cout << "Press ENTER or ESC to close.";
            setColor(2);
        }

        void updateBlinkModal(bool phase) {
            auto& item = current_menu[selected_item];
            if (item.solidity <= 90 || modal_solidity_y <= 0) return;
            int box_left = 6;
            gotoxy(box_left + 3, modal_solidity_y);
            if (phase) {
                setColor(10);
                std::cout << "CONFIDENCE LVL: " << item.solidity << "%  [CONFIRMED INTEL]";
            } else {
                setColor(2);
                std::cout << "CONFIDENCE LVL: " << item.solidity << "%  [CONFIRMED INTEL]";
            }
        }

        void updateBlinkMain(bool phase) {
            for(size_t i=0; i < current_menu.size(); ++i) {
                if (current_menu[i].solidity >= 90 && (int)i != selected_item) {
                    gotoxy(i==0 ? 68 : 2, current_menu[i].y_pos);
                    if (phase) {
                        setColor(10); // Bright Green
                    } else {
                        setColor(2);  // Dim Dark Green (flashes at 1 Hz)
                    }
                    std::cout << current_menu[i].title;
                }
            }
            setColor(2);
        }

        void updateBlinkListView(bool phase) {
            int draw_y = 7;
            for (int i = 0; i < MAX_VISIBLE_LIST; ++i) {
                int item_idx = list_scroll_offset + 1 + i;
                if (item_idx >= (int)current_menu.size()) break;
                if (current_menu[item_idx].solidity >= 90 && item_idx != selected_item) {
                    gotoxy(2, draw_y + i);
                    if (phase) setColor(10); else setColor(2);
                    std::cout << current_menu[item_idx].title;
                }
            }
            setColor(2);
        }

        void drawModalExit() {
            int box_top = 8, box_bot = 15, box_left = 18, box_right = 60;
            int inner_width = box_right - box_left - 1;

            // Border
            setColor(10);
            gotoxy(box_left, box_top);
            std::cout << "+";
            for(int x = box_left+1; x < box_right; x++) std::cout << "-";
            std::cout << "+";

            gotoxy(box_left, box_bot);
            std::cout << "+";
            for(int x = box_left+1; x < box_right; x++) std::cout << "-";
            std::cout << "+";

            for(int y = box_top+1; y < box_bot; y++) {
                gotoxy(box_left, y);
                std::cout << "|";
                setColor(15);
                for(int x = box_left+1; x < box_right; x++) std::cout << " ";
                setColor(10);
                std::cout << "|";
            }

            // Title
            setColor(160);
            gotoxy(box_left+1, box_top+1);
            std::string title = " SYSTEM LOGOUT ";
            std::cout << title;
            for(int x = title.length(); x < inner_width; x++) std::cout << " ";

            // Separator
            setColor(10);
            gotoxy(box_left+1, box_top+2);
            for(int x = 0; x < inner_width; x++) std::cout << "-";

            // Question
            setColor(15);
            gotoxy(box_left+3, box_top+4);
            std::cout << "Confirm Logout? (Y/N)";
            setColor(2);
        }

        void redrawCurrentState() {
            if (state == State::HUB_MENU) drawHub();
            else if (state == State::MAIN) drawMain();
            else if (state == State::LIST_VIEW) drawListView();
            else if (state == State::WIKI_VIEW) drawWiki();
            else if (state == State::DIR_CONFIG) drawDirConfig();
            else if (state == State::DIR_BROWSER) drawDirBrowser();
            else if (state == State::CAMPAIGN_MGR) drawCampaignMgr();
            else if (state == State::AI_CHAT) drawAIChat();
            else if (state == State::SETTINGS_MENU) drawSettingsMenu();
            else if (state == State::SETTINGS_AI) drawSettingsAI();
            else if (state == State::VAULT_MENU) drawVaultMenu();
        }

    public:
        void run() {
            hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            hIn = GetStdHandle(STD_INPUT_HANDLE);

            initConsole();

            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hOut, &cursorInfo);
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(hOut, &cursorInfo);

            drawHub();

            DWORD cc;
            INPUT_RECORD ir;
            bool running = true;
            bool blink_phase = false;

            while(running) {
                DWORD wait_res = WaitForSingleObject(hIn, 500); // 500ms half-cycle = 1000ms period = 1 blink per second (1.0 Hz)
                if (wait_res == WAIT_TIMEOUT) {
                    blink_phase = !blink_phase;
                    if (state == State::MODAL_INFO) {
                        updateBlinkModal(blink_phase);
                    }
                    continue;
                }

                ReadConsoleInput(hIn, &ir, 1, &cc);
                if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
                    WORD key = ir.Event.KeyEvent.wVirtualKeyCode;
                    char ch = ir.Event.KeyEvent.uChar.AsciiChar;

                    if (key == VK_ESCAPE) {
                        DWORD now = GetTickCount();
                        if (now - last_esc_time < 500) esc_count++;
                        else esc_count = 1;
                        last_esc_time = now;
                        
                        if (state == State::MODAL_INFO) {
                            playNavSound();
                            if (current_menu[0].type == ItemType::BACK) state = State::LIST_VIEW;
                            else state = State::MAIN;
                            redrawCurrentState();
                            continue;
                        } else if (state == State::SETTINGS_AI) {
                            playNavSound();
                            saveAIConfig();
                            state = State::SETTINGS_MENU;
                            drawSettingsMenu();
                            continue;
                        } else if (state == State::SETTINGS_MENU || state == State::VAULT_MENU) {
                            playNavSound();
                            returnToHub();
                            continue;
                        } else if (state == State::LIST_VIEW || state == State::WIKI_VIEW || state == State::DIR_CONFIG || state == State::DIR_BROWSER || state == State::CAMPAIGN_MGR || state == State::AI_CHAT) {
                            playNavSound();
                            if (state == State::LIST_VIEW) state = State::MAIN;
                            else if (state == State::DIR_BROWSER) state = State::DIR_CONFIG;
                            else if (state == State::DIR_CONFIG) returnToHub();
                              else if (state == State::CAMPAIGN_MGR) returnToHub();
                            else if (state == State::AI_CHAT && active_war_room != "") state = State::MAIN;
                            else { vault.package(); engine.clear(); state = State::HUB_MENU; }
                            
                            if (state == State::MAIN) loadMainMenu();
                            redrawCurrentState();
                            continue;
                        } else if (state == State::MAIN) {
                            playNavSound();
                            { vault.package(); engine.clear(); state = State::HUB_MENU; }
                            redrawCurrentState();
                            continue;
                        } else if (state == State::HUB_MENU) {
                            if (esc_count >= 3) {
                                state = State::MODAL_EXIT;
                                drawModalExit();
                            }
                            continue;
                        }
                    } else {
                        esc_count = 0;
                    }

                    if (state == State::HUB_MENU) {
                        if (key == VK_UP) {
                            playNavSound();
                            hub_selected_item = (hub_selected_item - 1 + HUB_ITEMS) % HUB_ITEMS;
                            drawHub();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            hub_selected_item = (hub_selected_item + 1) % HUB_ITEMS;
                            drawHub();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            if (hub_selected_item == 0 || hub_selected_item == 1) {
                                  if (vault.meta.name.empty()) {
                                      state = State::CAMPAIGN_MGR;
                                      vault_status_msg = "[000] ACTIVE CAMPAIGN REQUIRED. PLEASE LOAD OR CREATE.";
                                      drawCampaignMgr();
                                      continue;
                                  }
                                // Scan for .pws files in data_dirs and parent folders
                                for (const auto& dir : data_dirs) {
                                    if (std::filesystem::exists(dir)) {
                                        std::filesystem::path p(dir);
                                        std::vector<std::filesystem::path> check_paths = {p, p.parent_path()};
                                        for (const auto& cp : check_paths) {
                                            if (std::filesystem::exists(cp) && std::filesystem::is_directory(cp)) {
                                                for (const auto& entry : std::filesystem::directory_iterator(cp)) {
                                                    if (entry.path().extension() == ".pws") {
                                                        parsers::PWSExtract ext = parsers::PWSParser::parse(entry.path().string());
                                                        if (!ext.header.game_date.empty()) {
                                                            vault.meta.scenario = ext.header.scenario;
                                                            vault.meta.game_version = ext.header.game_version;
                                                            if (!ext.after_action_report.empty()) {
                                                                vault.saveReport("aar_" + ext.header.game_date + ".txt", ext.after_action_report);
                                                            }
                                                            if (!ext.sigint_report.empty()) {
                                                                vault.saveReport("sigint_" + ext.header.game_date + ".txt", ext.sigint_report);
                                                            }
                                                            vault.saveMeta();
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                // Process directories
                                engine.items.clear();
                                engine.stats = engine::DataStats();
                                for (const auto& dir : data_dirs) {
                                    engine.processDirectory(dir);
                                }
                                
                                if (!engine.items.empty()) {
                                    vault.saveIntelItems(engine.stats.turn_date, engine.items);
                                    vault.saveMeta();
                                } else {
                                    auto historical = vault.loadAllIntel();
                                    if (!historical.empty()) {
                                        engine.items = historical;
                                        engine.stats.turn_date = vault.meta.last_played;
                                        engine.stats.ops_days = 1;
                                        engine.stats.combat_days = 1;
                                        engine.stats.sigint_days = 1;
                                    }
                                }

                                if (engine.items.empty()) {
                                    dir_error_msg = "[!] NO VALID ARCHIVE FILES (OPERATIONS/SIGINT) FOUND IN THE CONFIGURED DIRECTORIES.";
                                    dir_selected = -1;
                                    state = State::DIR_CONFIG;
                                    drawDirConfig();
                                    playErrorSound();
                                } else {
                                    dir_error_msg = "";
                                    if (hub_selected_item == 0) {
                                        stats = engine.stats;
                                        active_war_room = "ALLIED";
                                        state = State::MAIN;
                                        loadMainMenu();
                                        drawMain();
                                    } else {
                                          stats = engine.stats;
                                          active_war_room = "JAPANESE";
                                          state = State::MAIN;
                                          loadMainMenu();
                                          drawMain();
                                      }
                                }
                            } else if (hub_selected_item == 2) {
                                  state = State::CAMPAIGN_MGR;
                                  campaign_selected = 0;
                                  vault_status_msg = "";
                                  drawCampaignMgr();
                            } else if (hub_selected_item == 3) {
                                state = State::SETTINGS_MENU;
                                settings_selected = 0;
                                drawSettingsMenu();
                            } else if (hub_selected_item == 4) {
                                state = State::WIKI_VIEW;
                                drawWiki();
                            }
                        }
                    } 
                    else if (state == State::VAULT_MENU) {
                        if (key == VK_UP) {
                            playNavSound();
                            vault_menu_selected = (vault_menu_selected - 1 + 3) % 3;
                            drawVaultMenu();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            vault_menu_selected = (vault_menu_selected + 1) % 3;
                            drawVaultMenu();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            if (vault_menu_selected == 0) {
                                state = State::CAMPAIGN_MGR;
                                campaign_selected = 0;
                                vault_status_msg = "";
                                campaign_input_mode = false;
                                zip_import_mode = false;
                                drawCampaignMgr();
                            } else if (vault_menu_selected == 1) {
                                dir_error_msg = "";
                                dir_selected = -1;
                                state = State::DIR_CONFIG;
                                drawDirConfig();
                            } else if (vault_menu_selected == 2) {
                                returnToHub();
                            }
                        }
                    } 
                    else if (state == State::MODAL_EXIT) {
                        if (ch == 'y' || ch == 'Y' || key == VK_RETURN) { playSelectSound(); vault.package(); engine.clear(); running = false; }
                        else if (ch == 'n' || ch == 'N' || key == VK_ESCAPE) { 
                            playNavSound();
                            returnToHub(); 
                        }
                    }
                    else if (state == State::MODAL_INFO) {
                        if (key == VK_RETURN) {
                            playNavSound();
                            if (current_menu[0].type == ItemType::BACK) state = State::LIST_VIEW;
                            else state = State::MAIN;
                            redrawCurrentState();
                        } else if (key == 'Y' || ch == 'y' || ch == 'Y') {
                            std::string yoink_dir = vault.vault_path + "\\yoink";
                            std::filesystem::create_directories(yoink_dir);
                            std::string bat_path = "scratch\\run_yoink.bat";
                            std::ofstream bat(bat_path);
                            bat << "@echo off\ncd /d \"" << yoink_dir << "\"\necho === YOINK PREDICTIONS ===\n\ntype *.md 2>nul || echo No predictions found yet.\necho.\npause\nexit\n";
                            bat.close();
                            std::system(("start \"\" \"" + bat_path + "\"").c_str());
                        }
                    }
                                        else if (state == State::WIKI_VIEW) {
                        if (wiki_selected_page == -1) {
                            if (key == VK_UP) {
                                playNavSound();
                                wiki_menu_selected = (wiki_menu_selected - 1 + wiki_pages.size()) % wiki_pages.size();
                                drawWiki();
                            } else if (key == VK_DOWN || key == VK_TAB) {
                                playNavSound();
                                wiki_menu_selected = (wiki_menu_selected + 1) % wiki_pages.size();
                                drawWiki();
                            } else if (key == VK_RETURN) {
                                playSelectSound();
                                wiki_selected_page = wiki_menu_selected;
                                wiki_scroll = 0;
                                drawWiki();
                            } else if (key == VK_ESCAPE) {
                                playNavSound();
                                returnToHub();
                            }
                        } else {
                            if (key == VK_UP) {
                                if (wiki_scroll > 0) {
                                    wiki_scroll--;
                                    drawWiki();
                                }
                            } else if (key == VK_DOWN) {
                                if (wiki_scroll < (int)wiki_pages[wiki_selected_page].content.size() - 14) {
                                    wiki_scroll++;
                                    drawWiki();
                                }
                            } else if (key == VK_ESCAPE) {
                                playNavSound();
                                returnToHub();
                            }
                        }
                    }
                    else if (state == State::SETTINGS_MENU) {
                        if (key == VK_UP) {
                            playNavSound();
                            settings_selected = (settings_selected - 1 + 3) % 3;
                            drawSettingsMenu();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            settings_selected = (settings_selected + 1) % 3;
                            drawSettingsMenu();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            if (settings_selected == 0) {
                                state = State::SETTINGS_AI;
                                ai_settings_selected = 0;
                                ai_input_mode = false;
                                settings_status_msg = "";
                                drawSettingsAI();
                            } else if (settings_selected == 1) {
                                dir_error_msg = "";
                                dir_selected = -1;
                                state = State::DIR_CONFIG;
                                drawDirConfig();
                            } else if (settings_selected == 2) {
                                returnToHub();
                            }
                        }
                    }
                    else if (state == State::SETTINGS_AI) {
                        if (ai_input_mode) {
                            if (key == VK_ESCAPE) {
                                playNavSound();
                                ai_input_mode = false;
                                ai_input_buffer = "";
                                drawSettingsAI();
                            } else if (key == VK_BACK) {
                                if (!ai_input_buffer.empty()) {
                                    ai_input_buffer.pop_back();
                                    drawSettingsAI();
                                }
                            } else if (key == VK_RETURN) {
                                playSelectSound();
                                if (ai_input_field == 0) {
                                    ai_config.gemini_api_key = ai_input_buffer;
                                    settings_status_msg = "Gemini API key updated!";
                                } else if (ai_input_field == 1) {
                                    ai_config.gemini_model = ai_input_buffer;
                                    settings_status_msg = "Gemini model updated to: " + ai_input_buffer;
                                } else if (ai_input_field == 2) {
                                    ai_config.local_endpoint = ai_input_buffer;
                                    settings_status_msg = "Local server endpoint updated to: " + ai_input_buffer;
                                } else if (ai_input_field == 3) {
                                    ai_config.local_model = ai_input_buffer;
                                    settings_status_msg = "Local model updated to: " + ai_input_buffer;
                                }
                                saveAIConfig();
                                ai_input_mode = false;
                                ai_input_buffer = "";
                                drawSettingsAI();
                            } else if (ch >= 32 && ch <= 126) {
                                ai_input_buffer += ch;
                                drawSettingsAI();
                            }
                        } else {
                            if (key == VK_UP) {
                                playNavSound();
                                ai_settings_selected = (ai_settings_selected - 1 + 6) % 6;
                                drawSettingsAI();
                            } else if (key == VK_DOWN || key == VK_TAB) {
                                playNavSound();
                                ai_settings_selected = (ai_settings_selected + 1) % 6;
                                drawSettingsAI();
                            } else if (key == VK_RETURN) {
                                playSelectSound();
                                if (ai_settings_selected == 0) {
                                    ai_config.provider = (ai_config.provider == "gemini") ? "local" : "gemini";
                                    saveAIConfig();
                                    settings_status_msg = "Switched active AI provider to: " + (ai_config.provider == "gemini" ? std::string("Google Gemini (Cloud)") : std::string("Local Model (Ollama/LM Studio)"));
                                    drawSettingsAI();
                                } else if (ai_settings_selected == 1) {
                                    ai_input_mode = true;
                                    ai_input_field = 0;
                                    ai_input_buffer = ai_config.gemini_api_key;
                                    drawSettingsAI();
                                } else if (ai_settings_selected == 2) {
                                    if (ai_config.gemini_model == "gemini-2.0-flash") ai_config.gemini_model = "gemini-2.5-flash";
                                    else if (ai_config.gemini_model == "gemini-2.5-flash") ai_config.gemini_model = "gemini-2.5-pro";
                                    else ai_config.gemini_model = "gemini-2.0-flash";
                                    saveAIConfig();
                                    settings_status_msg = "Model selected: " + ai_config.gemini_model;
                                    drawSettingsAI();
                                } else if (ai_settings_selected == 3) {
                                    ai_input_mode = true;
                                    ai_input_field = 2;
                                    ai_input_buffer = ai_config.local_endpoint;
                                    drawSettingsAI();
                                } else if (ai_settings_selected == 4) {
                                    ai_input_mode = true;
                                    ai_input_field = 3;
                                    ai_input_buffer = ai_config.local_model;
                                    drawSettingsAI();
                                } else if (ai_settings_selected == 5) {
                                    settings_status_msg = "Testing connection to " + (ai_config.provider == "gemini" ? std::string("Gemini API...") : std::string("Local Model..."));
                                    drawSettingsAI();
                                    saveAIConfig();
                                    ai_officer.start(vault.vault_path);
                                    ai_officer.sendMessage("{\"type\":\"test\"}", vault.vault_path);
                                    
                                    std::string resp = "";
                                    DWORD start_wait = GetTickCount();
                                    while (GetTickCount() - start_wait < 10000) {
                                        std::string l = ai_officer.readLine();
                                        if (!l.empty()) {
                                            size_t t = l.find("\"text\"");
                                            if (t != std::string::npos) {
                                                size_t q1 = l.find("\"", t + 6);
                                                size_t q2 = l.find("\"", q1 + 1);
                                                if (q1 != std::string::npos && q2 != std::string::npos) {
                                                    resp = l.substr(q1 + 1, q2 - q1 - 1);
                                                    break;
                                                }
                                            }
                                        }
                                        Sleep(50);
                                    }
                                    if (resp.empty()) resp = "No response from AI sidecar.";
                                    settings_status_msg = resp;
                                    drawSettingsAI();
                                }
                            } else if (ch == 't' || ch == 'T') {
                                ai_settings_selected = 5;
                                settings_status_msg = "Testing connection...";
                                drawSettingsAI();
                                saveAIConfig();
                                ai_officer.start(vault.vault_path);
                                ai_officer.sendMessage("{\"type\":\"test\"}", vault.vault_path);
                                std::string resp = "";
                                DWORD start_wait = GetTickCount();
                                while (GetTickCount() - start_wait < 10000) {
                                    std::string l = ai_officer.readLine();
                                    if (!l.empty()) {
                                        size_t t = l.find("\"text\"");
                                        if (t != std::string::npos) {
                                            size_t q1 = l.find("\"", t + 6);
                                            size_t q2 = l.find("\"", q1 + 1);
                                            if (q1 != std::string::npos && q2 != std::string::npos) {
                                                resp = l.substr(q1 + 1, q2 - q1 - 1);
                                                break;
                                            }
                                        }
                                    }
                                    Sleep(50);
                                }
                                if (resp.empty()) resp = "No response from AI sidecar.";
                                settings_status_msg = resp;
                                drawSettingsAI();
                            }
                        }
                    }
                    else if (state == State::DIR_CONFIG) {
                        if (key == VK_UP) {
                            playNavSound();
                            if (dir_selected == -1 && !data_dirs.empty()) {
                                dir_selected = (int)data_dirs.size() - 1;
                            } else if (dir_selected > 0) {
                                dir_selected--;
                            } else {
                                dir_selected = -1; // back to input
                            }
                            drawDirConfig();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            if (dir_selected == -1) {
                                if (!data_dirs.empty()) dir_selected = 0;
                            } else if (dir_selected < (int)data_dirs.size() - 1) {
                                dir_selected++;
                            } else {
                                dir_selected = -1; // wrap to input
                            }
                            drawDirConfig();
                        } else if (key == VK_RETURN) {
                            if (dir_selected == -1) {
                                // Open directory browser
                                playSelectSound();
                                browser_path = std::filesystem::current_path();
                                loadBrowserItems();
                                state = State::DIR_BROWSER;
                                drawDirBrowser();
                            } else {
                                // Delete selected directory
                                playSelectSound();
                                data_dirs.erase(data_dirs.begin() + dir_selected);
                                  if (!vault.meta.name.empty()) {
                                      vault.meta.source_dirs = data_dirs;
                                      vault.saveMeta();
                                  }
                                if (dir_selected >= (int)data_dirs.size()) {
                                    dir_selected = data_dirs.empty() ? -1 : (int)data_dirs.size() - 1;
                                }
                                drawDirConfig();
                            }
                        }
                    }
                    else if (state == State::DIR_BROWSER) {
                        if (key == VK_UP) {
                            playNavSound();
                            if (browser_selected == -1 && !browser_items.empty()) {
                                browser_selected = (int)browser_items.size() - 1;
                            } else if (browser_selected > 0) {
                                browser_selected--;
                            } else {
                                browser_selected = -1; // back to select button
                            }
                            drawDirBrowser();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            if (browser_selected == -1) {
                                if (!browser_items.empty()) browser_selected = 0;
                            } else if (browser_selected < (int)browser_items.size() - 1) {
                                browser_selected++;
                            } else {
                                browser_selected = -1; // wrap to select button
                            }
                            drawDirBrowser();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            if (browser_selected == -1) {
                                // Scan directory for .txt files before adding
                                bool has_txt = false;
                                try {
                                    for (const auto& entry : std::filesystem::directory_iterator(browser_path)) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                                            has_txt = true;
                                            break;
                                        }
                                    }
                                } catch (...) {}
                                
                                if (has_txt) {
                                    data_dirs.push_back(browser_path.string());
                                    if (!vault.meta.name.empty()) {
                                        vault.meta.source_dirs = data_dirs;
                                        vault.saveMeta();
                                    }
                                    dir_error_msg = "SUCCESS: Log files are processed and added the info to that campaign's knowledge base.";
                                } else {
                                    dir_error_msg = "[001] No log files (.txt) found in this directory.";
                                }
                                state = State::DIR_CONFIG;
                                drawDirConfig();
                            } else {
                                // Navigate to selected subfolder
                                std::string selected_name = browser_items[browser_selected];
                                if (selected_name == "..") {
                                    browser_path = browser_path.parent_path();
                                } else {
                                    browser_path /= selected_name;
                                }
                                loadBrowserItems();
                                drawDirBrowser();
                            }
                        }
                    }
                    else if (state == State::CAMPAIGN_MGR) {
                        if (campaign_input_mode) {
                            if (key == VK_ESCAPE) {
                                campaign_input_mode = false;
                                campaign_input_name = "";
                                drawCampaignMgr();
                            } else if (key == VK_BACK) {
                                if (!campaign_input_name.empty()) {
                                    campaign_input_name.pop_back();
                                    drawCampaignMgr();
                                }
                            } else if (key == VK_RETURN) {
                                if (!campaign_input_name.empty()) {
                                    vault.create(vault_root, campaign_input_name);
                                      vault.meta.source_dirs = data_dirs;
                                      vault.saveMeta();
                                      campaign_input_mode = false;
                                      campaign_input_name = "";
                                      chat_history = vault.loadChatHistory();
                                      stats = engine.stats;
                                      active_war_room = "ALLIED";
                                      state = State::MAIN;
                                      loadMainMenu();
                                      drawMain();
                                }
                            } else if (ch >= 32 && ch <= 126) {
                                campaign_input_name += ch;
                                drawCampaignMgr();
                            }
                        } else if (zip_import_mode) {
                            if (key == VK_ESCAPE) {
                                zip_import_mode = false;
                                zip_import_path = "";
                                drawCampaignMgr();
                            } else if (key == VK_BACK) {
                                if (!zip_import_path.empty()) {
                                    zip_import_path.pop_back();
                                    drawCampaignMgr();
                                }
                            } else if (key == VK_RETURN) {
                                if (!zip_import_path.empty()) {
                                    storage::CampaignVault::importZip(vault_root, zip_import_path);
                                    campaign_list = storage::CampaignVault::listCampaigns(vault_root);
                                    vault_status_msg = "CAMPAIGN ARCHIVE IMPORTED SUCCESSFULLY!";
                                    zip_import_mode = false;
                                    zip_import_path = "";
                                    drawCampaignMgr();
                                }
                            } else if (ch >= 32 && ch <= 126) {
                                zip_import_path += ch;
                                drawCampaignMgr();
                            }
                        } else {
                            if (key == VK_UP) {
                                playNavSound();
                                if (!campaign_list.empty()) {
                                    campaign_selected = (campaign_selected - 1 + campaign_list.size()) % campaign_list.size();
                                    drawCampaignMgr();
                                }
                            } else if (key == VK_DOWN || key == VK_TAB) {
                                playNavSound();
                                if (!campaign_list.empty()) {
                                    campaign_selected = (campaign_selected + 1) % campaign_list.size();
                                    drawCampaignMgr();
                                }
                            } else if (key == VK_RETURN) {
                                playSelectSound();
                                if (!campaign_list.empty() && campaign_selected < (int)campaign_list.size()) {
                                    std::string sel_name = campaign_list[campaign_selected];
                                    vault.load((std::filesystem::path(vault_root) / sel_name).string());
                                      engine.clear();
                                      data_dirs = vault.meta.source_dirs;
                                      for (const auto& dir : vault.meta.source_dirs) {
                                          if (std::filesystem::exists(dir)) {
                                              engine.processDirectory(dir);
                                          }
                                      }
                                      if (!engine.items.empty()) {
                                          vault.saveIntelItems(engine.stats.turn_date, engine.items);
                                      } else {
                                          auto historical = vault.loadAllIntel();
                                          if (!historical.empty()) {
                                              engine.items = historical;
                                              engine.stats.turn_date = vault.meta.last_played;
                                          }
                                      }
                                      chat_history = vault.loadChatHistory();
                                      stats = engine.stats;
                                      active_war_room = "ALLIED";
                                      state = State::MAIN;
                                      loadMainMenu();
                                      drawMain();
                                }
                            } else if (ch == 'n' || ch == 'N') {
                                playNavSound();
                                campaign_input_mode = true;
                                campaign_input_name = "";
                                drawCampaignMgr();
                            } else if (ch == 'e' || ch == 'E') {
                                playSelectSound();
                                std::string zip_p = vault.exportZip();
                                vault_status_msg = "EXPORTED ARCHIVE: " + zip_p;
                                drawCampaignMgr();
                            } else if (ch == 'i' || ch == 'I') {
                                playNavSound();
                                zip_import_mode = true;
                                zip_import_path = "";
                                drawCampaignMgr();
                            } else if (key == VK_ESCAPE) {
                                playNavSound();
                                state = State::VAULT_MENU;
                                drawVaultMenu();
                            }
                        }
                    }
                    else if (state == State::AI_CHAT) {
                        if (key == VK_ESCAPE) {
                            playNavSound();
                            if (active_war_room != "") {
                                state = State::MAIN;
                                loadMainMenu();
                                drawMain();
                            } else {
                                returnToHub();
                            }
                        } else if (key == VK_BACK) {
                            if (!chat_input.empty()) {
                                chat_input.pop_back();
                                drawAIChat();
                            }
                        } else if (key == VK_RETURN) {
                            if (!chat_input.empty()) {
                                std::string user_msg = chat_input;
                                chat_input = "";
                                chat_history.push_back({"user", user_msg});
                                vault.appendChat("user", user_msg);
                                ai_status_msg = "STAFF OFFICER ANALYZING INTELLIGENCE & COMMUNICATING...";
                                drawAIChat();

                                ai_officer.sendMessage(user_msg, vault.vault_path);

                                std::string full_response = "";
                                DWORD start_wait = GetTickCount();
                                while (GetTickCount() - start_wait < 20000) {
                                    std::string line = ai_officer.readLine();
                                    if (!line.empty()) {
                                        if (line.find("\"type\": \"error\"") != std::string::npos ||
                                            line.find("\"type\":\"error\"") != std::string::npos) {
                                            size_t t = line.find("\"text\"");
                                            if (t != std::string::npos) {
                                                size_t q1 = line.find("\"", t + 6);
                                                size_t q2 = line.find("\"", q1 + 1);
                                                if (q1 != std::string::npos && q2 != std::string::npos) {
                                                    full_response = "[ADVISORY] " + line.substr(q1 + 1, q2 - q1 - 1);
                                                }
                                            }
                                            break;
                                        }
                                        if (line.find("\"text\"") != std::string::npos) {
                                            size_t t = line.find("\"text\"");
                                            size_t q1 = line.find("\"", t + 6);
                                            size_t q2 = line.find("\"", q1 + 1);
                                            if (q1 != std::string::npos && q2 != std::string::npos) {
                                                std::string chunk = line.substr(q1 + 1, q2 - q1 - 1);
                                                for (size_t ci = 0; ci < chunk.length(); ++ci) {
                                                    if (chunk[ci] == '\\' && ci + 1 < chunk.length() && chunk[ci+1] == 'n') {
                                                        full_response += '\n';
                                                        ci++;
                                                    } else {
                                                        full_response += chunk[ci];
                                                    }
                                                }
                                            }
                                        }
                                        if (line.find("\"done\": true") != std::string::npos ||
                                            line.find("\"done\":true") != std::string::npos) {
                                            break;
                                        }
                                    }
                                    Sleep(50);
                                }

                                if (full_response.empty()) {
                                    full_response = "Commander, staff officer is awaiting fresh intelligence or connection to Gemini API. Please ensure your GEMINI_API_KEY environment variable is set or saved in ai/api_key.txt.";
                                }

                                chat_history.push_back({"officer", full_response});
                                vault.appendChat("assistant", full_response);
                                ai_status_msg = "";
                                drawAIChat();
                            }
                        } else if (ch >= 32 && ch <= 126) {
                            chat_input += ch;
                            drawAIChat();
                        }
                    }
                    else if (state == State::MAIN || state == State::LIST_VIEW) {
                        if (key == VK_UP) {
                            playNavSound();
                            selected_item = (selected_item - 1 + current_menu.size()) % current_menu.size();
                            redrawCurrentState();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            selected_item = (selected_item + 1) % current_menu.size();
                            redrawCurrentState();
                        } else if (state == State::MAIN && (ch == 'a' || ch == 'A')) {
                            state = State::AI_CHAT;
                            ai_officer.start(vault.vault_path);
                            drawAIChat();
                        } else if ((state == State::MAIN || state == State::LIST_VIEW) && (key == 'Y' || ch == 'y' || ch == 'Y')) {
                            // ponytail: generate bat file to bypass cmd quote escaping hell
                            std::string yoink_dir = vault.vault_path + "\\yoink";
                            std::filesystem::create_directories(yoink_dir);
                            std::string bat_path = "scratch\\run_yoink.bat";
                            std::ofstream bat(bat_path);
                            bat << "@echo off\ncd /d \"" << yoink_dir << "\"\necho === YOINK PREDICTIONS ===\n\ntype *.md 2>nul || echo No predictions found yet.\necho.\npause\nexit\n";
                            bat.close();
                            std::system(("start \"\" \"" + bat_path + "\"").c_str());
                        } else if (state == State::MAIN && (ch == 'c' || ch == 'C')) {
                            if (first_c_idx != -1) selected_item = first_c_idx; 
                            playNavSound();
                            drawMain();
                        } else if (state == State::MAIN && (ch == 'h' || ch == 'H')) {
                            if (first_h_idx != -1) selected_item = first_h_idx; 
                            playNavSound();
                            drawMain();
                        } else if (state == State::MAIN && (ch == 'd' || ch == 'D')) {
                            if (first_d_idx != -1) selected_item = first_d_idx; 
                            playNavSound();
                            drawMain();
                        } else if (ch == 'x' || ch == 'X') {
                            selected_item = 0; 
                            playNavSound();
                            redrawCurrentState();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            auto type = current_menu[selected_item].type;
                            if (type == ItemType::EXIT) {
                                returnToHub();
                            } else if (type == ItemType::BACK) {
                                state = State::MAIN;
                                loadMainMenu();
                                drawMain();
                            } else if (type == ItemType::MORE_CRITICAL) {
                                state = State::LIST_VIEW;
                                loadListView(engine::IntelCategory::CRITICAL);
                                drawListView();
                            } else if (type == ItemType::MORE_HVT) {
                                state = State::LIST_VIEW;
                                loadListView(engine::IntelCategory::HVT);
                                drawListView();
                            } else if (type == ItemType::MORE_DISCOVERED) {
                                state = State::LIST_VIEW;
                                loadListView(engine::IntelCategory::DISCOVERED);
                                drawListView();
                            } else if (type == ItemType::INTEL) {
                                if (current_menu[selected_item].solidity > 0) {
                                    state = State::MODAL_INFO;
                                    drawModalInfo();
                                } else {
                                    playErrorSound();
                                }
                            }
                        }
                    }
                }
            }

            clear();
            setColor(7);
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(hOut, &cursorInfo);
        }
    };

    void Dashboard::render(engine::IntelligenceEngine& engine) {
        InteractiveTUI tui(engine);
        tui.run();
    }
}