#include "Dashboard.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

namespace ui {

    enum class State { HUB_MENU, MAIN, MODAL_INFO, MODAL_EXIT, LIST_VIEW, WIKI_VIEW };
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
        const engine::IntelligenceEngine& engine;
        engine::DataStats stats;
        
        engine::IntelCategory list_view_category = engine::IntelCategory::CRITICAL;
        int list_scroll_offset = 0;
        const int MAX_VISIBLE_LIST = 15;
        
        int first_c_idx = -1;
        int first_h_idx = -1;
        int first_d_idx = -1;
        
        std::string active_war_room = "ALLIED";

    public:
        InteractiveTUI(const engine::IntelligenceEngine& eng) : engine(eng) {
            stats = engine.stats;
        }

    private:
        void playNavSound() { Beep(400, 20); }
        void playSelectSound() { Beep(800, 40); }
        void playErrorSound() { Beep(200, 100); }

        void clear() { system("cls"); }
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
            setColor(10); // Light green for CRT theme
            std::cout << "\n";
            std::cout << R"(
                                         |
                                       __|__
                                ___   |  |  |   ___
                               /   \__|__|__|__/   \
                           ___/                     \___
        __________________/                             \_________________
        \                                                                /
         \______________________________________________________________/
            )" << "\n";
            
            std::cout << R"(
        __  __   _    _   _______   _    _   _    _   _____ 
        |  \/  | | |  | | |__   __| | |  | | | |  | | |  __ \
        | \  / | | |  | |    | |    | |__| | | |  | | | |__) |
        | |\/| | | |  | |    | |    |  __  | | |  | | |  _  /
        | |  | | | |__| |    | |    | |  | | | |__| | | | \ \
        |_|  |_|  \____/     |_|    |_|  |_|  \____/  |_|  \_\
            )" << "\n";
            
            setColor(10); 
            std::cout << "        INTERFACE 1943.12 INITIALIZED\n";
            std::cout << "        ============================================================\n\n";
            
            std::string hub_items[3] = {"ALLIED WAR ROOM", "JAPANESE WAR ROOM", "WIKI"};
            for(int i=0; i<3; i++) {
                if (i == hub_selected_item) {
                    setColor(160); // Green BG, Black FG
                    std::cout << "        " << hub_items[i];
                    for(size_t j=hub_items[i].length(); j<25; j++) std::cout << " ";
                    std::cout << "\n";
                } else {
                    setColor(2); // Dark Green
                    std::cout << "        " << hub_items[i] << "\n";
                }
            }
            
            std::cout << "\n\n\n\n";
            setColor(10);
            std::cout << "                                                                 v0.01\n";
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
            std::cout << " > No Wiki entries available in this sector.\n";
            std::cout << " > Awaiting datalink transmission...\n\n\n";
            
            setColor(160);
            std::cout << " [BACK] ";
            setColor(2);
        }

        void drawModalInfo() {
            auto& item = current_menu[selected_item];
            if (item.solidity == 0) return;

            setColor(160); // Green on Black... actually, Modal should be dark background with light green text
            // Let's use 32 (Dark Green BG) with 10 (Light green text) = 42
            setColor(42); 
            
            for(int i=6; i<18; ++i) {
                gotoxy(10, i);
                std::cout << "                                                                "; // 64 spaces
            }
            
            gotoxy(12, 7);  std::cout << "INTEL ASSESSMENT";
            
            std::vector<std::string> wrapped_reasoning = wordWrap("REASONING: " + item.reasoning, 60);
            int current_y = 9;
            for(const auto& line : wrapped_reasoning) {
                gotoxy(12, current_y++);
                std::cout << line;
            }
            
            current_y++; 
            gotoxy(12, current_y++); std::cout << "SOLIDITY (CONFIDENCE): " << item.solidity << "%";
            
            int meter_len = item.solidity / 5;
            gotoxy(12, current_y++);
            std::cout << "[";
            for(int i=0; i<20; ++i) std::cout << (i < meter_len ? "#" : ".");
            std::cout << "]";

            gotoxy(12, current_y + 1); std::cout << "Press ENTER or ESC to close.";
            setColor(2);
        }

        void drawModalExit() {
            setColor(42); 
            for(int i=5; i<10; ++i) {
                gotoxy(20, i);
                std::cout << "                                        ";
            }
            gotoxy(22, 6); std::cout << "SYSTEM LOGOUT";
            gotoxy(22, 8); std::cout << "Confirm Logout? (Y/N)";
            setColor(2);
        }

        void redrawCurrentState() {
            if (state == State::HUB_MENU) drawHub();
            else if (state == State::MAIN) drawMain();
            else if (state == State::LIST_VIEW) drawListView();
            else if (state == State::WIKI_VIEW) drawWiki();
        }

    public:
        void run() {
            hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            hIn = GetStdHandle(STD_INPUT_HANDLE);

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
                        } else if (state == State::LIST_VIEW || state == State::WIKI_VIEW) {
                            playNavSound();
                            if (state == State::LIST_VIEW) state = State::MAIN;
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
                            hub_selected_item = (hub_selected_item - 1 + 3) % 3;
                            drawHub();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            playNavSound();
                            hub_selected_item = (hub_selected_item + 1) % 3;
                            drawHub();
                        } else if (key == VK_RETURN) {
                            playSelectSound();
                            if (hub_selected_item == 0) {
                                active_war_room = "ALLIED";
                                state = State::MAIN;
                                loadMainMenu();
                                drawMain();
                            } else if (hub_selected_item == 1) {
                                active_war_room = "JAPANESE";
                                state = State::MAIN;
                                loadMainMenu();
                                drawMain();
                            } else if (hub_selected_item == 2) {
                                state = State::WIKI_VIEW;
                                drawWiki();
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

    void Dashboard::render(const engine::IntelligenceEngine& engine) {
        InteractiveTUI tui(engine);
        tui.run();
    }
}