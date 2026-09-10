#include "Dashboard.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

namespace ui {

    enum class State { SPLASH, MAIN, MODAL_INFO, MODAL_EXIT, LIST_VIEW };
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
        State state = State::SPLASH;
        int selected_item = 1; 
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

    public:
        InteractiveTUI(const engine::IntelligenceEngine& eng) : engine(eng) {
            stats = engine.stats;
        }

    private:
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
            
            current_menu.push_back({ItemType::EXIT, "[X] EXIT", "", 0, 1});
            
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
            
            // Discovered Allied Units
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
                current_menu.push_back({ItemType::INTEL, "> No allied units discovered.", "", 0, y++});
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

        void drawSplash() {
            clear();
            setColor(10); // Light green
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
        ███╗   ███╗  ██╗  ██╗  ████████╗  ██╗  ██╗  ██╗   ██╗  ██████╗ 
        ████╗ ████║  ██║  ██║  ╚══██╔══╝  ██║  ██║  ██║   ██║  ██╔══██╗
        ██╔████╔██║  ██║  ██║     ██║     ███████║  ██║   ██║  ██████╔╝
        ██║╚██╔╝██║  ██║  ██║     ██║     ██╔══██║  ██║   ██║  ██╔══██╗
        ██║ ╚═╝ ██║▄ ██║  ██║▄    ██║  ▄  ██║  ██║▄ ╚██████╔╝▄ ██║  ██║▄
        ╚═╝     ╚═╝▀ ╚══════╝▀    ╚═╝  ▀  ╚═╝  ╚═╝▀  ╚═════╝ ▀ ╚═╝  ╚═╝▀
            )" << "\n";
            
            setColor(10); // Light green
            std::cout << "        INTERFACE 1943.12 INITIALIZED\n";
            std::cout << "        ============================================================\n\n";
            
            setColor(160); // Background Green (160 = Green background, black foreground or similar)
            // Windows console: 10 is light green, 2 is dark green. 
            // 2 << 4 = 32 (Dark Green BG). 10 << 4 = 160 (Light Green BG).
            // Let's use 160 (Light Green BG, Black Text) for the selected feel.
            std::cout << "        COMBAT SIMULATION        \n";
            
            setColor(2); // Dark Green
            std::cout << "        NAVIGATION GRID\n";
            std::cout << "        WEAPONS ARRAY\n";
            std::cout << "        SYSTEM DIAGNOSTICS\n";
            std::cout << "        LOGOUT\n";
            
            std::cout << "\n\n\n";
            setColor(10); // Light green
            std::cout << "                                                                 \xDB v1.0.7\n";
            
            // Revert back to safe color, hide cursor behavior during wait
            setColor(2);
        }

        void drawMain() {
            clear();
            setColor(8); 
            std::cout << "===============================================================================\n";
            setColor(15);
            std::cout << " [ ALLIED COMBAT INFORMATION CENTER ]                     Turn: " << stats.turn_date << "\n";
            setColor(8);
            std::cout << "===============================================================================\n";
            
            setColor(3);
            std::cout << " [ DATA SOURCES SYNCED ]\n";
            setColor(7);
            std::cout << " > Operations Reports : Last " << (stats.ops_days == 0 ? 1 : stats.ops_days) << " Days\n";
            std::cout << " > Combat Reports     : Last " << (stats.combat_days == 0 ? 1 : stats.combat_days) << " Days\n";
            std::cout << " > SIGINT             : Last " << (stats.sigint_days == 0 ? 1 : stats.sigint_days) << " Days\n";
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n\n";

            setColor(12);
            std::cout << " [C]ritical alerts\n";
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n";
            
            if (first_h_idx != -1) {
                gotoxy(0, current_menu[first_h_idx].y_pos - 2);
                setColor(14);
                std::cout << " [H]igh Value Target Tracker\n";
                setColor(8);
                std::cout << "-------------------------------------------------------------------------------\n";
            }
            
            if (first_d_idx != -1) {
                gotoxy(0, current_menu[first_d_idx].y_pos - 2);
                setColor(13); // Magenta
                std::cout << " [D]iscovered Allied Units\n";
                setColor(8);
                std::cout << "-------------------------------------------------------------------------------\n";
            }

            for(int i=0; i < current_menu.size(); ++i) {
                gotoxy(i==0 ? 70 : 2, current_menu[i].y_pos);
                if(i == selected_item) {
                    setColor(240); // Black on White
                } else {
                    setColor(i==0 ? 12 : 7); // Red for exit, white for rest
                    if (current_menu[i].type == ItemType::MORE_CRITICAL || current_menu[i].type == ItemType::MORE_HVT || current_menu[i].type == ItemType::MORE_DISCOVERED) {
                        setColor(11); // Cyan for more buttons
                    }
                }
                std::cout << current_menu[i].title;
                setColor(7); // reset
            }
        }

        void drawListView() {
            clear();
            setColor(8);
            std::cout << "===============================================================================\n";
            setColor(15);
            std::cout << " [ ALLIED COMBAT INFORMATION CENTER ]                     Turn: " << stats.turn_date << "\n";
            setColor(8);
            std::cout << "===============================================================================\n";
            
            if (list_view_category == engine::IntelCategory::CRITICAL) { setColor(12); std::cout << " [ FULL LIST ] CRITICAL ALERTS\n"; }
            else if (list_view_category == engine::IntelCategory::HVT) { setColor(14); std::cout << " [ FULL LIST ] HIGH VALUE TARGETS\n"; }
            else if (list_view_category == engine::IntelCategory::DISCOVERED) { setColor(13); std::cout << " [ FULL LIST ] DISCOVERED ALLIED UNITS\n"; }
            
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n";
            
            // Render Back button
            gotoxy(70, 1);
            if (selected_item == 0) setColor(240); else setColor(12);
            std::cout << current_menu[0].title;
            
            // Pagination logic
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
                    if (i == selected_item) setColor(240); else setColor(7);
                    std::cout << current_menu[i].title;
                    current_menu[i].y_pos = draw_y; 
                    draw_y++;
                } else {
                    current_menu[i].y_pos = -1; // offscreen
                }
            }
            setColor(7);
            
            // Draw scroll indicators
            if (list_scroll_offset > 0) {
                gotoxy(75, 5); setColor(11); std::cout << "^";
            }
            if (list_scroll_offset + 1 + MAX_VISIBLE_LIST <= current_menu.size()) {
                gotoxy(75, draw_y - 1); setColor(11); std::cout << "v";
            }
            setColor(7);
        }

        void drawModalInfo() {
            auto& item = current_menu[selected_item];
            if (item.solidity == 0) return; // Empty alert msg

            setColor(31); // White on Blue
            
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
            
            current_y++; // spacer
            gotoxy(12, current_y++); std::cout << "SOLIDITY (CONFIDENCE): " << item.solidity << "%";
            
            int meter_len = item.solidity / 5;
            gotoxy(12, current_y++);
            std::cout << "[";
            for(int i=0; i<20; ++i) std::cout << (i < meter_len ? "#" : ".");
            std::cout << "]";

            gotoxy(12, current_y + 1); std::cout << "Press ENTER or ESC to close.";
            setColor(7);
        }

        void drawModalExit() {
            setColor(79); // White on Red
            for(int i=5; i<10; ++i) {
                gotoxy(20, i);
                std::cout << "                                        ";
            }
            gotoxy(22, 6); std::cout << "SYSTEM SHUTDOWN";
            gotoxy(22, 8); std::cout << "Do you want to exit? (Y/N)";
            setColor(7);
        }

        void redrawCurrentState() {
            if (state == State::MAIN) drawMain();
            else if (state == State::LIST_VIEW) drawListView();
        }

    public:
        void run() {
            hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            hIn = GetStdHandle(STD_INPUT_HANDLE);

            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo(hOut, &cursorInfo);
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(hOut, &cursorInfo);

            drawSplash();

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
                            if (current_menu[0].type == ItemType::BACK) state = State::LIST_VIEW;
                            else state = State::MAIN;
                            redrawCurrentState();
                            continue;
                        } else if (state == State::LIST_VIEW) {
                            state = State::MAIN;
                            loadMainMenu();
                            drawMain();
                            continue;
                        } else if (state == State::MAIN) {
                            if (esc_count >= 3) {
                                state = State::MODAL_EXIT;
                                drawModalExit();
                            }
                            continue;
                        }
                    } else {
                        esc_count = 0;
                    }

                    if (state == State::SPLASH) {
                        state = State::MAIN;
                        loadMainMenu();
                        drawMain();
                    } 
                    else if (state == State::MODAL_EXIT) {
                        if (ch == 'y' || ch == 'Y' || key == VK_RETURN) { running = false; }
                        else if (ch == 'n' || ch == 'N' || key == VK_ESCAPE) { state = State::MAIN; drawMain(); }
                    }
                    else if (state == State::MODAL_INFO) {
                        if (key == VK_RETURN) {
                            if (current_menu[0].type == ItemType::BACK) state = State::LIST_VIEW;
                            else state = State::MAIN;
                            redrawCurrentState();
                        }
                    }
                    else if (state == State::MAIN || state == State::LIST_VIEW) {
                        if (key == VK_UP) {
                            selected_item = (selected_item - 1 + current_menu.size()) % current_menu.size();
                            redrawCurrentState();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            selected_item = (selected_item + 1) % current_menu.size();
                            redrawCurrentState();
                        } else if (state == State::MAIN && (ch == 'c' || ch == 'C')) {
                            if (first_c_idx != -1) selected_item = first_c_idx; 
                            drawMain();
                        } else if (state == State::MAIN && (ch == 'h' || ch == 'H')) {
                            if (first_h_idx != -1) selected_item = first_h_idx; 
                            drawMain();
                        } else if (state == State::MAIN && (ch == 'd' || ch == 'D')) {
                            if (first_d_idx != -1) selected_item = first_d_idx; 
                            drawMain();
                        } else if (ch == 'x' || ch == 'X') {
                            selected_item = 0; 
                            redrawCurrentState();
                        } else if (key == VK_RETURN) {
                            auto type = current_menu[selected_item].type;
                            if (type == ItemType::EXIT) {
                                state = State::MODAL_EXIT;
                                drawModalExit();
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