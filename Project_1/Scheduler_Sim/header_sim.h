

#ifndef HEADER_SIM_H
#define HEADER_SIM_H
void init();
void run_sim();
void generate_report();
//Scheduler prototypes
void FCFS();
void SRTF();
void HRRN();
void RR();
//
void schedule_event(int type);
void process_event(int type);
void scheudle_rr();
void process_rr();
void pop_ready_queue();
void pop_event_queue();
//Utility prototypes
bool check_preemptive();
float cpu_finish_time();
float urand();
float genExp(float);
//computation prototypes
float avg_TA();
float avg_throughput();
float cpu_util();
float avg_processes_in_readyQ();

#endif
