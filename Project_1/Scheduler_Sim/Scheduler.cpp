#include "header.h"
#include <cmath>
#include <stdio.h>
#include <stdlib.h>               // rand()
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;

/////////////////////////////////////////////////////
// define structs
struct cpuNode{
   float clock;                // simulation clock
   bool cpuBusy;                // busy flag
   struct process* pLink;    // the target process
};

struct readyQueue{
   struct process* pLink;    // point to matching process in process list
   struct readyQueue* rNext;      // point to next process in the ready queue
};

struct eventQueue{
   float time;
   int type;                      // 1 = arrival; 2 = departure;
                                  // 3 = allocation; 4 = preemption
   struct eventQueue* eNext;      // point to next event in the event queue
   struct process* pLink;    // point to matching process in process list
};

struct process{
   float arrivalTime;
   float startTime;
   float reStartTime;
   float finishTime;
   float serviceTime;
   float remainingTime;
   struct process* pNext;    // point to next process in the list
};

/////////////////////////////////////////////////////
// declare global variables
int schedulerType;               // argv[1] from console
int lambda;                      // argv[2] from console
float avgTs;                     // argv[3] from console
float quantum;                   // argv[4] from console
int batchSize;                   // argv[5] from console
float mu;                        // 1/avgTs
float quantumClock;              // for RR
eventQueue* eHead;               // head of event queue
process* pHead;             // head of oprocess list
readyQueue* rHead;               // head of ready queue
cpuNode* cpuHead;                // the cpu node (there's only one)

/////////////////////////////////////////////////////
// helper function
void insertIntoEventQ(eventQueue*);
float getResponseRatioValue(process*);
process* getSRTFProcess();
process* getHRRNProcess();

/////////////////////////////////////////////////////
// define function implementations

// initialize global variables to values of args from console
void parseArgs(char *argv[]){
   schedulerType = atoi( argv[1] );
   lambda = atoi( argv[2] );
   avgTs = (float)atof( argv[3] );
   quantum = (float)atof( argv[4] );
   batchSize = atoi( argv[5] );
}

// initialize all variable, states, and end conditions
void init(){
   // mu is used in genExp(float) to get service time
   mu = (float)1.0/avgTs;

   // initialize the RR quantum clock to 0
   quantumClock = 0.0;

   // create the cpu node
   cpuHead = new cpuNode;
   cpuHead->clock = 0.0;
   cpuHead->cpuBusy = 0;   // cpu flag: 0=false=idle, 1=true=busy
   cpuHead->pLink = 0;


   // create process list node, point pHead to it, initialize member vars
   // this first node is being initialized as the first process for the sim
   pHead = new process;
   pHead->arrivalTime = genExp((float)lambda);
   pHead->startTime = 0.0;
   pHead->reStartTime = 0.0;
   pHead->finishTime = 0.0;
   pHead->serviceTime = genExp(mu);
   pHead->remainingTime = pHead->serviceTime;
   pHead->pNext = 0;

   // create event queue node, point pHead to it, initialize member vars
   eHead = new eventQueue;
   eHead->time = pHead->arrivalTime;
   eHead->type = 1;
   eHead->eNext = 0;
   eHead->pLink = pHead;
}

void run_sim(){
   switch ( schedulerType ){
     case 1:
        cout << "The sim is running FCFS. . . " << endl;
        FCFS();
        break;
     case 2:
        cout << "The sim is running SRTF. . . " << endl;
        SRTF();
        break;
     case 3:
        cout << "The sim is running HRRN. . . " << endl;
        HRRN();
        break;
     case 4:
        cout << "The sim is running RR. . . " << endl;
        RR();
        break;
     default:
        cout << "Error in run_sim(). . . " << endl;
   }
}

void FCFS(){
   int departureCount = 0;
   while( departureCount < batchSize ){
      // CASE 1: cpu is not busy -------------------
      if( cpuHead->cpuBusy == false )
        {
         scheduleArrival();
         if( rHead != 0 )
         {
         scheduleAllocation();
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

void SRTF(){
   int arrivalCount = 0;
   int departureCount = 0;
   while( departureCount < batchSize ){
      // on each pass, schedule an arrival, up to batchSize * 1.20
      if( arrivalCount < ( batchSize * 1.20 ) ){
         scheduleArrival();
         arrivalCount++;
      }
      // CASE 1: cpu is not busy -------------------
      if( cpuHead->cpuBusy == false ){
         if( rHead != 0 ) scheduleAllocation();
      }
      // CASE 2: cpu is busy -----------------------
      else{
         // if( okayToDepart ) scheduleDeparture();
         if( eHead->type == 1 ){
            // if arrival time > cpu finish time, it's ok to depart
            if( eHead->time > cpuEstFinishTime() ){
               scheduleDeparture();
            }
            // if arrival preempts cpu, then schedule a preemption
            else if( isPreemptive() ){
               schedulePreemption();
            }
         }
      }
      // ANY CASE: handle next event ---------------
      if( eHead->type == 1) handleArrival();
      else if( eHead->type == 2 ){
         handleDeparture();
         //okayToDepart = false;
         departureCount++;
      }
      else if( eHead->type == 3 ) handleAllocation();
      else if( eHead->type == 4 ) handlePreemption();
   } // end while
}

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

void RR(){
   int arrivalCount = 0;
   int departureCount = 0;
   while( departureCount < batchSize ){
      // on each pass, schedule an arrival, up to batchSize * 1.20
      if( arrivalCount < ( batchSize * 1.20 ) ){
         scheduleArrival();
         arrivalCount++;
      }
      // CASE 1: cpu is not busy -------------------
      if( cpuHead->cpuBusy == false ){
         scheduleArrival();
         if( rHead != 0 ){
            scheduleQuantumAllocation();
         }
      }
      // CASE 2: cpu is busy -----------------------
      else{
         // if job will finish before time slice ends, schedule departure
         if( cpuEstFinishTime() < getNextQuantumClockTime() ){
            scheduleQuantumDeparture();
         }
         else{
            if( rHead != 0 ){
               if( rHead->pLink->arrivalTime > cpuEstFinishTime() ){
                  scheduleQuantumDeparture();
               }
               else{
                  scheduleQuantumPreemption();
               }
            }
         }
      }
   // ANY CASE: handle next event ---------------
   if( eHead->type == 1 ) handleArrival();
   else if( eHead->type == 2 ){
      handleQuantumDeparture();
      departureCount++;

      // if a job is waiting in readyQ, allocate it to cpu
      // immediately without waiting for next time slice
      if( rHead != 0 && ( rHead->pLink->arrivalTime <
                          cpuHead->clock) ){
         scheduleQuantumAllocation();
      }
   }
   else if( eHead->type == 3 ) handleQuantumAllocation();
   else if( eHead->type == 4 ) handleQuantumPreemption();
   } // end while
}

float cpuEstFinishTime(){
   float estFinish;
   float startTime = cpuHead->pLink->startTime;
   float reStartTime = cpuHead->pLink->reStartTime;
   float remainingTime = cpuHead->pLink->remainingTime;

   if( reStartTime == 0 ) estFinish = startTime + remainingTime;
   else estFinish = reStartTime + remainingTime;

   return estFinish;
}

bool isPreemptive(){
   // get estimated cpu finish time
   float cpuFinishTime = cpuEstFinishTime();

   // get estimated remaining time if cpu proc is preempted
   float cpuRemainingTime = cpuFinishTime - eHead->time;

   // if arrival time < cpuFinishTime && arrival's
   // remaining time < cpuRemainingTime, then arrival is preemptive
   if( ( eHead->time < cpuFinishTime ) &&
       ( eHead->pLink->remainingTime <
         cpuRemainingTime ) ){
      return true;
   }
   else return false;
}

// adds a process to process list and
// schedules an arrival event in eventQ
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

// moves process from eventQ to readyQ
void handleArrival(){
   // create a new readyQ node based on proc in eHead
   readyQueue* nuReady = new readyQueue;
   nuReady->pLink = eHead->pLink;
   nuReady->rNext = 0;

   // push the new node into the readyQ
   if( rHead == 0 ) rHead = nuReady;
   else{
      readyQueue* rIt = rHead;
      while( rIt->rNext != 0 ){
         rIt = rIt->rNext;
      }
      rIt->rNext = nuReady;
   }

   // pop the arrival from the eventQ
   popEventQHead();
}

// schedules an allocation event in eventQ
void scheduleAllocation(){
   // create a new event queue node
   eventQueue* nuAllocation = new eventQueue;

   // identify the next process to be allocated to the cpu:
   process* nextProc;
   if( schedulerType == 1 ) nextProc = rHead->pLink;          // FCFS
   else if( schedulerType == 2 ){			                  // SRTF
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

// moves process from readyQ to CPU
void handleAllocation(){
   // point cpu to the proc named in the allocation event
   cpuHead->pLink = eHead->pLink;

   if( schedulerType == 2 ||      // FCFS
      schedulerType == 3 ){       // HRRN
      // find the corresponding process in readyQ and move
      // it to top of readyQ if it's not already there
      readyQueue* rIt = rHead->rNext;
      readyQueue* rItPrev = rHead;
      if( rItPrev->pLink->arrivalTime != eHead->pLink->arrivalTime ){
         while( rIt != 0 ){
	    if( rIt->pLink->arrivalTime ==
		   eHead->pLink->arrivalTime ){
               rItPrev->rNext = rIt->rNext;
               rIt->rNext = rHead;
               rHead = rIt;
               break;
            }
            rIt = rIt->rNext;
            rItPrev = rItPrev->rNext;
         }
      }
   }

   // pop the readyQ and eventQ records
   popReadyQHead();
   popEventQHead();

   // set the busy flag to show the cpu is now busy
   cpuHead->cpuBusy = true;

   // update sim clock
   if( cpuHead->clock < cpuHead->pLink->arrivalTime ){
      // if clock < arrival time, then clock = arrival time
      cpuHead->clock = cpuHead->pLink->arrivalTime;
   }

   // update start/restart time as needed
   if( cpuHead->pLink->startTime == 0 ){
      cpuHead->pLink->startTime = cpuHead->clock;
   }
   else{
      cpuHead->pLink->reStartTime = cpuHead->clock;
   }
}

// schedules a departure event in eventQ
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

// terminates a process and clears the cpu
void handleDeparture(){
   // update cpu data
   cpuHead->pLink->finishTime = eHead->time;
	cpuHead->pLink->remainingTime = 0.0;
   cpuHead->pLink = 0;
   cpuHead->clock = eHead->time;
   cpuHead->cpuBusy = false;

   // pop the departure from the eventQ
   popEventQHead();
}

// schedules a preemption event in eventQ
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

// moves a preempting proc from readyQ to the cpu
void handlePreemption(){
   // create a temp ptr to hold the current cpu pLink
   process* preemptedProcPtr = cpuHead->pLink;

   // update the remaining time
   cpuHead->pLink->remainingTime =
      cpuEstFinishTime() - eHead->time;

   // point cpu to preempting process and update data as needed
   cpuHead->pLink = eHead->pLink;
   cpuHead->clock = eHead->time;
   if( cpuHead->pLink->reStartTime == 0.0  ){
      cpuHead->pLink->startTime = eHead->time;
   }
   else{
      cpuHead->pLink->reStartTime = eHead->time;
   }

   // schedule an arrival event for the preempted proc
   eventQueue* preemptedProcArrival = new eventQueue;
   preemptedProcArrival->time = eHead->time;
   preemptedProcArrival->type = 1;
   preemptedProcArrival->eNext = 0;
   preemptedProcArrival->pLink = preemptedProcPtr;

   // pop the preemption event from the eventQ
   popEventQHead();

   // insert new event into eventQ
   insertIntoEventQ( preemptedProcArrival );
}

void scheduleQuantumAllocation(){
   // create a new event queue node
   eventQueue* nuAllocation = new eventQueue;

   // identify the next process to be allocated to the cpu:
   process* nextProc;
   nextProc = rHead->pLink;

   // set the time for allocation if it begins mid-quantum
   if( rHead != 0 ){
      if( rHead->pLink->arrivalTime < cpuHead->clock ){
         nuAllocation->time = cpuHead->clock;
      }
      else{
         // update simClock
         cpuHead->clock = rHead->pLink->arrivalTime;

         // update quantum clock
         float nextQuantumTime = quantumClock;
         while( nextQuantumTime < cpuHead->clock ){
            nextQuantumTime += quantum;
         }
         quantumClock = nextQuantumTime;

         // and set the time for allocation to begin on a quantum
         nuAllocation->time = getNextQuantumAllocationTime();
      }
   }
   else{
      cout << "Error in scheduleQuantumAllocation()" << endl;
   }

   // set the values for type, next, and pLink
   nuAllocation->type = 3;
   nuAllocation->eNext = 0;
   nuAllocation->pLink = nextProc;

   // insert new event into eventQ
   insertIntoEventQ( nuAllocation );
}

void handleQuantumAllocation(){
   // point cpu to the proc named in the allocation event
   cpuHead->pLink = eHead->pLink;

   // set the busy flag to show the cpu is now busy
   cpuHead->cpuBusy = true;

   // update start/restart time as needed
   if( cpuHead->pLink->startTime == 0 ){
      cpuHead->pLink->startTime = eHead->time;
   }
   else{
      cpuHead->pLink->reStartTime = eHead->time;
   }

   // pop the readyQ and eventQ records
   popReadyQHead();
   popEventQHead();
}

void scheduleQuantumDeparture(){
   // create a new event node for the departure event
   eventQueue* nuDeparture = new eventQueue;
   nuDeparture->type = 2;
   nuDeparture->eNext = 0;
   nuDeparture->pLink = cpuHead->pLink;

   // set the departure time for the event
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

   // insert the new event into eventQ in asc time order
   insertIntoEventQ(nuDeparture);
}

void handleQuantumDeparture(){
   // update cpu data
   cpuHead->pLink->finishTime = eHead->time;
      cpuHead->pLink->remainingTime = 0.0;
   cpuHead->pLink = 0;
   cpuHead->clock = eHead->time;
   cpuHead->cpuBusy = false;

   // pop the departure from the eventQ
   popEventQHead();
}

void scheduleQuantumPreemption(){
   // create a new preemption event
   eventQueue* nuPreemption = new eventQueue;
   nuPreemption->type = 4;
   nuPreemption->eNext = 0;

   // update simClock
   cpuHead->clock = rHead->pLink->arrivalTime;

   // update quantum clock
   float nextQuantumTime = quantumClock;
   while( nextQuantumTime < cpuHead->clock ){
      nextQuantumTime += quantum;
   }
   quantumClock = nextQuantumTime;

   // set the time for the preemption event
   nuPreemption->time = getNextQuantumClockTime();

   // link to the preempting process
   nuPreemption->pLink = rHead->pLink;

   // insert new event into eventQ
   insertIntoEventQ( nuPreemption );
}

void handleQuantumPreemption(){
   // create a temp ptr to hold the current cpu pLink
   process* preemptedProcPtr = cpuHead->pLink;

   // update the remaining time
   cpuHead->pLink->remainingTime =
      cpuEstFinishTime() - eHead->time;

   // point cpu to preempting process and update data as needed
   cpuHead->pLink = eHead->pLink;
   cpuHead->clock = eHead->time;
   if( cpuHead->pLink->startTime == 0.0  ){
      cpuHead->pLink->startTime = eHead->time;
   }
   else{
      cpuHead->pLink->reStartTime = eHead->time;
   }

   // update quantum clock
   float nextQuantumTime = quantumClock;
   while( nextQuantumTime < eHead->time  ){
      nextQuantumTime += quantum;
   }
   quantumClock = nextQuantumTime;

   // schedule an arrival event for the preempted proc
   eventQueue* preemptedProcArrival = new eventQueue;
   preemptedProcArrival->time = eHead->time;
   preemptedProcArrival->type = 1;
   preemptedProcArrival->eNext = 0;
   preemptedProcArrival->pLink = preemptedProcPtr;

   // pop the preemption event from the eventQ
   popEventQHead();

   // pop the preempting process from the readyQ
   popReadyQHead();

   // insert new event into eventQ
   insertIntoEventQ( preemptedProcArrival );
}

float getNextQuantumClockTime(){
   return quantumClock + quantum;
}

float getNextQuantumAllocationTime(){
   float nextQuantumTime = quantumClock;
   while( nextQuantumTime < rHead->pLink->arrivalTime  ){
      nextQuantumTime += quantum;
   }
   return nextQuantumTime;
}

/////////////////////////////////////////////////////
// helper functions

// returns a random number bewteen 0 and 1
float urand(){
   return( (float) rand() / RAND_MAX );
}

// returns a random number that follows an exp distribution
float genExp(float val){
   float u, x;
   x = 0;
   while( x == 0 ){
      u = urand();
      x = (-1/val)*log(u);
   }
   return x;
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

void popEventQHead(){
   eventQueue* tempPtr = eHead;
   eHead = eHead->eNext;
   delete tempPtr;
}

void popReadyQHead(){
   readyQueue* tempPtr = rHead;
   rHead = rHead->rNext;
   delete tempPtr;
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

/////////////////////////////////////////////////////
// metric computation functions
float getAvgTurnaroundTime(){
   float totTurnaroundTime = 0.0;
   process* pIt = pHead;
   while( pIt->finishTime != 0 ){
      // tally up the turnaround times
      totTurnaroundTime +=
         ( pIt->finishTime - pIt->arrivalTime );
      pIt = pIt->pNext;
   }
   return (totTurnaroundTime/batchSize);
}

float getTotalThroughput(){
   process* pIt = pHead;
   float finTime = 0.0;
   while( pIt->finishTime != 0 ){
      // get the final timestamp
      if( pIt->pNext->finishTime == 0 )
         finTime = pIt->finishTime;
      pIt = pIt->pNext;
   }
   return ( (float)batchSize / finTime );
}

float getCpuUtil(){
   process* pIt = pHead;
   float busyTime = 0.0;
   float finTime = 0.0;
   while( pIt->finishTime != 0 ){
      busyTime += pIt->serviceTime;
      if( pIt->pNext->finishTime == 0 )
         finTime = pIt->finishTime;
      pIt = pIt->pNext;
   }
   return ( busyTime / finTime );
}

float getAvgNumProcInQ(){
   // identify the final second of processing (timeN)
   // as it would appear on a seconds-based timeline
   float timeNmin1 = 0.0;
   process* pIt = pHead;
   while( pIt->finishTime != 0 ){
      if( pIt->pNext->finishTime == 0 )
         timeNmin1 = pIt->finishTime;
      pIt = pIt->pNext;
   }
   int timeN = static_cast<int>(timeNmin1) + 1;

   // tally up the total processes in the ready queue
   // for each second of the seconds-based timeline
   pIt = pHead;
   int time = 0;;
   int numProcsInQ = 0;
   for( time = 0; time < timeN; time++ ){
      while( pIt->finishTime != 0 ){
         if( ( pIt->arrivalTime < time && pIt->startTime > time ) ||
             ( pIt->arrivalTime > time && pIt->arrivalTime < (time + 1) ) ){
            numProcsInQ ++;
         }
         pIt = pIt->pNext;
     }
     pIt = pHead;
   }

   return ( (float)numProcsInQ / timeN );
}

void generate_report(){
	string scheduler;

	if( schedulerType == 1 )      scheduler = "FCFS()";
   else if( schedulerType == 2 ) scheduler = "SRTF()";
   else if( schedulerType == 3 ) scheduler = "HRRN()";
   else if( schedulerType == 4 ) scheduler = "RORO()";


   if( lambda == 1 ){
      ofstream report ( "report.txt", ios::out | ios::app );
      report << endl
          << "Scheduler \tLambda \t\tAvgTs\t AvgTurnaround \tTotalThruput\tCPU Util\tAvg#ProcsInQ \tQuantum" << endl
          << "---------------------------------------------------------------------------------------------------------------"
          << endl;
      report.close();
   }

	ofstream report ( "report.txt", ios::out | ios::app );
   report << scheduler << "\t\t"
          << setprecision(7)
          << lambda << "     \t\t"
          << avgTs  << "\t"
          << getAvgTurnaroundTime() << "\t"
          << getTotalThroughput() << "\t"
          << getCpuUtil() << "\t"
          << getAvgNumProcInQ() << "\t"
			 << quantum
          << endl;
   report.close();

	cout << endl << "Program completed. "
        << "Results written to <report.txt> in the .exe folder" << endl;
}
