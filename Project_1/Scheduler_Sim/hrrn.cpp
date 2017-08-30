void HRRN(){
   int departureCount = 0;
   while( departureCount < batchSize ){
      // CASE 1: cpu is not busy -------------------
      if( cpuHead->cpuBusy == false ){
         scheduleArrival();
         if( rHead != 0 ){
            scheduleAllocation(); // HRR rules are in the method
         }
       }
       // CASE 2: cpu is busy -----------------------
       else scheduleDeparture();

       // ANY CASE: handle next event ---------------
       if( eHead->type == 1 ) handleArrival();
       else if( eHead->type == 2 ){
          handleDeparture();
          departureCount++;
       }
       else if( eHead->type == 3 ) handleAllocation();
   } // end while
}


process* getHRRNProcess(){
   readyQueue* rIt = rHead;
   process* hrrProc = rIt->pLink;
   float hrr = getResponseRatioValue( hrrProc );

   while( rIt != 0){
      if( getResponseRatioValue( rIt->pLink ) > hrr ){
         hrr = getResponseRatioValue( rIt->pLink );
	 hrrProc = rIt->pLink;
      }
      rIt = rIt->rNext;
   }
   return hrrProc;
}
float getResponseRatioValue( process* thisProc ){
   float HRR = ( ( cpuHead->clock -
                   thisProc->arrivalTime ) +
		   thisProc->serviceTime ) /
		   thisProc->serviceTime;
   return HRR;
}
