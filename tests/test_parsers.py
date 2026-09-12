import pytest
from src.witp_ae_helper.parsers.sigint import parse_sigint
from src.witp_ae_helper.parsers.ops import parse_ops
from src.witp_ae_helper.parsers.combat import parse_combat
from src.witp_ae_helper.parsers.events import parse_events
from src.witp_ae_helper.config import DEFAULT_CONFIG

def test_sigint_empty_file():
    obs, places = parse_sigint([], "Allied", DEFAULT_CONFIG)
    assert len(obs) == 0
    assert len(places) == 0

def test_sigint_loc_parsing():
    # Test specific location formats mentioned in SDD
    lines = [
        "Heavy Volume of Radio transmissions detected at Kagi(85,64).",
        "Radio transmissions detected at Matsue (107,57)."
    ]
    obs, places = parse_sigint(lines, "Allied", DEFAULT_CONFIG)
    assert len(obs) == 2
    assert obs[0]["event_type"] == "HEAVY_RADIO"
    assert obs[0]["location_name"] == "Kagi"
    assert obs[0]["hex_x"] == 85
    assert obs[0]["hex_y"] == 64
    
    assert obs[1]["event_type"] == "RADIO"
    assert obs[1]["location_name"] == "Matsue"
    assert obs[1]["hex_x"] == 107
    assert obs[1]["hex_y"] == 57

def test_ops_tfcont():
    lines = [
        "CL Naka detected by PBY-4 Catalina at 83,65 near Takao"
    ]
    obs, places = parse_ops(lines, "Allied", DEFAULT_CONFIG)
    assert len(obs) == 1
    assert obs[0]["event_type"] == "ENEMY_PRESENCE"
    assert obs[0]["hex_x"] == 83
    assert obs[0]["hex_y"] == 65
    assert obs[0]["location_name"] == "Takao"

def test_ops_mine():
    lines = [
        "CM Okinoshima lays minefield at Batan Island ( 50 , 84 ) and sets course for Takao"
    ]
    obs, places = parse_ops(lines, "Allied", DEFAULT_CONFIG)
    assert len(obs) == 1
    assert obs[0]["event_type"] == "MINE"
    assert obs[0]["ship_type"] == "CM"
    assert obs[0]["ship_name"] == "Okinoshima"
    assert obs[0]["location_name"] == "Batan Island"
    assert obs[0]["hex_x"] == 50
    assert obs[0]["hex_y"] == 84

def test_combat_damage():
    lines = [
        "Japanese Ships",
        "BB Nevada, Bomb hits 4, Torpedo hits 4,  heavy fires,  heavy damage",
        "xAK Asakasan Maru sinks...."
    ]
    # We parse this using combat parser. Wait, the "sinks...." is from events, but let's test damage tail.
    obs, blocks, places = parse_combat(lines, "Allied", DEFAULT_CONFIG)
    assert len(obs) >= 1
    assert obs[0]["event_type"] == "SHIP_DAMAGE"
    assert obs[0]["ship_type"] == "BB"
    assert obs[0]["ship_name"] == "Nevada"
    assert "Bomb hits 4" in obs[0]["damage_tail"]

def test_events_rescue_and_partisan():
    lines = [
        "TF 85 rescuing 192 men from 12th Engineer Regiment",
        "Low garrison leads to damage and 1 VP loss from partisan attack at Agra (50, 19)!"
    ]
    obs, places = parse_events(lines, "Allied", DEFAULT_CONFIG)
    assert len(obs) == 2
    assert obs[0]["event_type"] == "RESCUE"
    assert obs[0]["men_count"] == 192
    assert obs[0]["unit_name"] == "12th Engineer Regiment"
    
    assert obs[1]["event_type"] == "PARTISAN"
    assert obs[1]["location_name"] == "Agra"
    assert obs[1]["hex_x"] == 50
    assert obs[1]["hex_y"] == 19
