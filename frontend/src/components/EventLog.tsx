import React from 'react';
import { mockEvents } from '../mockData';

const EventLog = () => {
  return (
    <div className="bg-[#1e293b]/40 border-t border-[#334155] flex flex-col">
      <div className="px-4 py-2 border-b border-[#334155] flex justify-between items-center">
        <h2 className="text-sm font-bold uppercase tracking-wider">EVENT LOG</h2>
      </div>
      <div className="p-2 overflow-y-auto max-h-48 text-xs mono">
        {mockEvents.map(event => (
          <div key={event.id} className="flex space-x-4 py-1 hover:bg-[#334155]/30">
            <div className="w-20 text-[#94a3b8]">{event.time}</div>
            <div className={`w-16 font-bold ${event.type === 'safe' ? 'text-[#10b981]' : 'text-[#3b82f6]'}`}>
              [{event.source}]
            </div>
            <div className="flex-1 text-[#e2e8f0]">{event.desc}</div>
          </div>
        ))}
      </div>
    </div>
  );
};

export default EventLog;
