import os

path = 'documents/design/WITP_CIC_SDD_v1.md'
with open(path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_lines = []
skip_mode = False
for line in lines:
    # Remove BDA Corrections from Enums
    if 'BDA_CONFLICT' in line or 'CORRECTION' in line:
        line = line.replace('BDA_CONFLICT|', '')
        line = line.replace('CORRECTION|', '')
        line = line.replace('CORRECTION.', '')
        line = line.replace('re-sighting after sunk → DAMAGED + BDA_CONFLICT (CRIT) + CORRECTION;', '')
        
    if 'PySide6 (Qt)' in line:
        line = line.replace('PySide6 (Qt)', 'Terminal/Console UI')
        
    if '## S9. UI' in line:
        line = '## S9. UI (Text-Based Terminal)\n'
        
    if 'Tab1 "Intel Summary"' in line:
        line = '- **Intel Summary**: Text dashboard [HVT | Concentrations | Ports | Routes] with 0-100% `Solidity` meters.\n- **Detail Pop-ups**: Pressing Enter on any intel line opens a pop-up summarizing the stats/reports that led to this intel.\n'
        
    if 'Right DossierPanel' in line:
        line = '- **Unit Finder**: Autocomplete dropdown of all identified land/air/naval units. Returns last position, status, and 0-100% solidity.\n'

    if 'pip install pyside6' in line:
        line = line.replace('pip install pyside6', 'pip install textual')
        
    new_lines.append(line)

with open(path, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)
