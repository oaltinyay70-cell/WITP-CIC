cmake_minimum_required(VERSION 3.14)
project(WITP_CIC_Helper VERSION 0.01 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch FTXUI
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)

# Add Executable
add_executable(witp_cic 
  src/main.cpp 
  src/ui/Dashboard.cpp 
  src/engine/IntelligenceEngine.cpp
  src/storage/CampaignVault.cpp
  src/ai/StaffOfficer.cpp
  src/parsers/PWSParser.cpp
)

# Link Libraries
target_link_libraries(witp_cic
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)