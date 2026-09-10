#include "Dashboard.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

namespace ui {

    enum class State { SPLASH, MAIN, MODAL_INFO, MODAL_EXIT };

    struct MenuItem {
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
        
        std::vector<MenuItem> items;
        engine::DataStats stats;
        
        int first_c_idx = -1;
        int first_g_idx = -1;

    public:
        InteractiveTUI(const engine::IntelligenceEngine& engine) {
            stats = engine.stats;
            
            // Build items list
            items.push_back({"[X] EXIT", "", 0, 1});
            
            int current_y = 10;
            
            // Critical
            for (const auto& intel : engine.items) {
                if (intel.is_critical) {
                    if (first_c_idx == -1) first_c_idx = items.size();
                    items.push_back({intel.title, intel.reasoning, intel.solidity, current_y++});
                }
            }
            if (first_c_idx == -1) {
                items.push_back({"> No critical alerts at this time.", "No data.", 0, current_y++});
            }
            
            current_y += 3; // Space for the HVT header
            
            // HVT Tracker
            for (const auto& intel : engine.items) {
                if (!intel.is_critical) {
                    if (first_g_idx == -1) first_g_idx = items.size();
                    items.push_back({intel.title, intel.reasoning, intel.solidity, current_y++});
                }
            }
            if (first_g_idx == -1) {
                items.push_back({"> No high value targets detected.", "No data.", 0, current_y++});
            }
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

        void drawSplash() {
            clear();
            setColor(11);
            std::cout << "\n\n";
            std::cout << "         *-----------------------------------------------------*\n";
            std::cout << "         |                 [ ALLIED FORCES ]                   |\n";
            std::cout << "         |                                                     |\n";
            std::cout << "         |   [ US ]   [ GBR ]       [ ANZAC ]   [ ROC ]        |\n";
            std::cout << "         |   * * *    +--+--+       *     *     +----+         |\n";
            std::cout << "         |   * * *    |  |  |         *         | () |         |\n";
            std::cout << "         |   * * *    +--+--+       *   * *     +----+         |\n";
            std::cout << "         |                                                     |\n";
            std::cout << "         |         WAR IN THE PACIFIC - C.I.C. SYSTEM          |\n";
            std::cout << "         *-----------------------------------------------------*\n";
            setColor(7);
            std::cout << "\n\n                     Press any key to initialize...";
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
            
            // Draw critical items...
            int hvt_y = 12;
            if (first_c_idx != -1) hvt_y = items[first_c_idx].y_pos + 1; // estimate
            for(int i=1; i < items.size(); ++i) {
                if (first_g_idx != -1 && i == first_g_idx) {
                    hvt_y = items[i].y_pos - 2;
                    break;
                }
            }
            
            gotoxy(0, hvt_y);
            setColor(14);
            std::cout << " [H]igh Value Target Tracker\n";
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n";

            for(int i=0; i < items.size(); ++i) {
                gotoxy(i==0 ? 70 : 2, items[i].y_pos);
                if(i == selected_item) {
                    setColor(240); // Black on White
                } else {
                    setColor(i==0 ? 12 : 7); // Red for exit, white for rest
                }
                std::cout << items[i].title;
                setColor(7); // reset
            }
        }

        void drawModalInfo() {
            auto& item = items[selected_item];
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
                        if (esc_count >= 3 && state == State::MAIN) {
                            state = State::MODAL_EXIT;
                            drawModalExit();
                            continue;
                        }
                    } else {
                        esc_count = 0;
                    }

                    if (state == State::SPLASH) {
                        state = State::MAIN;
                        drawMain();
                    } 
                    else if (state == State::MODAL_EXIT) {
                        if (ch == 'y' || ch == 'Y' || key == VK_RETURN) { running = false; }
                        else if (ch == 'n' || ch == 'N' || key == VK_ESCAPE) { state = State::MAIN; drawMain(); }
                    }
                    else if (state == State::MODAL_INFO) {
                        if (key == VK_RETURN || key == VK_ESCAPE) { state = State::MAIN; drawMain(); }
                    }
                    else if (state == State::MAIN) {
                        if (key == VK_UP) {
                            selected_item = (selected_item - 1 + items.size()) % items.size();
                            drawMain();
                        } else if (key == VK_DOWN || key == VK_TAB) {
                            selected_item = (selected_item + 1) % items.size();
                            drawMain();
                        } else if (ch == 'c' || ch == 'C') {
                            if (first_c_idx != -1) selected_item = first_c_idx; 
                            drawMain();
                        } else if (ch == 'h' || ch == 'H') {
                            if (first_g_idx != -1) selected_item = first_g_idx; 
                            drawMain();
                        } else if (ch == 'x' || ch == 'X') {
                            selected_item = 0; 
                            drawMain();
                        } else if (key == VK_RETURN) {
                            if (selected_item == 0) {
                                state = State::MODAL_EXIT;
                                drawModalExit();
                            } else {
                                if (items[selected_item].solidity > 0) {
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