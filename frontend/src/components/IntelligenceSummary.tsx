import React from 'react';
import { mockHVTs, mockPorts, mockCorrections } from '../mockData';

const IntelligenceSummary = () => {
  return (
    <div className="h-full flex flex-col p-4 space-y-4 overflow-y-auto">
      {/* Tabs */}
      <div className="flex border-b border-[#334155]">
        <div className="px-6 py-2 border-b-2 border-white text-white font-bold tracking-wider text-sm">
          INTELLIGENCE SUMMARY
        </div>
        <div className="px-6 py-2 text-[#94a3b8] font-bold tracking-wider text-sm cursor-pointer hover:text-white">
          SIGINT MAP
        </div>
      </div>
      
      {/* Grid Container */}
      <div className="grid grid-cols-3 gap-4 flex-1">
        
        {/* HVTs */}
        <div className="col-span-1 border border-[#334155] bg-[#1e293b]/20 flex flex-col">
          <div className="p-2 border-b border-[#334155] font-bold text-sm tracking-wider uppercase">HIGH-VALUE TARGETS (HVTs)</div>
          <div className="p-2 flex space-x-2">
            <div className="flex-1 bg-[#ef4444]/20 border border-[#ef4444] flex flex-col items-center justify-center py-2">
              <span className="text-[#ef4444] text-xl font-bold">1</span>
              <span className="text-[#ef4444] text-xs">CRITICAL</span>
            </div>
            <div className="flex-1 bg-[#f59e0b]/20 border border-[#f59e0b] flex flex-col items-center justify-center py-2">
              <span className="text-[#f59e0b] text-xl font-bold">3</span>
              <span className="text-[#f59e0b] text-xs">HIGH</span>
            </div>
          </div>
          <div className="flex-1 p-2 space-y-2">
            {mockHVTs.map(hvt => (
              <div key={hvt.id} className="text-xs">
                <div className="flex justify-between mb-1">
                  <span className="font-bold">{hvt.track}</span>
                  <span className={hvt.color === 'critical' ? 'text-[#ef4444]' : 'text-[#f59e0b]'}>{hvt.value}</span>
                </div>
                <div className="flex justify-between text-[#94a3b8] mb-1">
                  <span>{hvt.name}</span>
                  <span>TRACK {hvt.id}</span>
                </div>
                <div className="w-full h-1 bg-[#334155]">
                  <div className={`h-1 ${hvt.color === 'critical' ? 'bg-[#ef4444]' : 'bg-[#f59e0b]'}`} style={{width: `${hvt.value * 2}%`}}></div>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Strike Concentrations */}
        <div className="col-span-1 border border-[#334155] bg-[#1e293b]/20 flex flex-col">
          <div className="p-2 border-b border-[#334155] font-bold text-sm tracking-wider uppercase">STRIKE CONC.</div>
          <div className="flex-1 relative bg-[#0B1120] m-2 border border-[#334155] overflow-hidden">
            {/* Mock map background */}
            <div className="absolute inset-0 opacity-20 bg-[url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyMCIgaGVpZ2h0PSIyMCI+PGNpcmNsZSBjeD0iMSIgY3k9IjEiIHI9IjEiIGZpbGw9IiM5NGEzYjgiLz48L3N2Zz4=')]"></div>
            {/* Mock circles */}
            <div className="absolute top-1/4 left-1/4 w-8 h-8 rounded-full border border-[#f59e0b] bg-[#f59e0b]/30 flex items-center justify-center text-xs text-[#f59e0b]">3</div>
            <div className="absolute top-1/2 left-2/3 w-6 h-6 rounded-full border border-[#f59e0b] bg-[#f59e0b]/30 flex items-center justify-center text-xs text-[#f59e0b]">1</div>
            <div className="absolute bottom-1/4 left-1/3 w-10 h-10 rounded-full border border-[#f59e0b] bg-[#f59e0b]/30 flex items-center justify-center text-xs text-[#f59e0b]">2</div>
          </div>
        </div>

        {/* Port Concentrations */}
        <div className="col-span-1 border border-[#334155] bg-[#1e293b]/20 flex flex-col">
          <div className="p-2 border-b border-[#334155] font-bold text-sm tracking-wider uppercase">PORT CONCENTRATIONS</div>
          <div className="flex-1 p-2 space-y-2">
            {mockPorts.map(port => (
              <div key={port.id} className={`flex justify-between items-center p-2 border ${port.active ? 'bg-[#10b981]/20 border-[#10b981]' : 'border-[#334155]'} rounded-sm`}>
                <div>
                  <div className={`font-bold text-sm ${port.active ? 'text-[#10b981]' : 'text-white'}`}>{port.name}</div>
                  <div className="text-xs text-[#94a3b8]">{port.vessels} VESSELS PRESENT</div>
                </div>
                <div className={`w-6 h-6 rounded-full flex items-center justify-center text-xs font-bold ${port.active ? 'bg-[#10b981] text-white' : 'bg-[#334155] text-white'}`}>
                  {port.badge}
                </div>
              </div>
            ))}
          </div>
        </div>
        
        {/* Row 2 */}
        {/* Established Routes */}
        <div className="col-span-2 border border-[#334155] bg-[#1e293b]/20 flex flex-col h-48">
          <div className="p-2 border-b border-[#334155] font-bold text-sm tracking-wider uppercase">ESTABLISHED ROUTES</div>
          <div className="flex-1 relative bg-[#0B1120] m-2 border border-[#334155]">
            <div className="absolute top-2 left-2 text-xs text-[#94a3b8]">COMMON ROUTES ON THE CHART</div>
            {/* Simple SVG paths for mock routes */}
            <svg className="w-full h-full" viewBox="0 0 100 100" preserveAspectRatio="none">
              <path d="M 10,80 Q 40,20 90,30" fill="none" stroke="#10b981" strokeWidth="1" />
              <path d="M 20,90 Q 50,50 80,10" fill="none" stroke="#3b82f6" strokeWidth="1" />
              <path d="M 5,60 Q 60,80 95,50" fill="none" stroke="#f59e0b" strokeWidth="1" />
            </svg>
          </div>
        </div>

        {/* Corrections */}
        <div className="col-span-1 border border-[#334155] bg-[#1e293b]/20 flex flex-col h-48">
          <div className="p-2 border-b border-[#334155] font-bold text-sm tracking-wider uppercase">CORRECTIONS & UPDATES</div>
          <div className="flex-1 p-2 space-y-2 overflow-y-auto">
            {mockCorrections.map(corr => (
              <div key={corr.id} className="border-l-2 border-[#3b82f6] pl-2 py-1">
                <div className="text-xs font-bold text-white">{corr.text}</div>
                <div className="text-[10px] text-[#94a3b8] uppercase">{corr.text} | LOC: {corr.loc}</div>
              </div>
            ))}
          </div>
        </div>
        
      </div>
    </div>
  );
};

export default IntelligenceSummary;
