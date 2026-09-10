#include "Dashboard.hpp"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

namespace ui {

    enum class State { SPLASH, MAIN, MODAL_INFO, MODAL_EXIT };
    enum class MenuIndex { EXIT_BTN = 0, C_ITEM1, C_ITEM2, G_ITEM1, G_ITEM2, MAX };

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

        std::vector<MenuItem> items = {
            {"[X] EXIT", "", 0, 1},
            {"> INVASION WARNING: 4/20th Infantry Regiment loaded on xAKL -> Legaspi", "SIGINT REPORT FOR Dec 07, 41: 4/20th Infantry Regiment is loaded on a Std-E Cargo class xAKL moving to Legaspi.", 90, 12},
            {"> CONCENTRATION: 6 Japanese ships moving NW near Kalidjati", "OPERATIONAL REPORT FOR Dec 07, 41: C.XI-W sighting report: 6 Japanese ships at 51,99 near Kalidjati, speed 16, Moving Northwest.", 85, 13},
            {"> CV Akagi        | Last seen: 1 day ago  | Status: UNKNOWN", "SIGINT REPORT FOR Dec 07, 41: Heavy Volume of Radio transmissions detected at 90,96.", 40, 17},
            {"> TF 420          | Shadowed by Float Plane | Status: ACTIVE", "OPERATIONAL REPORT FOR Dec 07, 41: TF 420 shadowed by Japanese Float Plane at 52,82 near Mersing.", 95, 18}
        };

        void clear() {
            system("cls");
        }

        void setColor(int color) {
            SetConsoleTextAttribute(hOut, color);
        }

        void gotoxy(int x, int y) {
            COORD c;
            c.X = x;
            c.Y = y;
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
            std::cout << " [ ALLIED COMBAT INFORMATION CENTER ]                     Turn: Dec 08, 1941\n";
            setColor(8);
            std::cout << "===============================================================================\n";
            
            // Top data sources box
            setColor(3); // Cyan
            std::cout << " [ DATA SOURCES SYNCED ]\n";
            setColor(7);
            std::cout << " > Operations Reports : Last 60 Days\n";
            std::cout << " > Combat Reports     : Last 80 Days\n";
            std::cout << " > SIGINT             : Last 30 Days\n";
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n\n";

            // Critical alerts
            setColor(12); // Red
            std::cout << " [C]ritical alerts\n";
            setColor(8);
            std::cout << "-------------------------------------------------------------------------------\n";
            std::cout << "\n\n\n";
            
            // Ghost Fleet Tracker
            setColor(14); // Yellow
            std::cout << " [G]host Fleet Tracker (High Value Targets)\n";
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
            setColor(31); // White on Blue
            
            // Draw modal background
            for(int i=6; i<18; ++i) {
                gotoxy(10, i);
                std::cout << "                                                                "; // 64 spaces
            }
            
            gotoxy(12, 7);  std::cout << "INTEL ASSESSMENT";
            
            // Word wrap the reasoning text
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
                            selected_item = 1; 
                            drawMain();
                        } else if (ch == 'g' || ch == 'G') {
                            selected_item = 3; 
                            drawMain();
                        } else if (ch == 'x' || ch == 'X') {
                            selected_item = 0; 
                            drawMain();
                        } else if (key == VK_RETURN) {
                            if (selected_item == 0) {
                                state = State::MODAL_EXIT;
                                drawModalExit();
                            } else {
                                state = State::MODAL_INFO;
                                drawModalInfo();
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

    void Dashboard::render() {
        InteractiveTUI tui;
        tui.run();
    }
}