void scheduleArrival(){
   // add a process to the process list
   process* pIt = pHead;
   while( pIt->pNext !=0 ){
      pIt = pIt->pNext;
   }
   pIt->pNext = new process;
   pIt->pNext->arrivalTime = pIt->arrivalTime + genExp((float)lambda);
   pIt->pNext->startTime = 0.0;
   pIt->pNext->reStartTime = 0.0;
   pIt->pNext->finishTime = 0.0;
   pIt->pNext->serviceTime = genExp(mu);
   pIt->pNext->remainingTime = pIt->pNext->serviceTime;
   pIt->pNext->pNext = 0;

   // create a corresponding arrival event
   eventQueue* nuArrival = new eventQueue;
   nuArrival->time = pIt->pNext->arrivalTime;
   nuArrival->type = 1;
   nuArrival->pLink = pIt->pNext;
   nuArrival->eNext = 0;

   // insert into eventQ in asc time order
   insertIntoEventQ(nuArrival);
}
void scheduleAllocation(){
   // create a new event queue node
   eventQueue* nuAllocation = new eventQueue;

   // identify the next process to be allocated to the cpu:
   process* nextProc;
   if( schedulerType == 1 ) nextProc = rHead->pLink;          // FCFS
   else if( schedulerType == 2 ){			      // SRTF
      if( cpuHead->clock > rHead->pLink->arrivalTime ){
         nextProc = getSRTFProcess();
      }
      else{
         nextProc = rHead->pLink;
      }
   }
   else if( schedulerType == 3 ){                             // HRRN
      nextProc = getHRRNProcess();
   }

   // set the time of the allocation event
   if( cpuHead->clock < nextProc->arrivalTime ){
      nuAllocation->time = nextProc->arrivalTime;
   }
   else{
      nuAllocation->time = cpuHead->clock;
   }

   // set the values for type, next, and pLink
   nuAllocation->type = 3;
   nuAllocation->eNext = 0;
   nuAllocation->pLink = nextProc;

   // insert new event into eventQ
   insertIntoEventQ( nuAllocation );
}
void scheduleDeparture(){
   // create a new event node for the departure event
   eventQueue* nuDeparture = new eventQueue;
   nuDeparture->type = 2;
   nuDeparture->eNext = 0;
   nuDeparture->pLink = cpuHead->pLink;

   // set the departure time for the event
   if( schedulerType == 1 ||                  // FCFS
       schedulerType == 3 ){                  // HRRN
          nuDeparture->time =
	     cpuHead->pLink->startTime +
             cpuHead->pLink->remainingTime;
   }
   else if( schedulerType == 2 ){             // SRTF
      if( cpuHead->pLink->reStartTime == 0 ){
         nuDeparture->time =
            cpuHead->pLink->startTime +
            cpuHead->pLink->remainingTime;
      }
      else{
      nuDeparture->time =
         cpuHead->pLink->reStartTime +
         cpuHead->pLink->remainingTime;
      }
   }

   // insert the new event into eventQ in asc time order
   insertIntoEventQ(nuDeparture);
}

void schedulePreemption(){
   eventQueue* nuPreemption = new eventQueue;
   nuPreemption->time = eHead->time;
   nuPreemption->type = 4;
   nuPreemption->eNext = 0;
   nuPreemption->pLink = eHead->pLink;

   // pop the arrival event from eHead so that
   // it can be replaced by the preemption event
   popEventQHead();

   // insert new event into eventQ
   insertIntoEventQ( nuPreemption );
}
process* getSRTFProcess(){
   // pre-condition: there is something in the readyQ
   readyQueue* rIt = rHead;
   process* srtProc = rIt->pLink;
   float srt = rIt->pLink->remainingTime;
   while( rIt != 0){
      if( rIt->pLink->remainingTime < srt ){
         srt = rIt->pLink->remainingTime;
	 srtProc = rIt->pLink;
      }
      rIt = rIt->rNext;
   }
   return srtProc;
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
void insertIntoEventQ( eventQueue* nuEvent ){
   // put the new event in the readyQ, sorted by time
   if( eHead == 0 ) eHead = nuEvent;
   else if( eHead->time > nuEvent->time ){
      nuEvent->eNext = eHead;
      eHead = nuEvent;
   }
   else{
      eventQueue* eIt = eHead;
      while( eIt != 0 ){
         if( (eIt->time < nuEvent->time) && (eIt->eNext == 0) ){
            eIt->eNext = nuEvent;
	    break;
         }
         else if( (eIt->time < nuEvent->time) &&
                  (eIt->eNext->time > nuEvent->time)){
            nuEvent->eNext = eIt->eNext;
            eIt->eNext = nuEvent;
            break;
         }
         else{
            eIt = eIt->eNext;
         }
      }
   }
}
