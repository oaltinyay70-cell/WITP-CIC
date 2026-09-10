#include "ui/Dashboard.hpp"
#include "engine/IntelligenceEngine.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
    engine::IntelligenceEngine engine;
    
    // Parse all text files in data/samples
    engine.processDirectory("data/samples");
    
    ui::Dashboard dashboard;
    dashboard.render(engine);

    return 0;
}