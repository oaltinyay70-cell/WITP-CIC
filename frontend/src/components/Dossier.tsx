import React from 'react';
import { Search } from 'lucide-react';

const Dossier = () => {
  return (
    <div className="h-full bg-[#1e293b]/40 border-l border-[#334155] flex flex-col">
      <div className="px-4 py-3 border-b border-[#334155] flex justify-between items-center">
        <h2 className="text-sm font-bold uppercase tracking-wider">DOSSIER</h2>
      </div>
      
      <div className="p-3">
        <div className="relative mb-4">
          <input 
            type="text" 
            placeholder="Search Profiles, Tracks, Assets..." 
            className="w-full bg-[#0B1120] border border-[#334155] rounded text-sm text-white px-3 py-2 pl-8 focus:outline-none focus:border-[#3b82f6]"
          />
          <Search className="w-4 h-4 text-[#94a3b8] absolute left-2 top-2.5" />
        </div>
        
        <div className="h-32 bg-gray-700 w-full rounded mb-3 flex items-center justify-center text-gray-400">
          [Asset Image Placeholder]
        </div>
        
        <h3 className="text-[#ef4444] font-bold text-sm mb-3 uppercase">
          HVT: ADMIRAL GORSHKOV CLASS FRIGATE (AA-03)
        </h3>
        
        <div className="space-y-1 mb-4 text-xs">
          <div className="text-white"><span className="text-[#94a3b8]">Propulsion:</span> 13.38 km (Inspusion)</div>
          <div className="text-white"><span className="text-[#94a3b8]">Range:</span> 2500 km (300 km/s)</div>
          <div className="text-white"><span className="text-[#94a3b8]">Weapon Systems:</span> RNAS Gotzurs Pormo, Becoet Completon (PIII)</div>
          <div className="text-white"><span className="text-[#94a3b8]">Complement:</span> 70 Complements</div>
        </div>
        
        <div className="border-t border-[#334155] pt-3 mb-3 text-xs">
          <div className="text-[#94a3b8] mb-1">Last known Location:</div>
          <div className="text-white font-semibold">ES. 3R.1M, 35.1N 139.8E</div>
        </div>
        
        <div className="border-t border-[#334155] pt-3 mb-3 text-xs">
          <div className="text-[#94a3b8] mb-1">Associated Task Group:</div>
          <div className="text-white font-semibold">Uron A</div>
        </div>
        
        <div className="border-t border-[#334155] pt-3 text-xs">
          <div className="text-white font-bold mb-2 uppercase">Recent Activity Log</div>
          <div className="space-y-1 mono text-[#94a3b8]">
            <div>12:51:30Z: TRACK | GUMBACU...</div>
            <div>12:51:30Z: TASK. 6NHRT1109-DREC168</div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Dossier;
