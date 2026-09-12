import React from 'react';
import Header from './components/Header';
import AlertFeed from './components/AlertFeed';
import IntelligenceSummary from './components/IntelligenceSummary';
import EventLog from './components/EventLog';
import Dossier from './components/Dossier';

function App() {
  return (
    <div className="h-screen w-screen flex flex-col bg-[#0B1120] text-white overflow-hidden selection:bg-[#3b82f6]/30">
      <Header />
      
      {/* Main 3-Column Layout */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left Sidebar ~20% */}
        <div className="w-1/5 min-w-[250px] max-w-[300px]">
          <AlertFeed />
        </div>
        
        {/* Center Main ~55% */}
        <div className="flex-1 flex flex-col min-w-[500px]">
          <div className="flex-1 overflow-hidden">
            <IntelligenceSummary />
          </div>
          <div className="h-[25%] min-h-[150px]">
            <EventLog />
          </div>
        </div>
        
        {/* Right Dossier ~25% */}
        <div className="w-1/4 min-w-[300px] max-w-[400px]">
          <Dossier />
        </div>
      </div>
    </div>
  );
}

export default App;
