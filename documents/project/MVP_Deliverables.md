# WITP:AE CIC Helper - MVP Deliverables

This document defines the Minimum Viable Product (MVP) deliverables for the WITP:AE CIC Intelligence Helper. The product is designed to process up to 4 years of daily reports, turning historical logs into predictive, actionable intelligence.

## 1. Architecture & Technology Stack
*   **Core Application Stack**: C/C++ for maximum performance, handling parsing, the intelligence engine, and the terminal UI.
*   **RAG Backend Stack**: Python is retained *exclusively* for the RAG ingestion pipeline (ChromaDB and sentence-transformers).
*   **Side-Specific Executables**: Separate, compiled native C++ binaries for each side (e.g., `WITP_CIC_Allied.exe` and `WITP_CIC_Japanese.exe`) to strictly enforce Fog of War.
*   **Setup Wizard**: Automated discovery of the WITP:AE export directory and game configuration.

## 2. Parsing & Ingestion Engine (C++)
*   **Multi-format Parsers**: High-performance C++ regex-driven parsers for daily SIGINT, Operational Reports, Combat Reports, and Combat Events.
*   **Defensive Processing**: A fail-safe mechanism that categorizes unreadable lines as `RAW` events rather than crashing, ensuring 99%+ uptime.

## 3. Predictive Intelligence Engine (C++)
*   **Tactical Early Warnings**: 
    *   *Amphibious Assaults*: Correlates SIGINT embarkation intercepts with Ops tracking of transport TFs.
    *   *Air Strikes*: Flags impending bombardments by detecting precursor enemy recon flights or approaching Carrier Task Forces.
    *   *Surface & Submarine Threats*: Tracks high-speed warship vectors and maps ambush zones.
    *   *Ground Offensives*: Cross-references SIGINT attack planning with frontline disruption.
*   **"Ghost Fleet" & HVT Tracking**: A persistent tracker for High-Value Targets logging "days since last seen" and projecting search areas.
*   **Trade Route Ascertainment**: Aggregates years of merchant/tanker sightings to map established enemy shipping corridors.
*   **Predictive Habit Profiling**: Statistically analyzes repetitive enemy actions to trigger alerts *before* the next occurrence.
*   *Note: BDA Corrections have been explicitly removed from the MVP scope.*

## 4. CIC Dashboard (C++ Text-Based UI)
*   **Terminal Interface**: A strictly text-based console UI built in C/C++, ensuring lightweight performance and universal compatibility.
*   **Solidity Meters**: Every piece of suggested intelligence features a 0-100% "Solidity" meter, visualizing the exact confidence level of the intel.
*   **Summary Pop-ups**: Pressing `Enter` on any intelligence line opens a pop-up displaying a summary of the stats and reports that led to that specific conclusion.
*   **Interactive Unit/Ship Finder**: A searchable dropdown list containing all identified land, air, and naval units in the game. Querying a unit instantly returns its last known position and operational status.
*   **Prioritized Alert Feed**: A section notifying the player of critical developments and tactical warnings.

## 5. Quality Assurance & Testing Suite
*   **Golden Reference Tests**: Automated test suite executing against historical reference files.
*   **Synthetic Scenarios**: Pre-scripted edge cases testing habit-prediction, route-ascertainment, and early-warning algorithms.
