import React from 'react';
import { MessageSquareWarning, AlertTriangle, ShieldAlert } from 'lucide-react';
import { mockAlerts } from '../mockData';

const AlertFeed = () => {
  return (
    <div className="h-full flex flex-col bg-[#1e293b]/40 border-r border-[#334155]">
      <div className="flex items-center space-x-2 px-4 py-3 border-b border-[#334155]">
        <MessageSquareWarning className="w-5 h-5 text-[#10b981]" />
        <div>
          <h2 className="text-sm font-bold uppercase tracking-wider">ALERT FEED</h2>
          <div className="text-[10px] text-[#94a3b8] uppercase">Prioritized Military Intelligence</div>
        </div>
      </div>
      
      <div className="flex-1 overflow-y-auto p-2 space-y-2">
        <div className="flex">
          <div className="w-8 flex flex-col items-center space-y-4 pt-4 border-r border-[#334155]/50 mr-2 text-[#94a3b8]">
            <AlertTriangle className="w-4 h-4 cursor-pointer hover:text-white" />
            <ShieldAlert className="w-4 h-4 cursor-pointer hover:text-white" />
          </div>
          <div className="flex-1 space-y-2">
            {mockAlerts.map((alert) => {
              const bgColors = {
                critical: 'bg-[#ef4444]/20 border-l-[#ef4444]',
                high: 'bg-[#f59e0b]/20 border-l-[#f59e0b]',
                info: 'bg-[#3b82f6]/20 border-l-[#3b82f6]',
                safe: 'bg-[#10b981]/20 border-l-[#10b981]',
              };
              const textColors = {
                critical: 'text-[#ef4444]',
                high: 'text-[#f59e0b]',
                info: 'text-[#3b82f6]',
                safe: 'text-[#10b981]',
              };
              
              return (
                <div key={alert.id} className={`p-2 border border-[#334155] border-l-4 ${bgColors[alert.severity as keyof typeof bgColors]}`}>
                  <div className="flex justify-between items-start">
                    <div className={`text-xs font-bold ${textColors[alert.severity as keyof typeof textColors]}`}>{alert.title}</div>
                    <div className="text-[10px] mono text-[#94a3b8]">{alert.time}</div>
                  </div>
                  <div className="text-xs text-white mt-1 uppercase">{alert.subtext}</div>
                </div>
              );
            })}
          </div>
        </div>
      </div>
    </div>
  );
};

export default AlertFeed;
