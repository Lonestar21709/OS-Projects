#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;


/**********************
       CPU Node
***********************/
struct cpu
{
    float clock; //cpu clock
    //bool idle;  // is cpu idle ?
    bool cpu_busy; //is cpu free or busy
    struct process* p_ptr; //Cpu points to process running

}; cpu* top_cpu; // head of the cpu

/***********************
   Process Ready Queue
************************/
struct ready_queue
{
    struct process* p_ptr; // points to corresponding process
    struct readyQueue* next_r; // points to next event in the ready queue

}; ready_queue* top_r; // head of ready queue

/***********************
      Event Queue
************************/
 struct event_queue
 {
     float time;
     //1 = arrival, 2 = departure, 3 = execute, 4 = preemption
     int type;
     struct event_queue* next_e;
     struct process* p_ptr;

 }; event_queue* top_e; // head of event queue

/***********************
  Process Control Block
************************/
struct process
{
   int id; //process id, will increment by 1
   // 1 = new, 2 = ready, 3 = running, 4 = waiting, 5 = terminated
   //int process_state;
   // accounting/scheduling info
   float arrival_time; //
   float service_time; //
   float execution_time;   //
   float restart_time; //
   float completion_time;  //
   float remaining_time; //
   //point to next process
   struct process* next_p; //
}; process* top_p; //head of pcb list

/**********************
  Function Prototypes
***********************/
void init();
void run_sim();
void generate_report();
//Scheduler prototypes
void FCFS();

//
void schedule_event(int type, int counter);
void process_event(int type);
void scheudle_rr();
void process_rr();
void insert_event_queue(event_queue*);
//Utility prototypes
float urand();
float genExp(float);
/**********************
    GLOBAL VARIABLES
***********************/
int num_processes; // 10,000 processes
int schedule_type; // 4 types
float lambda;        // avg arrival rate
float Ts;          // avg service time
float mew;         // avg service rate
float quantum;      //
float quantum_clock; //
int arrival_counter; // counter matches up processes as they arrive
int process_counter;
//variables to set which event to schedule and process
int arrival = 1,
    departure = 2,
    assign = 3,
    preemption = 4;

/**********************
  Function Definitions
***********************/

/**********************
      Initialize
***********************/
void init()
{
    process_counter = 0;
    num_processes = 10;
    //create the cpu
    top_cpu = new cpu;
    top_cpu->clock = 0.0; // clock is at 0
    top_cpu->cpu_busy = false; // cpu is not busy at start
    top_cpu->p_ptr = 0; // no process in cpu at start


    // first process initialize, that top will always point too
    //which will have an id = 0
    top_p = new process;
    top_p->id = process_counter;
    //top_p->process_state = 1; // new states
    top_p->arrival_time = genExp(lambda);
    top_p->service_time = genExp(mew);
    top_p->execution_time = 0.0;
    top_p->restart_time = 0.0;
    top_p->completion_time = 0.0;
    top_p->remaining_time = top_p->service_time;
    top_p->next_p = 0; //NULL
    //initialize first event, set to arrival, and point to first process
    top_e = new event_queue;
    top_e->time = top_p->arrival_time;
    top_e->type = 1;
    top_e->next_e = 0; //NULL
    top_e->p_ptr = top_p;
    // initialize the rest of the processes
    while(process_counter < num_processes)
    {

        // just setting up the id for each process,
        process* new_p = top_p;
        //traverse the process list to the end
        while( new_p->next_p !=0 )
        {
            new_p = new_p->next_p;
        }
        new_p->next_p = new process;
        //new_p->id = process_counter;
        new_p->next_p->id = process_counter;
        //new_p->process_state = 1; // new state
        //new_p->arrival_time = new_p->arrival_time + genExp(lambda);
        new_p->next_p->arrival_time = new_p->arrival_time + genExp(lambda);
        //new_p->service_time = genExp(mew);
        new_p->next_p->service_time = genExp(mew);
        //new_p->execution_time = 0.0;
        new_p->next_p->execution_time = 0.0;
        //new_p->restart_time = 0.0;
        new_p->next_p->restart_time = 0.0;
        //new_p->completion_time = 0.0;
        new_p->next_p->completion_time = 0.0;
        //new_p->remaining_time = new_p->next_p->service_time;
        new_p->next_p->remaining_time = new_p->next_p->service_time;
        new_p->next_p->next_p = 0;
        //
        process_counter++;
    }
}
/**********************
   Schedule Events
***********************/
void schedule_event(int type, int counter)
{
    //ARRIVAL
    if (type == 1)
    {
        //match the process with the arrival
        process* temp_p = top_p;
        while(temp_p->id != counter)
        {
            temp_p = temp_p->next_p;
        }
        //create arrival event
        event_queue* arrival_e = new event_queue;
        arrival_e->time = temp_p->next_p->arrival_time;
        arrival_e->type = 1;
        arrival_e->p_ptr = temp_p->next_p;
        arrival_e->next_e = 0;
        insert_event_queue(arrival_e);
    }
    //DEPARTURE
    else if (type == 2)
    {
        event_queue* departure_e = new event_queue;
        departure_e->type = 2;
        departure_e->p_ptr = top_cpu->p_ptr;
        departure_e->next_e = 0;
        //find the departure time
        if(schedule_type == 1) //FCFS
        {
            departure_e->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;
        }
        else if (schedule_type == 2) //SRTF
        {
            //case 1
            if(top_cpu->p_ptr->restart_time == 0)
                departure_e->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;
            else
                departure_e->time = top_cpu->p_ptr->restart_time + top_cpu->p_ptr->remaining_time;
        }
        else if (schedule_type == 3) //HRRN
        {
            departure_e->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;

        }
        insert_event_queue(departure_e);
    }
    //ASSIGNMENT
    else if (type == 3)
    {
        //create assignment event, and allocate process to cpu
        event_queue assign_e = new event_queue;
        process* cpu_process;
        if(schedule_type == 1)
            cpu_process = top_r->p_ptr;
        else if(schedule_type == 2)
        {
            if ( top_cpu->clock > top_r->p_ptr->arrival_time)
            {
                //cpu_process = SRTFPROCESS
            }
            else
            {
                cpu_process = top_r->p_ptr;
            }
        }
        else if (schedule_type == 3)
        {
            //cpu_process = HRRNPROCESS
        }

        // set assignment event variables
        if(top_cpu->clock < cpu_process->arrival_time)
        {
            assign_e->time = cpu_process->arrival_time;
        }
        else
        {
            assign_e->time = top_cpu->clock;
        }
        assign_e->type = 3;
        assign_e->next_e = 0;
        assign_e->p_ptr = cpu_process;

        //insert into event queue
        insert_event_queue(assign_e);

    }
    //PREEMPTION
    //else if (type == 4)

}
/**********************
    Process Events
***********************/
void process_event(int type)
{

}

/**********************
       Schedulers
***********************/
void FCFS()
{
    int departure_count = 0;
    int arrival_counter = 0;
    while(departure_count < num_processes)
    {
        //if cpu is busy
        if(top_cpu->cpu_busy == true)
        {
            schedule_event(departure, arrival_counter);
        }
        //cpu is not busy
        else
        {
            schedule_event(arrival, arrival_counter);
            arrival_counter++;
            //check if ready queue has a process ready for cpu
            if(top_r != 0)
            {
                schedule_event(assign, arrival_counter);
            }
        }
        //check for events in event queue to process
        if (top_e->type == 1)
        {
            process_event(arrival);
        }
        else if (top_e->type == 2)
        {
            process_event(departure);
            departure_count++;
        }
        else if (top_e->type == 3)
        {
            process_event(assign);
        }

    }
}


//INSERT INTO EVENT QUEUE BASED ON TIME
void insert_event_queue(event_queue* event)
{
    if (top_e == 0)
        top_e = event;
    else if( top_e->time > event->time)
    {
        event->next_e = top_e;
        top_e = event;
    }
    else
    {
      event_queue* temp_e = top_e;
      while(temp_e != 0)
        {
             if((temp_e->time < event->time) && (temp_e->next_e == 0))
             {
                temp_e->next_e = event;
                break;
             }
            else if((temp_e->time < event->time) && (temp_e->next_e->time > event->time))
             {
                event->next_e = temp_e->next_e;
                temp_e->next_e = event;
                break;
             }
            else
             {
                temp_e = temp_e->next_e;
             }
       }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
float urand()
{
	return( (float) rand()/RAND_MAX );
}
float genExp(float lambda)
{
   float u,x;
   x = 0;
   while( x == 0 )
    {
      u = urand();
      x = (-1/lambda)*log(u);
    }
   return (x);
}
void run_sim()
{
    switch(schedule_type)
    {
    case 1:
        cout << "TESTING" << endl;
        FCFS();
        break;

    }
}
/////////////////////////////////////////////////////////////////////

int main()
{
    schedule_type = 1;
    init();
    cout << "TESTING" << endl;
    run_sim();
    cout << "TESTING" << endl;
    return 0;
}




/////////////////////////////////////////////////////////////////////

































