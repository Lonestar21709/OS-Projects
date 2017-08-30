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
      if( cpuHead->cpuBusy == false ){
         scheduleArrival();
         if( rHead != 0 ){
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
// returns a random number bewteen 0 and 1
float urand(){
   return( (float) rand() / RAND_MAX );
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
