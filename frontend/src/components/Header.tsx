import React from 'react';

const Header = () => {
  return (
    <div className="w-full flex items-center justify-between px-4 py-2 bg-[#0f172a] border-b border-[#334155]">
      <div className="flex items-center space-x-3">
        <div className="w-8 h-8 rounded-full bg-[#1e293b] border border-[#334155] flex items-center justify-center text-xs font-bold text-[#94a3b8]">USN</div>
        <h1 className="text-xl font-bold tracking-wider">CNIC INTELLIGENCE DASHBOARD</h1>
      </div>
      <div className="text-[#10b981] text-xs font-semibold tracking-widest uppercase">
        UNCLASSIFIED // FOUO
      </div>
      <div className="flex flex-col items-end">
        <div className="text-lg font-bold mono">12:52:15 ZULU</div>
        <div className="text-[#10b981] text-xs uppercase font-semibold">SYSTEM ONLINE</div>
      </div>
    </div>
  );
};

export default Header;
