#include "header_sim.h"
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
    struct ready_queue* next_r; // points to next event in the ready queue

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
   //int id; //process id, will increment by 1
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
    GLOBAL VARIABLES
***********************/
int num_processes = 10; // 10,000 processes
int schedule_type; // 4 types
int lambda;        // avg arrival rate
float Ts;          // avg service time
float mew;         // avg service rate
float quantum;      //
float quantum_clock; //
//int arrival_counter; // counter matches up processes as they arrive
int process_counter;
//variables to set which event to schedule and process
int arrival = 1,
    departure = 2,
    assign_p = 3,
    preemption = 4;

/**********************
  Function Definitions
***********************/
void sort_event_queue(event_queue*);
float get_response_ratio_value(process*);
/**********************
      Initialize
***********************/
void init()
{
    schedule_type = 1;
    Ts = .06;
    lambda = 16;
    mew = float(1.0)/Ts;
    //process_counter = 0;
    //num_processes = 10;
    //create the cpu
    top_cpu = new cpu;
    top_cpu->clock = 0.0; // clock is at 0
    top_cpu->cpu_busy = false; // cpu is not busy at start
    top_cpu->p_ptr = 0; // no process in cpu at start


    // first process initialize, that top will always point too
    //which will have an id = 0
    top_p = new process;
    //top_p->id = process_counter;
    //top_p->process_state = 1; // new states
    top_p->arrival_time = genExp(float(lambda));
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
    /*
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
        new_p->next_p->id = process_counter;
        new_p->next_p->arrival_time = new_p->arrival_time + genExp(lambda);
        new_p->next_p->service_time = genExp(mew);
        new_p->next_p->execution_time = 0.0;
        new_p->next_p->restart_time = 0.0;
        new_p->next_p->completion_time = 0.0;
        new_p->next_p->remaining_time = new_p->next_p->service_time;
        new_p->next_p->next_p = 0;
        process_counter++;
    }*/
}
////////////////////////////////////////////////////////////////////////////
void run_sim()
{
    switch(schedule_type)
    {
            case 1:
                cout << "FCFS Simulation" << endl;
                FCFS();
                break;

            case 2:
                cout << "SRTF Simulation" << endl;
                SRTF();
                break;

            case 3:
                cout << "HRRN Simulation" << endl;
                HRRN();
                break;

            case 4:
                cout << "Round Robin Simulation" << endl;
                RR();
                break;

            default:
                cout << "ERROR" << endl;
    }
}
/**********************
       Schedulers
***********************/
void FCFS()
{
    int departure_count = 0;
    int arrival_count = 0;
    while(departure_count < num_processes)
    {
        //if cpu is busy
        if(top_cpu->cpu_busy == false)
        {
            schedule_event(arrival);
            arrival_count++;
            //check if ready queue has a process ready for cpu
            if(top_r != 0)
            {
                schedule_event(assign_p);
            }
        }
        //cpu is not busy
        else
        {
            schedule_event(departure);
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
            process_event(assign_p);
        }

    }


}
void SRTF()
{
    int arrival_count = 0;
    int departure_count = 0;
    while(departure_count < num_processes)
    {
        if(arrival_count < num_processes)
        {//schedule arrival
            schedule_event(arrival);
            arrival_count++;
        }
        // is cpu is not busy
        if(top_cpu->cpu_busy == false)
        {
            //check ready queue
            if(top_r != 0)
                schedule_event(assign_p);
        }
        else
            if(top_e->type == 1)
            {
                if(top_e->time > cpu_finish_time())
                {
                    schedule_event(departure);
                }
                else if( check_preemptive)
                {
                    schedule_event(preemption);
                }
            }
        // check for events in event queue to process
        if(top_e->type == 1 )
            schedule_event(arrival);
        else if (top_e->type == 2)
        {
            schedule_event(departure);
            departure_count++;
        }
        else if (top_e->type == 3)
            schedule_event(assign_p);
        else if (top_e->type == 4)
            schedule_event(preemption);
    }
}
void HRRN()
{

}
void RR()
{

}
/**********************
   Helper Functions
***********************/
float cpu_finish_time()
{
    float finish_time;
    float execution_time = top_cpu->p_ptr->execution_time;
    float restart_time = top_cpu->p_ptr->restart_time;
    float remaining_time = top_cpu->p_ptr->remaining_time;
    if(restart_time == 0)
        finish_time = execution_time + remaining_time;
    else
        finish_time = restart_time + remaining_time;
    return finish_time;
}
bool check_preemptive()
{
    //find the finish time of process in cpu
    float proccess_cpu_time = cpu_finish_time();
    //find remaining time if the process wasn't done, got preempted
    float cpu_remaining_time = proccess_cpu_time - top_e->time;
    //check if preemptive
    //if the process time at the top of the event queue is less than process in cpu
    //and its remaining time is less than cpu remaining time
    float total_time_event;
    float total_time_cpu;
    total_time_event = top_e->time + top_e->p_ptr->remaining_time;
    total_time_cpu = proccess_cpu_time + cpu_remaining_time;
    if (total_time_event < total_time_cpu)
        return true;
    else
        return false;
}
/**********************
   Schedule Events
***********************/
void schedule_event(int type)
{
    event_queue* scheduled_event = new event_queue;
    //ARRIVAL
    if (type == 1)
    {
        //match the process with the arrival
        process* new_p = top_p;
        while(new_p->next_p != 0)
        {
            new_p = new_p->next_p;
        }
        new_p->next_p = new process;
        //new_p->next_p->id = process_counter;
        new_p->next_p->arrival_time = new_p->arrival_time + genExp(lambda);
        new_p->next_p->service_time = genExp(mew);
        new_p->next_p->execution_time = 0.0;
        new_p->next_p->restart_time = 0.0;
        new_p->next_p->completion_time = 0.0;
        new_p->next_p->remaining_time = new_p->next_p->service_time;
        new_p->next_p->next_p = 0;
        process_counter++;

        /////////////////////////////////
        /*while(temp_p->id != counter)
        {
            temp_p = temp_p->next_p;
        }
        */
        //create arrival event
        //event_queue* arrival_e = new event_queue;
        scheduled_event->time = new_p->next_p->arrival_time;
        scheduled_event->type = 1;
        scheduled_event->p_ptr = new_p->next_p;
        scheduled_event->next_e = 0;
        //
        sort_event_queue(scheduled_event);
    }
    //DEPARTURE
    else if (type == 2)
    {
        //event_queue* departure_e = new event_queue;
        scheduled_event->type = 2;
        scheduled_event->p_ptr = top_cpu->p_ptr;
        scheduled_event->next_e = 0;
        //find the departure time
        if(schedule_type == 1) //FCFS
        {
            scheduled_event->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;
        }
        else if (schedule_type == 2) //SRTF
        {
            //case 1
            if(top_cpu->p_ptr->restart_time == 0)
                scheduled_event->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;
            else
                scheduled_event->time = top_cpu->p_ptr->restart_time + top_cpu->p_ptr->remaining_time;
        }
        else if (schedule_type == 3) //HRRN
        {
            scheduled_event->time = top_cpu->p_ptr->execution_time + top_cpu->p_ptr->remaining_time;

        }
        sort_event_queue(scheduled_event);
    }
    //ASSIGNMENT
    else if (type == 3)
    {
        //create assignment event, and allocate process to cpu
        //event_queue* scheduled_event = new event_queue;
        process* cpu_process;
        if(schedule_type == 1)
            cpu_process = top_r->p_ptr;
        else if(schedule_type == 2)
        {
            if ( top_cpu->clock > top_r->p_ptr->arrival_time)
            {
                //find the shortest remaining time first in the ready queue
                ready_queue* temp_r = top_r;
                cpu_process = temp_r->p_ptr;
                float srtf = temp_r->p_ptr->remaining_time;
                while(temp_r != 0)
                {
                    if(temp_r->p_ptr->remaining_time < srtf)
                    {
                        srtf = temp_r->p_ptr->remaining_time;
                        cpu_process = temp_r->p_ptr;
                    }
                    temp_r = temp_r->next_r;
                }
            }
            else
            {
                cpu_process = top_r->p_ptr;
            }
        }
        else if (schedule_type == 3)
        {
            ready_queue* temp_r = top_r;
            cpu_process = temp_r->p_ptr;
            //response ration value
            float hrrn = get_response_ratio_value(cpu_process);

            while( temp_r != 0)
                if(get_response_ratio_value(temp_r->p_ptr) > hrrn)
                {
                    hrrn = get_response_ratio_value(temp_r->p_ptr);
                    cpu_process = temp_r->p_ptr;
                }
                temp_r = temp_r->next_r;
            //cpu_process = HRRNPROCESS
        }

        // set assignment event variables
        if(top_cpu->clock < cpu_process->arrival_time)
        {
            scheduled_event->time = cpu_process->arrival_time;
        }
        else
        {
            scheduled_event->time = top_cpu->clock;
        }
        scheduled_event->type = 3;
        scheduled_event->next_e = 0;
        scheduled_event->p_ptr = cpu_process;

        //insert into event queue
        sort_event_queue(scheduled_event);

    }
    //PREEMPTION
    else if (type == 4)
    {
        scheduled_event->time = top_e->time;
        scheduled_event->type =  4;
        scheduled_event->next_e = 0;
        scheduled_event->p_ptr = top_e->p_ptr;
        //pop arrival event
        pop_event_queue();
        //insert preemption into event queue
        sort_event_queue(scheduled_event);
    }

}
/**********************
    Process Events
***********************/
void process_event(int type)
{
    //moving arrival from event queue to ready queue
    if (type == 1)     //process arrival
    {
        ready_queue* ready = new ready_queue;
        ready->p_ptr = top_e->p_ptr;
        ready->next_r = 0;

        // push process onto ready queue
        //check if its empty
        if(top_r == 0)
            top_r = ready;
        //or place at the end
        else
        {
            ready_queue* temp_r = top_r;
            //traverse the list
            while(temp_r->next_r !=0 )
            {
                temp_r = temp_r->next_r;
            }
            temp_r->next_r = ready;
        }
        // pop the arrival event from the ready queue
        pop_event_queue();
    }
    else if (type == 2) //process departure
    {
        //updating cpu node, no longer busy
        top_cpu->p_ptr->completion_time = top_e->time;
        top_cpu->p_ptr->remaining_time = 0.0;
        top_cpu->p_ptr = 0; // points to nada
        top_cpu->clock = top_e->time;
        top_cpu->cpu_busy = false;
        //PLACE TOTAL THROUGHPUT AND TURNAROUND TIME HERE
        //total_TA = total_TA + (top_cpu->p_ptr->completion_time - top_cpu->p_ptr->arrival_time);
        pop_event_queue();
    }
    else if (type == 3) //process assignment
    {
        top_cpu->p_ptr = top_e->p_ptr;
        pop_ready_queue();
        pop_event_queue();

        top_cpu->cpu_busy = true;
        //update the simulator clock
        if( top_cpu->clock < top_cpu->p_ptr->arrival_time )
        {
            top_cpu->clock = top_cpu->p_ptr->arrival_time;
        }
        //if a process is restarting update, and update execution times
        if( top_cpu->p_ptr->execution_time == 0 )
        {
            top_cpu->p_ptr->execution_time = top_cpu->clock;
        }
        else
        {
            top_cpu->p_ptr->restart_time = top_cpu->clock;
        }
    }
    else if (type == 4) //process preemption
    {
        //create a temporary pointer to hold whats in cpu
        process* temp_ptr = top_cpu->p_ptr;
        //update the remaining time
        top_cpu->p_ptr->remaining_time = cpu_finish_time() - top_e->time;
        //re-allocate cpu to preempting process
        top_cpu->p_ptr = top_e->p_ptr;
        top_cpu->clock = top_e->time;
        if(top_cpu->p_ptr->restart_time == 0)
            top_cpu->p_ptr->execution_time = top_e->time;
        else
            top_cpu->p_ptr->restart_time = top_e->time;
        // create a arrival event for the interrupting process
        event_queue* preemptive_arrival = new event_queue;
        preemptive_arrival->time = top_e->time;
        preemptive_arrival->type = 1;
        preemptive_arrival->next_e = 0;
        preemptive_arrival->p_ptr = temp_ptr;
        //pop preemption from event queue
        pop_event_queue();
        //insert into the new event into event queue
        sort_event_queue(preemptive_arrival);
    }
}
/**********************
 Queue Sort and Pop
***********************/
//SORTING AND POPPING FUNCTIONS
//INSERT INTO EVENT QUEUE BASED ON TIME
void sort_event_queue(event_queue* event)
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
void pop_event_queue()
{
    event_queue* temp_ptr = top_e;
    top_e = top_e->next_e;
    delete temp_ptr;
}
void pop_ready_queue()
{
    ready_queue* temp_ptr = top_r;
    top_r = top_r->next_r;
    delete temp_ptr;
}
float get_response_ratio_value(process* process_hrrn)
{
    float ratio_hrrn;
    float w = top_cpu->clock - process_hrrn->arrival_time;
    float s = process_hrrn->service_time;
    ratio_hrrn = (w + s) / s;
    return ratio_hrrn;
}
////////////////////////////////////////////////////////////////////////////////////////
/**********************
 Computation Functions
***********************/
float avg_TA()
{
    float total_turn_around = 0.0;
    process* temp_ptr = top_p;
    while(temp_ptr->completion_time != 0)
    {
        float ct = temp_ptr->completion_time;
        float at = temp_ptr->arrival_time;
        total_turn_around = total_turn_around + (ct - at);
        temp_ptr = temp_ptr->next_p;
    }
    float avg;
    avg = total_turn_around/num_processes;
    return avg;
}
float avg_throughput()
{
    // need to find the time it takes to run/complete
    float total_time = 0.0;
    process* temp_ptr = top_p;
        while(temp_ptr->completion_time != 0)
        {
            if (temp_ptr->next_p->completion_time == 0)
                total_time = temp_ptr->completion_time;
            temp_ptr = temp_ptr->next_p;
        }
    float throughput;
    throughput = ((float)num_processes /total_time);
    return throughput;
}
float cpu_util()
{
    process* temp_ptr = top_p;
    float total_time = 0.0;
    float cpu_busy_time = 0.0;
    while(temp_ptr->completion_time != 0)
    {
        cpu_busy_time = cpu_busy_time + temp_ptr->service_time;
        if(temp_ptr->next_p->completion_time == 0)
        {
            total_time = temp_ptr->completion_time;
        }
        temp_ptr = temp_ptr->next_p;
    }
    float rho;
    rho = cpu_busy_time/total_time;
    return rho;
}
float avg_processes_in_readyQ()
{
   float timeNmin1 = 0.0;
   process* temp_ptr = top_p;
   while( temp_ptr->completion_time != 0 )
    {
      if( temp_ptr->next_p->completion_time == 0 )
         timeNmin1 = temp_ptr->completion_time;
      temp_ptr = temp_ptr->next_p;
    }
    int timeN = static_cast<int>(timeNmin1) + 1;

   temp_ptr = top_p;
   int time = 0;;
   int num_processes_Q = 0;
   for( time = 0; time < timeN; time++ ){
      while( temp_ptr->completion_time != 0 )
        {
         if((temp_ptr->arrival_time < time && temp_ptr->execution_time > time ) ||
             (temp_ptr->arrival_time > time && temp_ptr->arrival_time < (time + 1)))
             {
                num_processes_Q++;
             }
         temp_ptr = temp_ptr->next_p;
        }
     temp_ptr = top_p;
   }
   float avg_processes_Q;
   avg_processes_Q = (float)num_processes_Q / timeN;
   return avg_processes_Q;
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


void generate_report()
{
    cout << "Testing GENERATE REPORT" << endl;
    float util, avgTA, throughput, num_proc;
    util = cpu_util();
    avgTA = avg_TA();
    throughput = avg_throughput();
    num_proc = avg_processes_in_readyQ();\
    cout << util <<  endl;
    cout << avgTA << endl;
    cout << throughput << endl;
    cout << num_proc << endl;
}
/////////////////////////////////////////////////////////////////////

int main()
{
    init();
    run_sim();
    generate_report();
    return 0;
}




/////////////////////////////////////////////////////////////////////

               // cout << "Testing" << endl;




















