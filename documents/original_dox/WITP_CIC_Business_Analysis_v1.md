# BUSINESS ANALYSIS DOCUMENT
## Project: WITP:AE CIC Intelligence Helper — Version 1
Date: 1 September 2026 · Status: Approved for development · Target platform for this release is Windows... then with a linux platform that will help gaming done on wine or proton.

---

## 1. Executive Summary

This document describes a small desktop application for players of the computer wargame *War in the Pacific: Admiral's Edition* (WITP:AE), specifically for those playing by email (PBEM). The game already produces several plain-text reports at the end of every turn. The application reads those reports, analyses them over time, and presents the player with a "Combat Information Center" (CIC) style dashboard that surfaces intelligence a human reader would likely miss: enemy fleet concentrations, invasion warnings, merchant shipping routes, recurring enemy habits, and corrections to earlier battle claims.

The product is strictly read-only. It never touches the game itself, never makes decisions for the player, and never communicates with anyone. All data stays on the player's own computer. Version 1 ships for Mac first; the same codebase will produce the Windows version. 


## 2. Background and the Problem

WITP:AE is one of the most detailed Second World War Pacific simulations ever made. In PBEM play, two people (Allies and Japan) take turns by exchanging files, often over weeks or months.

Each turn, the game writes thousands of lines of reports per side: signals intelligence (radio intercepts), operational reports, after-action combat reports, and a turn event log. Players must read all of this manually and remember it from turn to turn. In practice this means:

- Important signals get buried in noise (a single turn can contain hundreds of report lines).
- Tracking an enemy task force across several turns is done by hand, if at all.
- Subtle contradictions between reports — for example a ship reported sunk in one place reappearing later — are easily missed.
- Each side only sees its own reports, and under the game's "fog of war" some of what a report says may be wrong in reality (a "sunk" ship may only be damaged; a ship may be misidentified).

There is no existing tool that performs this ongoing intelligence analysis for the player. The result is fatigue, mistakes, and a weaker experience than the game deserves.

## 3. Target Users

- Primary: PBEM players of WITP:AE, Allied or Japanese side. Typically experienced, patient wargamers who invest many hours per game.
- Secondary: solo players running hotseat games, and community members writing after-action reports who want a clean intelligence record.

The market is niche but deeply engaged, and the product has zero marginal cost per user (a free or low-price desktop utility).

## 4. The Product in Plain Terms

When opened, the application shows a short splash screen, then a setup wizard. The wizard asks which side the player commands, finds the folder where the game writes its reports (searching automatically and guiding the user with clear messages if anything is missing), and confirms the game's turn length.

The main screen is a dark, high-contrast dashboard modelled on a naval Combat Information Center. It has two main tabs:

**Tab 1 — Intelligence Summary.** A set of boards:
1. High-Value Targets — detected enemy carriers, capital ships, tankers/oilers and troop ships.
2. Strike Concentrations — groups of three or more warships (cruiser-sized or larger) detected within ten hexes of each other (the rule is adjustable).
3. Port Concentrations — warships or merchant ships gathered at named ports.
4. Established Routes — merchant and tanker corridors built up from repeated sightings, with inferred start and end points.
5. Corrections — changes to earlier battle claims (e.g., "reported sunk, now believed damaged").
Plus: invasion warnings, alerts that a force has sailed, predictions of recurring enemy behaviour, and a running count of enemy capital ships never yet accounted for (the "ghost fleet").

**Tab 2 — SIGINT Map.** The game's planning map with the analysed signals intelligence drawn on it, layer by layer (radio detections, sightings, concentration circles, routes). The player can move backwards and forwards through past turns.

Permanent side panels complete the picture: an alert feed on the left (new intelligence, prioritised), a dossier panel on the right (the player types the name of any base, ship, unit, air group or city and receives a compiled intelligence file on it, including strength estimates), and an event log along the bottom.

## 5. The Data the Product Uses

Four report families, all plain text, all already produced by the game:

1. Signals intelligence (SIGINT) — one file per side per turn.
2. Operational reports — one per side per turn.
3. Combat report — the same file for both players.
4. Combat events log — same file name for both players, but each side's copy contains its own content.

File names carry the side and the date (for example `asigint_411207.txt` is the Allied SIGINT file for 7 December 1941), and the application uses that information automatically.

## 6. How the Product Handles Truth and Fog of War

This is the heart of the design, and a key selling point. The application does not treat reports as gospel. It keeps a "belief model":

- Allied signals intelligence is treated as certain (a deliberate design decision reflecting the historical Allied code-breaking advantage; it is also configurable).
- Japanese signals intelligence is treated as probable, not certain.
- A side's operational reports are treated as fact for its own forces.
- The shared combat report is treated with healthy scepticism: claims of sinkings and damage are held as claims, gain or lose confidence as later turns corroborate them, and are corrected openly when contradicted.

Because each player's copy of the application only ever reads that player's own files, the two opponents' tools can — correctly — disagree about what happened. Example from real game data: both sides' tools see the claim that the destroyer *Maratha* was sunk; only the Allied tool sees the Allied admission confirming it; only the Japanese tool knows two of its own transports sank after being damaged in that same battle. Each player gets the truth as his side knows it.

## 7. Examples of the Intelligence the Product Delivers

All of the following come from the very first turn of a real game (7 December 1941), demonstrating immediate value:

- Invasion warnings for Legaspi and Vigan, built from certain Allied intercepts of Japanese units embarking on transports and planning attacks.
- A confirmed landing concentration at Kota Bharu, combining sightings, combat reports and named enemy ships.
- A major unseen force near hex 90,96 revealed by repeated heavy radio traffic — exactly the kind of signal a human reader skims past.
- A tanker identified near Midway, seeding a future refuelling-route track.
- A named merchant ship (*Ryuyo Maru*) whose repeated sightings will, over a few turns, become an established trade route the Allied player can interdict.

Over later turns the product adds: recurring-behaviour predictions ("enemy recon over this base every two turns; next expected turn 17"), course-change and radio-silence alerts, and corrections when earlier sinkings prove wrong.

## 8. User Control and Safety

Every judgement the product makes (radius for concentrations, minimum ship counts, scepticism toward battle claims, silence thresholds) has a sensible default and a plain-language setting the player can change, with a one-click reset. Parse problems never crash the program; unreadable lines are counted and shown, and empty report files are treated as a normal "nothing to report" occurrence.

## 9. Technology and Platform (Business View)

One codebase serves Mac and Windows, using Python with the Qt interface toolkit — mature, cross-platform, and well suited to data-heavy desktop tools. There is no server, no account system and no telemetry: the product is a private, offline utility, which simplifies support and removes privacy concerns entirely. Distribution for this occasion is a standard Mac disk image (.dmg); Windows installer follows with the same code.

Development itself is AI-assisted: a complete Software Design Document and a prepared prompt allow a coding agent (Nemotron 3 Ultra) to implement the product against fixed specifications and tests, keeping development cost low and quality checkable.

## 10. Version 1 Scope — What Is In and What Is Deliberately Out

In: everything described in sections 4–8, plus automated and manual testing using real game output as the reference material.

Out (deferred by decision, not forgotten): detailed surface-combat report parsing (Ver 1.1); global weather capture (the game shows weather on screen only; a future version may read it); placing ship icons on the map (analysis overlays only); any interaction with the game files; multiplayer or cloud features.

## 11. Quality Assurance

The six real report files from 7 December 1941 are kept as permanent reference ("golden") files. Every build must parse them exactly, reproduce the known intelligence (counts of reports, sinkings, warnings), and pass multi-turn synthetic scenarios that exercise route-building, habit prediction and corrections. A manual checklist covers both operating systems, the setup wizard's guidance paths, and the dashboard's behaviour. No build ships unless all automated tests pass and skipped-line rates stay below one percent.

## 12. Risks and Mitigations

- Game patches change report wording → parsers are defensive, log what they cannot read, and the reference files make regressions visible immediately.
- Fog-of-war misreading leads to wrong conclusions → the belief/confidence model and the corrections board make uncertainty visible instead of hiding it.
- Scope creep → the out-of-scope list is frozen for Ver 1.
- Niche market → minimal overhead (no servers, no licences beyond free tooling), community-driven distribution.

## 13. Value Proposition

The product converts hours of manual bookkeeping into seconds of reading, catches the subtle discrepancies human readers miss, and adds a layer of professional-feeling intelligence analysis that deepens immersion. Nothing comparable exists for this game; the closest tools are map viewers and spreadsheets that perform no analysis at all.

## 14. Roadmap Sketch

Ver 1 (this document) → Ver 1.1 surface-combat detail → Ver 2: global weather capture, richer map interaction, after-action-report export, and an optional local question-and-answer assistant grounded in the game manual.

## 15. Glossary

- PBEM: play-by-email, two players exchanging turn files.
- Hex: one cell of the game's map grid.
- SIGINT: signals intelligence, i.e., radio intercepts.
- Task Force (TF): a group of ships the player organises.
- Fog of war: the game's deliberate uncertainty about what the enemy is doing.
- BDA: battle damage assessment — judgements about hits, damage and sinkings.
- CIC: Combat Information Center, a warship's intelligence room; the model for this product's dashboard.

---
Companion documents: Software Design Document (`WITP_CIC_SDD_v1.md`), coding-agent prompt (`agent_prompt.txt`), reference game files (turn of 7 December 1941).