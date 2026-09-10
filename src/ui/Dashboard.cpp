#include "Dashboard.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <ctime>

namespace ui {

    enum class State { HUB_MENU, MAIN, MODAL_INFO, MODAL_EXIT, LIST_VIEW, WIKI_VIEW, DIR_CONFIG, DIR_BROWSER };
    enum class ItemType { EXIT, BACK, INTEL, MORE_CRITICAL, MORE_HVT, MORE_DISCOVERED };

    struct MenuItem {
        ItemType type;
        std::string title;
        std::string reasoning;
        int solidity;
        int y_pos;
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
        const int HUB_ITEMS = 5;
        
        // Directory browser
        std::filesystem::path browser_path;
        std::vector<std::string> browser_items;
        int browser_selected = 0;
        int browser_scroll = 0;
        // Intro screen arts
        std::vector<std::string> intro_arts;
        int current_art_index = 0;

    public:
        InteractiveTUI(engine::IntelligenceEngine& eng) : engine(eng) {
            stats = engine.stats;
            data_dirs.push_back("data/samples"); // default directory
            
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
        }

    private:
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
            std::cout << "     ============================================================\n\n";
            
            std::string hub_items[5] = {"ALLIED WAR ROOM", "JAPANESE WAR ROOM", "WIKI", "ADD FILES TO PROCESS", "PREFERENCES"};
            for(int i=0; i<HUB_ITEMS; i++) {
                if (i == hub_selected_item) {
                    setColor(160);
                    std::cout << "     " << hub_items[i];
                    for(size_t j=hub_items[i].length(); j<25; j++) std::cout << " ";
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
            gotoxy(box_left, box_top+1); std::cout << "| "; setColor(10); std::cout << "WHAT'S NEW - v0.01             "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+2); std::cout << "|---------------------------------|";
            gotoxy(box_left, box_top+3); std::cout << "| "; setColor(10); std::cout << "- MUTHUR CRT Interface         "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+4); std::cout << "| "; setColor(10); std::cout << "- Directory Config feature     "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+5); std::cout << "| "; setColor(10); std::cout << "- Tactical Early Warnings      "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+6); std::cout << "| "; setColor(10); std::cout << "- HVT & Discovered Tracking    "; setColor(2); std::cout << "|";
            gotoxy(box_left, box_top+7); std::cout << "+---------------------------------+";
            
            // Get window height and position the version at the bottom
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hOut, &csbi);
            int bottom = csbi.srWindow.Bottom - csbi.srWindow.Top;
            gotoxy(64, bottom);
            setColor(10);
            std::cout << "v0.01";
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
            std::cout << " > Combat Reports     : Last " << (stats.combat_days == 0 ? 1 : stats.combat_days) << " Days\n";
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
        }

        void drawListView() {
            clear();
            setColor(2);
            std::cout << "===============================================================================\n";
            setColor(10);
            std::cout << " [ " << active_war_room << " COMBAT INFORMATION CENTER ]                     Turn: " << stats.turn_date << "\n";
            setColor(2);
            std::cout << "===============================================================================\n";
            
            setColor(10);
            if (list_view_category == engine::IntelCategory::CRITICAL) std::cout << " [ FULL LIST ] CRITICAL ALERTS\n";
            else if (list_view_category == engine::IntelCategory::HVT) std::cout << " [ FULL LIST ] HIGH VALUE TARGETS\n";
            else if (list_view_category == engine::IntelCategory::DISCOVERED) std::cout << " [ FULL LIST ] DISCOVERED UNITS\n";
            
            setColor(2);
            std::cout << "-------------------------------------------------------------------------------\n";
            
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
                    gotoxy(2, draw_y);
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
            std::cout << "===============================================================================\n";
            setColor(10);
            std::cout << " [ M.U.T.H.U.R WIKI DATABASE ]\n";
            setColor(2);
            std::cout << "===============================================================================\n\n";
            setColor(10);
            std::cout << "\n\n\n";
            std::cout << "                   +--------------------------------------+\n";
            std::cout << "                   |                                      |\n";
            std::cout << "                   |   [ SECTION UNDER DEVELOPMENT ]      |\n";
            std::cout << "                   |                                      |\n";
            std::cout << "                   |   This module is not yet active.      |\n";
            std::cout << "                   |   Check back in a future release.     |\n";
            std::cout << "                   |                                      |\n";
            std::cout << "                   +--------------------------------------+\n\n\n";
            
            setColor(2);
            std::cout << "                   Press ESC or ENTER to return.\n";
        }

        void drawDirConfig() {
            clear();
            setColor(2);
            std::cout << "===============================================================================\n";
            setColor(10);
            std::cout << " [ FILE DIRECTORY CONFIGURATION ]\n";
            setColor(2);
            std::cout << "===============================================================================\n\n";

            setColor(10);
            std::cout << " Active data directories:\n";
            setColor(2);
            std::cout << " -----------------------------------------------------------------------\n";
            
            if (data_dirs.empty()) {
                setColor(2);
                std::cout << "   (none configured)\n";
            } else {
                for (int i = 0; i < (int)data_dirs.size(); i++) {
                    if (dir_selected == i) {
                        setColor(160);
                        std::cout << "   [" << (i+1) << "] " << data_dirs[i];
                        // Pad
                        for (size_t p = data_dirs[i].length(); p < 55; p++) std::cout << " ";
                        std::cout << "[DEL]";
                        std::cout << "\n";
                    } else {
                        setColor(10);
                        std::cout << "   [" << (i+1) << "] " << data_dirs[i] << "\n";
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
                std::cout << " > [ BROWSE FOR DIRECTORY ]                                      \n";
            } else {
                setColor(2);
                std::cout << " > [ BROWSE FOR DIRECTORY ]                                      \n";
            }
            
            setColor(2);
            std::cout << "\n -----------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Select/Delete  |  ESC: Back to M.U.T.H.U.R\n";
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
            std::cout << "===============================================================================\n";
            setColor(10);
            std::cout << " [ SELECT DIRECTORY ]\n";
            setColor(2);
            std::cout << "===============================================================================\n\n";

            setColor(15);
            std::string p_str = browser_path.string();
            if (p_str.length() > 60) p_str = "..." + p_str.substr(p_str.length() - 57);
            std::cout << " CURRENT: " << p_str << "\n\n";

            // The 'Select this directory' button is index 0 visually, but let's make it part of the scroll list.
            // Actually, let's keep it fixed at the top.
            if (browser_selected == -1) {
                setColor(160);
                std::cout << " > [ CHOOSE THIS DIRECTORY ]                                     \n\n";
            } else {
                setColor(10);
                std::cout << " > [ CHOOSE THIS DIRECTORY ]                                     \n\n";
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
                    std::cout << "   " << browser_items[idx];
                    for(size_t p=browser_items[idx].length(); p<65; p++) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(10);
                    std::cout << "   " << browser_items[idx] << "\n";
                }
            }

            setColor(2);
            std::cout << " -----------------------------------------------------------------------\n";
            std::cout << " UP/DOWN: Navigate  |  ENTER: Enter/Select  |  ESC: Cancel\n";
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

            // Solidity
            current_y++;
            gotoxy(box_left + 3, current_y);
            if (item.solidity >= 90) {
                // Blink for high confidence
                setColor(10);
                std::cout << "\033[5m" << "SOLIDITY: " << item.solidity << "%" << "\033[0m";
            } else {
                setColor(10);
                std::cout << "SOLIDITY: ";
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

            while(running) {
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
                        } else if (state == State::LIST_VIEW || state == State::WIKI_VIEW || state == State::DIR_CONFIG || state == State::DIR_BROWSER) {
                            playNavSound();
                            if (state == State::LIST_VIEW) state = State::MAIN;
                            else if (state == State::DIR_BROWSER) state = State::DIR_CONFIG;
                            else state = State::HUB_MENU;
                            
                            if (state == State::MAIN) loadMainMenu();
                            redrawCurrentState();
                            continue;
                        } else if (state == State::MAIN) {
                            playNavSound();
                            state = State::HUB_MENU;
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
                            if (hub_selected_item == 0) {
                                // Reprocess all configured directories
                                engine.items.clear();
                                engine.stats = engine::DataStats();
                                for (const auto& dir : data_dirs) {
                                    engine.processDirectory(dir);
                                }
                                stats = engine.stats;
                                active_war_room = "ALLIED";
                                state = State::MAIN;
                                loadMainMenu();
                                drawMain();
                            } else if (hub_selected_item == 1 || hub_selected_item == 2 || hub_selected_item == 4) {
                                state = State::WIKI_VIEW;
                                drawWiki();
                            } else if (hub_selected_item == 3) {
                                dir_selected = -1;
                                state = State::DIR_CONFIG;
                                drawDirConfig();
                            }
                        }
                    } 
                    else if (state == State::MODAL_EXIT) {
                        if (ch == 'y' || ch == 'Y' || key == VK_RETURN) { playSelectSound(); running = false; }
                        else if (ch == 'n' || ch == 'N' || key == VK_ESCAPE) { 
                            playNavSound();
                            state = State::HUB_MENU; drawHub(); 
                        }
                    }
                    else if (state == State::MODAL_INFO) {
                        if (key == VK_RETURN) {
                            playNavSound();
                            if (current_menu[0].type == ItemType::BACK) state = State::LIST_VIEW;
                            else state = State::MAIN;
                            redrawCurrentState();
                        }
                    }
                    else if (state == State::WIKI_VIEW) {
                        if (key == VK_RETURN || key == VK_ESCAPE) {
                            playNavSound();
                            state = State::HUB_MENU;
                            drawHub();
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
                                // Add this path and go back to config
                                data_dirs.push_back(browser_path.string());
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
                    else if (state == State::MAIN || state == State::LIST_VIEW) {
                        if (key == VK_UP) {
                            playNavSound();
                            selected_item = (selected_item - 1 + current_menu.size()) % current_menu.size();
                            redrawCurrentState();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            selected_item = (selected_item + 1) % current_menu.size();
                            redrawCurrentState();
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
                                state = State::HUB_MENU;
                                drawHub();
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