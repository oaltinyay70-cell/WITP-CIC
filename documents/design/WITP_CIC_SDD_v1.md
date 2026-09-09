# WITP:AE CIC HELPER — SOFTWARE DESIGN DOCUMENT (SDD)
**Version:** 1.0 (final, consolidated) · **Target platform:** macOS · **Stack:** Python 3.11+, Terminal/Console UI, SQLite

This document is complete and self-contained. An implementation agent must follow it literally: use the regexes, SQL, defaults, and algorithms exactly as written. "Log raw" = store the line in `observations` with `event_type='RAW'`, never raise.

---

## S1. Product Definition
Read-only desktop intelligence assistant for PBEM players of *War in the Pacific: Admiral's Edition*. Ingests one side's end-of-turn text exports, mines them, and presents a CIC dashboard: high-value targets, concentrations, routes, habits, corrections, invasion warnings, and a queryable dossier. Never writes to game files.
**Principle 0 — beliefs, not facts:** the DB models what this player's side believes; every record carries a confidence.

## S2. Sources & Truth Model
| Source | Filename pattern | Visibility | Confidence |
|---|---|---|---|
| SIGINT | `(a|j)sigint_(\d{6})\.txt` | own side | `sigint_certainty[side]` (Allied 1.0, Japanese 0.6) |
| OPS | `(a|j)operationsreport_(\d{6})\.txt` | own side | 1.0 |
| COMBAT | `combatreport_(\d{6})\.txt` | same bytes both sides | skeptical (`bda_skepticism` 0.6) |
| EVENTS | `Combat_Events_(\d{6})\.txt` | own content per side | own-side 1.0; enemy-side 0.8 |

Filename digits = YYMMDD; cross-check header date, warn on mismatch, do not fail. Same-named files use `config.player_side`.

## S3. Project Structure
```
witp_cic/
  main.py  config.py  db.py  hexmath.py
  parsers/ common.py sigint.py ops.py combat.py events.py
  engine/ belief.py tails.py concentrations.py routes.py habits.py patterns.py corrections.py dossier.py
  ui/ splash.py wizard.py mainwindow.py alert_feed.py boards.py map_view.py dossier_panel.py event_log.py
  assets/map.jpg
  tests/ fixtures/ synthetic/ test_parsers.py test_engine.py test_integration.py
```

## S4. config.json — exact defaults
```json
{
 "player_side": "Allied", "export_dir": "", "db_path": "~/.witp_cic/cic.db",
 "turn_duration_hours": 24, "hex_nm": 40,
 "sigint_certainty": {"Allied": 1.0, "Japanese": 0.6},
 "fuzzy_radius": 3, "max_speed_knots": 35, "radio_silence_turns": 2,
 "course_change_deg": 45, "bda_skepticism": 0.6,
 "conc_radius": 10, "conc_min_warships": 3,
 "route_cell": 5, "route_min_traversals": 3,
 "habit_min_samples": 3, "habit_cv_max": 0.25,
 "hvt_classes": ["CV","CVL","BB","BC","AO","AOG","TK","AP","xAP"],
 "warship_classes": ["CV","CVL","BB","BC","CA","CL","DD"],
 "merchant_prefix": ["xAK","xAKL","AK","AP","AO","TK","AMc"],
 "auto_process": true, "density": "M", "map_cal": null
}
```

## S5. Database — exact DDL
```sql
CREATE TABLE game_turns(turn_id INTEGER PRIMARY KEY, game_date TEXT, side TEXT, processed_at TEXT);
CREATE TABLE place_names(place TEXT PRIMARY KEY, hex_x INTEGER, hex_y INTEGER, first_seen INTEGER);
CREATE TABLE observations(
  obs_id INTEGER PRIMARY KEY AUTOINCREMENT, turn_id INTEGER, block_id INTEGER,
  source TEXT, source_file TEXT, event_type TEXT, observer_side TEXT, subject_side TEXT,
  unit_name TEXT, ship_name TEXT, ship_type TEXT, class_desc TEXT, men_count INTEGER,
  ac_type TEXT, ac_count INTEGER, location_name TEXT, hex_x INTEGER, hex_y INTEGER,
  target_name TEXT, heading INTEGER, speed INTEGER, ship_count INTEGER,
  status_reported TEXT, damage_tail TEXT, pulse TEXT, confidence REAL,
  entity_id INTEGER, raw_line TEXT);
CREATE TABLE entities(
  entity_id INTEGER PRIMARY KEY AUTOINCREMENT, kind TEXT, name TEXT, class TEXT, side TEXT,
  belief_status TEXT, confidence REAL, first_seen INTEGER, last_obs INTEGER, alt_names TEXT);
CREATE TABLE tails(tail_id INTEGER PRIMARY KEY AUTOINCREMENT, entity_id INTEGER,
  start_turn INTEGER, end_turn INTEGER, status TEXT,
  avg_heading REAL, avg_speed REAL, proj_x INTEGER, proj_y INTEGER);
CREATE TABLE combat_blocks(block_id INTEGER PRIMARY KEY AUTOINCREMENT, turn_id INTEGER,
  block_type TEXT, target TEXT, place2 TEXT, hex_x INTEGER, hex_y INTEGER, weather TEXT);
CREATE TABLE concentrations(conc_id INTEGER PRIMARY KEY AUTOINCREMENT, turn_id INTEGER,
  kind TEXT, cx INTEGER, cy INTEGER, ship_est INTEGER, warship_count INTEGER,
  emitters INTEGER, trend TEXT, place_hint TEXT, certainty REAL);
CREATE TABLE route_segments(seg_id INTEGER PRIMARY KEY AUTOINCREMENT,
  cell_from TEXT, cell_to TEXT, traversals INTEGER, last_turn INTEGER,
  kind TEXT, confidence REAL, start_place TEXT, end_place TEXT);
CREATE TABLE habits(habit_id INTEGER PRIMARY KEY AUTOINCREMENT, subject TEXT,
  event_type TEXT, place TEXT, mean_interval REAL, cv REAL, samples INTEGER,
  last_turn INTEGER, next_predicted INTEGER);
CREATE TABLE alerts(alert_id INTEGER PRIMARY KEY AUTOINCREMENT, turn_id INTEGER,
  alert_type TEXT, severity TEXT, message TEXT, entity_id INTEGER, is_read INTEGER DEFAULT 0);
CREATE INDEX ix_obs_spatial ON observations(turn_id, hex_x, hex_y);
CREATE INDEX ix_obs_entity ON observations(entity_id);
```
Enums — `belief_status`: OPERATIONAL|DAMAGED|SUNK_UNCONFIRMED|SUNK_PROBABLE|LOST_CONTACT. `alert_type`: MOVING_TAIL|COURSE_CHANGE|RADIO_SILENCE|UNCONFIRMED_KILL|IDENT_MISMATCH|CONCENTRATION|PORT_CONCENTRATION|CONVERGENCE|INVASION_WARNING|SORTIE|ROUTE_ASCERTAINED|HABIT_PREDICTION|UNIT_COMMITTED|REPAIR_STALLED| `severity`: INFO|WARN|CRIT.

## S6. hexmath.py — exact code
```python
import math
def to_cube(x, y): return (x - (y - (y & 1)) // 2, y)      # odd-r offset
def hex_distance(a, b):
    ax, az = to_cube(*a); bx, bz = to_cube(*b)
    ay, by = -ax - az, -bx - bz
    return max(abs(ax-bx), abs(ay-by), abs(az-bz))
def hex_to_xy(x, y): return (x + 0.5 * (y & 1), y * 0.8660254)
def bearing(a, b):
    (x1, y1), (x2, y2) = hex_to_xy(*a), hex_to_xy(*b)
    return (math.degrees(math.atan2(x2-x1, -(y2-y1)))) % 360.0
def speed_knots(a, b, turns, cfg):
    return hex_distance(a, b) * cfg["hex_nm"] / (turns * cfg["turn_duration_hours"])
def cell(x, y, size): return f"{x//size},{y//size}"
```

## S7. Parsers
All: UTF-8, strip `\r`, skip blanks; unknown → RAW; store `raw_line`; upsert `place_names` on named+hex.

### S7.1 common.py
```python
LOC = (r"(?:\s*(?P<name>[A-Za-z][A-Za-z\s\.\'\-\/]*?)\s*\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)"
       r"|\s+(?P<bx>\d+)\s*,\s*(?P<by>\d+)\s*)")
def loc(m):
    if m.group("x") is not None: return m.group("name").strip(), int(m.group("x")), int(m.group("y"))
    return None, int(m.group("bx")), int(m.group("by"))
```

### S7.2 sigint.py
```python
HEADER  = r"^SIG INT REPORT FOR ([A-Z][a-z]{2} \d{2}, \d{2})$"
RADIO   = rf"^Radio transmissions detected at{LOC}\.$"
HEAVY   = rf"^Heavy Volume of Radio transmissions detected at{LOC}\.$"
UNITLOC = rf"^(?P<unit>.+?)\s+is located at{LOC}\.$"
MENBASE = rf"^(?P<men>\d+) men are based at{LOC}\.$"
PLAN    = r"^(?P<unit>.+?)\s+is planning for an attack on\s+(?P<target>.+?)\.$"
EMBARK  = r"^(?P<unit>.+?)\s+is loaded on a (?P<nat>Japanese|Allied)\s+(?P<stype>[A-Za-z]+)\s+moving to\s+(?P<dest>.+?)\.$"
CALLSIGN= r"^Radio call sign of\s+(?P<stype>[A-Za-z]+)\s+(?P<ship>.+?)\s+detected at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)\.$"
```
Event types: RADIO, HEAVY_RADIO, UNIT_LOCATED, MEN_BASED, UNIT_PLANNING, UNIT_EMBARKED, SHIP_CALLSIGN. Confidence = `sigint_certainty[observer_side]`.

### S7.3 ops.py
```python
HEADER  = r"^OPERATIONAL REPORT FOR (.+)$"
TFCONT  = r"^(?:TF\s+(?P<tf>\d+)|(?P<st>[A-Za-z]+)\s+(?P<sn>.+?))\s+(?P<verb>followed by|shadowed by|snooped by|detected by|sighted by|observes|sights|detects)\s+(?P<obs>.+?)\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)\s+near\s+(?P<place>.+)$"
SIGHT   = r"^(?:(?P<ac>[\w\-\.\d ]+?)\s+)?[Ss]ighting report:\s*(?P<n>\d+)\s+(?P<nat>Japanese|Allied)\s+ships?\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)\s+near\s+(?P<place>[^,]+?)\s*(?:,\s*speed\s+(?P<spd>\d+))?\s*(?:,\s*Moving\s+(?P<dir>[A-Za-z]+))?\s*$"
CLASSID = r"^(?P<ac>[A-Za-z0-9\-\.\d]+(?: [A-Za-z0-9\-\.\d]+)?)(?:\s+from\s+(?P<unit>.+?))?\s+has spotted an?\s+(?P<cls>.+?)\s+at\s+\(?\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)?\s*$"
SHADOW  = r"^(?P<ac>[A-Za-z0-9\-\.\d]+(?: [A-Za-z0-9\-\.\d]+)?)\s+reports (?P<what>object near surface|shape below surface|submarine wake|object under water|shadow in water|suspected submarine|periscope wake|periscope|conning tower)\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)\s+near\s+(?P<place>.+)$"
AIROVER = r"^(?P<ac>.+?)\s+sighted over\s+(?P<place>.+)$"
OWNSINK = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+sinks\s+(?P<where>in port|at sea)$"
ABANDON = r"^Damage Control efforts fail and\s+(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+is abandoned$"
ADMIT   = r"^Loss of\s+(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+on\s+(?P<date>.+?)\s+is admitted$"
ESUNK   = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+is reported to have been sunk near\s+(?P<place>.+?)\s+on\s+(?P<date>.+)$"
OWNHIT  = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+is reported HIT$"
EHIT    = r"^an?\s+(?P<subj>.+?)\s+is reported HIT$"
CAPTURE = r"^(?P<nat>Japanese|Allied)\s+forces CAPTURE\s+(?P<place>.+?)\s+!!!$"
MINE    = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+lays minefield (?:at|near)\s+(?P<place>.+?)\s*\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)\s+and sets course for\s+(?P<dest>.+)$"
ARRIVE  = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+arrives at\s+(?P<place>.+)$"
REFIT   = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+(?:beginning refit in shipyard at|taken out of commission to begin refit at)\s+(?P<place>.+)$"
STALL   = r"^No additional repairs possible on\s+(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+using currently assigned resources at\s+(?P<place>.+)$"
FORT    = r"^(?P<place>.+?)\s+expands fortifications to size\s+(?P<n>\d+)$"
REPL    = r"^(?P<n>\d+)\s+(?P<ac>.+?)\s+replacements arrive at\s+(?P<place>.+)$"
CWATCH  = r"^Coastwatcher sighting:\s*(?P<n>\d+)\s+(?P<nat>Japanese|Allied)\s+ships?\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)\s+near\s+(?P<place>.+)$"
```
`TFCONT` → event ENEMY_PRESENCE at hex. `SIGHT/CLASSID/SHADOW/CWATCH` confidence 1.0. `ESUNK/EHIT` confidence `bda_skepticism`. `STALL` → alert REPAIR_STALLED (WARN). Kill credits, upgrades, transfers, aircraft-writeoff lines → RAW.

### S7.4 combat.py — state machine
Headers (create `combat_blocks`, reset mode):
```python
SEP   = r"^-{10,}$"
MIDGET= r"^Midget Sub attack inside harbor of\s+(?P<place>.+?)!!!$"
ASW   = r"^ASW attack near\s+(?P<place>.+?)\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)$"
AIR   = r"^(?P<tod>Morning|Afternoon|Evening|Night)\s+Air attack on\s+(?P<target>.+?)\s*,?\s*at\s+\(?(?P<x>\d+)\s*,\s*(?P<y>\d+)\)?(?:\s*\((?P<place2>.+?)\))?$"
BOMB  = r"^Naval bombardment of\s+(?P<place>.+?)\s+at\s+(?P<x>\d+)\s*,\s*(?P<y>\d+)$"
PREINV= r"^Pre-Invasion action off\s+(?P<place>.+?)\s+\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)$"
AMPH  = r"^Amphibious Assault at\s+(?P<place>.+?)\s+\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)$"
GROUND= r"^Ground combat at\s+(?P<place>.+?)\s+\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)$"
```
Mode switches: `Japanese Ships|Allied Ships` → ship:<side>; `*aircraft` → aircount; `*aircraft losses` → airloss; `*ground losses:` → ground; `Aircraft Attacking:|CAP engaged:|Combat modifiers|Assaulting units:` → skip.
```python
WX    = r"^Weather in hex:\s*(?P<wx>.+)$"
SHIP  = r"^(?P<st>[A-Za-z]+)\s+(?P<name>[^,]+?)(?:,\s*(?P<tail>.+))?$"
AIRC  = r"^(?P<ac>.+?)\s+x\s+(?P<n>\d+)$"
AIRL  = r"^(?P<ac>.+?):\s+(?P<detail>.+)$"                      # RAW
FAC   = r"^(?P<what>Airbase|Airbase supply|Runway|Port|Port fuel|Repair Shipyard) hits\s+(?P<n>\d+)$"
SMOKE = r"^Heavy smoke from fires obscuring\s+(?P<st>[A-Za-z]+)\s+(?P<name>.+)$"
UNLOAD= r"^TF\s+(?P<tf>\d+)\s+troops unloading over beach at\s+(?P<place>.+?)\s*,\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)$"
FIRE  = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+firing at\s+(?P<target>.+)$"
ATK   = r"^Attacking force\s+(?P<t>\d+) troops,\s+(?P<g>\d+) guns,\s+(?P<v>\d+) vehicles, Assault Value =\s+(?P<av>\d+)$"
ODDS  = r"^(?P<nat>Japanese|Allied)\s+assault odds:\s+(?P<a>\d+)\s+to\s+(?P<b>\d+)\s+\(fort level\s+(?P<f>\d+)\)$"
```
SHIP tail tokens: `Bomb hits (\d+)`, `Torpedo hits (\d+)`, `on fire|heavy fires|heavy damage`; sunk = tail ends with "and is sunk". Ship rows → `SHIP_DAMAGE`/`SHIP_PRESENT` with block_id. SMOKE ⇒ those ships' claims confidence ×0.8. Confidence = `bda_skepticism`, except subject_side == observer_side ⇒ 1.0.

### S7.5 events.py
```python
HEADER = r"^COMBAT EVENTS FOR (\d{2}/\d{2}/\d{2})$"
PHASE  = r"^AIR OPERATIONS PHASE\s*:\s*(?P<pulse>AM|PM)$"
SINK   = r"^(?P<st>[A-Za-z]+)\s+(?P<name>.+?)\s+sinks\.\.\.$"
RESCUE = r"^TF\s+(?P<tf>\d+)\s+rescuing\s+(?P<n>\d+)\s+men from\s+(?P<unit>.+)$"
PART   = r"^Low garrison leads to damage and\s+(?P<vp>\d+)\s+VP loss from partisan attack at\s+(?P<place>.+?)\s*\(\s*(?P<x>\d+)\s*,\s*(?P<y>\d+)\s*\)!$"
AMPHIB = r"^Amphibious TF\s+(?P<tf>\d+)\s+offshore of\s+(?P<place>.+)$"
BOMBING= r"^(?P<nat>Japanese|Allied)\s+Ships Bombarding\s+(?P<place>.+)$"
KILLED = r"^(?P<rank>[A-Z]+)\s+(?P<name>.+?)\s+has been KILLED$"
HITM   = r"^::::::::\s+an?\s+(?P<subj>.+?)\s+is reported HIT$"
```
Reuse ops SIGHT/CLASSID/SHADOW/ARRIVE/FORT/REPL. Other all-caps lines ignored. SINK: 1.0 if own OOB else 0.8. Attach current pulse to all observations.

## S8. Engine (run order: tails → belief/corrections → concentrations → routes → habits → patterns)
**S8.1 tails:** contacts = RADIO, HEAVY_RADIO, SIGHTING, CLASSID, SHIP_CALLSIGN, SHADOW, ENEMY_PRESENCE, enemy SHIP_DAMAGE. Link new contact to candidate at T-1/T-2 with `hex_distance <= fuzzy_radius` and `speed_knots <= max_speed_knots` (min distance; tie: heading continuity); else new TAIL entity. Recompute vector over last ≤5 pts; store proj hex. Alerts: MOVING_TAIL (first ≥3 pts, INFO), COURSE_CHANGE (|Δbearing| > course_change_deg, WARN), RADIO_SILENCE (no link for radio_silence_turns → LOST_CONTACT, WARN).
**S8.2 belief/corrections:** sunk claim → SUNK_UNCONFIRMED conf bda_skepticism (1.0 own fact); +0.05/quiet turn cap 0.90; ≥0.75 → SUNK_PROBABLE;  own ADMIT/OWNSINK/SINK → 1.0; enemy SINK ledger → 0.8; class conflict same hex±2 same turn → alt_names + IDENT_MISMATCH (WARN) if warship class.
**S8.3 concentrations:** warship points per turn = enemy named warships (class in warship_classes) at block hex + warship CLASSID + one point per SIGHTING. Greedy cluster within conc_radius; warship_count ≥ conc_min_warships → kind STRIKE; trend vs previous cluster (growing/steady/dispersing); alert CONCENTRATION (WARN). Port: contacts at place_names hex, ≥3 ships → PORT + PORT_CONCENTRATION (INFO).
**S8.4 routes:** merchant entities (ship_type startswith merchant_prefix, or cls contains AO class/AK/AP/AMc). Consecutive positions → segment cell→cell, traversals+=1. ≥ route_min_traversals → alert ROUTE_ASCERTAINED (INFO); snap start_place/end_place = nearest place_names within 3 hexes.
**S8.5 habits:** group by (subject_key, event_type); if samples ≥ habit_min_samples: mean interval, cv=std/mean; cv ≤ habit_cv_max → upsert habit, next_predicted = last + round(mean), alert HABIT_PREDICTION (INFO).
**S8.6 patterns:** INVASION_WARNING (CRIT, once per target) from UNIT_PLANNING/UNIT_EMBARKED; CONVERGENCE (WARN) ≥2 tails with proj within conc_radius; SORTIE (WARN) HEAVY_RADIO at H then silence + sighting within 12 hexes consistent bearing ±30°; UNIT_COMMITTED (INFO) unit from intent intel appears in AMPHIBIOUS/GROUND/UNLOAD.
**S8.7 dossier:** resolve query (exact/prefix over place_names/entities/own ships, else difflib ≥0.6). Sections: identity (alt_names, class, side); status (belief_status, confidence, last_obs); journal (observations by turn, tagged `[Tn][SOURCE]`); derived per type (place: garrison = latest MEN_BASED sum + UNIT_LOCATED list, raid count, facility hits, fort events, concentrations, habits, routes; ship: damage history, routes, BDA conflicts; unit: locations, embarkation, planning, commitments, casualties). Plain-text renderer + Export/Copy.

## S9. UI (Text-Based Terminal)
- **Intel Summary**: Text dashboard [HVT | Concentrations | Ports | Routes] with 0-100% `Solidity` meters.
- **Detail Pop-ups**: Pressing Enter on any intel line opens a pop-up summarizing the stats/reports that led to this intel.

## S10. Settings dialog
Tabs: Game I/O; Intelligence Engine (all engine keys, tooltips, Reset to Defaults); Parsing Rules (fixed Auto); Alerts & UI (density, sound).

## S11. Tests — exact assertions
Fixtures: six real 411207 files. Synthetic t2/t3: move Ryuyo Maru +3 hexes/turn; re-sight DM Tracy t2; repeat AIROVER Singapore t1–t3; correction line.
- parsers: asigint = 20 non-RAW; jsigint = 5; empty file → 0 obs 0 errors; `Kagi(85,64)` & `Matsue (107,57)` parse; HEAVY 90,96 rows = 3; TFCONT parses `CL Naka detected by PBY-4 Catalina at 83,65 near Takao`; MINE parses `( 50 , 84 )`; combat blocks == 41, `and is sunk` == 3, SMOKE == 3 with discount, UNLOAD TFs {85,90,96}; events SINK == 13, RESCUE (192, 12th Engineer Regiment), PARTISAN == 6 incl `(50, 19)`.
- engine: Allied SIGINT conf 1.0 / Japanese 0.6; Tracy Allied 1.0 vs Japanese SUNK_UNCONFIRMED; Ryuyo tail links t1–t3; 200-hex jump rejected; concentrations T1 Allied ≥2; invasion warnings == {Legaspi, Vigan}; HVT contains CVL Ryujo + Type-1 TL AO; route ascertained t3; habit prediction t3, "warming up" t1.
- integration: no dup on re-run; Japanese DB has no asigint rows; dossier("Kota Bharu") contains "TF 85", "Sendai", "56th Recon".
- ui (offscreen): MainWindow builds; HVT rowCount ≥ 2; tabs == 2.

## S12. Verification checklist
pytest green on macOS; Tab1 populated from fixtures; zero unhandled exceptions; skipped ≤1%; .dmg builds and launches.

## Appendix A — sample raw lines
`Heavy Volume of Radio transmissions detected at 90,96.` · `Radio call sign of xAK Ryuyo Maru detected at 70,103.` · `TF 85 troops unloading over beach at Kota Bharu, 51,75` · `BB Nevada, Bomb hits 4, Torpedo hits 4,  heavy fires,  heavy damage` · `xAK Asakasan Maru sinks....` · `TF 85 rescuing 192 men from 12th Engineer Regiment` · `Low garrison leads to damage and 1 VP loss from partisan attack at Agra (50, 19)!`