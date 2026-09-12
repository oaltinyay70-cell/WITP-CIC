export const mockAlerts = [
  {
    id: 1,
    severity: 'critical',
    title: 'HVT-RED: AA-03 VODPAD',
    time: '12:48:21Z',
    subtext: 'TRACK 1142 | 12:48:21Z',
  },
  {
    id: 2,
    severity: 'high',
    title: 'STRIKE CONC-AMBER: FORMATION 224',
    time: '12:47:05Z',
    subtext: 'AREA 3A | 12:47:05Z',
  },
  {
    id: 3,
    severity: 'high',
    title: 'PORT CONC-AMBER: GORSHKOV GROUP',
    time: '12:45:10Z',
    subtext: 'PORT C | 12:45:10Z',
  },
  {
    id: 4,
    severity: 'high',
    title: 'ESTAB ROUTE-AMBER: RT 15 ALT. | SEC 4',
    time: '12:40:55Z',
    subtext: '12:40:55Z',
  },
  {
    id: 5,
    severity: 'info',
    title: 'CORRECTIONS-BLUE: TRACK 1109 | ID UPDATE',
    time: '12:35:12Z',
    subtext: 'ID UPDATE | 12:35:12Z',
  },
];

export const mockHVTs = [
  { id: 1142, track: 'TRACK 1142', name: 'AA-03 VODPAD', value: 11, color: 'critical' },
  { id: 1143, track: 'TRACK 1142', name: 'AA-03 VODPAD', value: 22, color: 'high' },
  { id: 1109, track: 'TRACK 1109', name: 'GORSHKOV GROUP', value: 27, color: 'high' },
];

export const mockPorts = [
  { id: 'A', name: 'Port A', vessels: 6, badge: 3, active: false },
  { id: 'B', name: 'Port B', vessels: 2, badge: 2, active: false },
  { id: 'C', name: 'Port C', vessels: 5, badge: 3, active: true },
];

export const mockCorrections = [
  { id: 1, text: 'TRACK RE-IDENTIFICATION', loc: 'RACK 1109' },
  { id: 2, text: 'RECENT RE-IDENTIFICATION', loc: 'RACK 1109' },
  { id: 3, text: 'RECENT RE-IDENTIFICATION', loc: 'RACK 1109' },
  { id: 4, text: 'RECENT RE-IDENTIFICATION', loc: 'RACK 1109' },
];

export const mockEvents = [
  { id: 1, time: '12:51:30Z', source: 'SAT-INT', type: 'safe', desc: 'NEW CONTACT | TRK 1201 | TYPE: SMALL CRAFT | LOC: 35.1N 139.8E' },
  { id: 2, time: '12:50:55Z', source: 'SIGINT', type: 'safe', desc: 'COMMS INTERCEPT | GORSHKOV GROUP | FREQ: 420.5 MHz' },
  { id: 3, time: '12:50:55Z', source: 'SIGINT', type: 'safe', desc: 'COMMS INTERCEPT | GORSHKOV GROUP | FREQ: 420.5 MHz' },
  { id: 4, time: '12:50:54Z', source: 'SIGINT', type: 'safe', desc: 'COMMS INTERCEPT | GORSHKOV GROUP | FREQ: 420.5 MHz' },
  { id: 5, time: '12:50:57Z', source: 'SIGINT', type: 'safe', desc: 'COMMS INTERCEPT | GORSHKOV GROUP | FREQ: 420.5 MHz' },
  { id: 6, time: '12:50:28Z', source: 'AIS', type: 'info', desc: 'NEW CONTACT | TRK 1201 | TYPE: SMALL CRAFT' },
];
