#include "ui/Dashboard.hpp"
#include "engine/IntelligenceEngine.hpp"
#include "parsers/OpsParser.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
    parsers::OpsParser parser;
    engine::IntelligenceEngine engine;
    ui::Dashboard dashboard;

    parser.parse("data/samples/media_1788967272339.txt");
    engine.process();
    dashboard.render();

    return 0;
}