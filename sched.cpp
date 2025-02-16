#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <queue>
#include <deque>
#include <iomanip>
#include <getopt.h>
using namespace std;

class Process {
public:

    int ID;
    int AT;
    int TC;
    int CB;
    int IO;
    int FT;
    int TT;
    int IT;
    int CW;
    int cpu_burst;
    int io_burst;
    int remaining_execution_time;
    int static_priority;
    int dynamic_priority;
    int state_ts;
    
    Process(int ID, int AT, int TC, int CB, int IO, int static_priority)
    {
        this->ID = ID;
        this->AT = AT;
        this->TC = TC;
        this->CB = CB;
        this->IO = IO;
        this->FT = 0;
        this->TT = 0;
        this->IT = 0;
        this->CW = 0;
        this->cpu_burst = 0;
        this->remaining_execution_time = TC;
        this->static_priority = static_priority;
        this->dynamic_priority = static_priority - 1;
        this->state_ts = AT;       
    }
};

class Scheduler {
public:
    deque<Process*> run_queue;
    virtual void add_process(Process* process) = 0;
    virtual Process* get_next_process() = 0;
    virtual ~Scheduler() {}
};

class FCFS_Scheduler : public Scheduler {
public:
    void add_process(Process* process) override
    {
        run_queue.push_back(process);
    }
    Process* get_next_process() override
    {
        if(run_queue.empty())
        {
            return nullptr;
        }
        Process* next_process = run_queue.front();
        run_queue.pop_front();
        return next_process;
    }
};

class LCFS_Scheduler : public Scheduler {
public:
    void add_process(Process* process) override
    {
        run_queue.push_front(process);
    }
    Process* get_next_process() override
    {
        if(run_queue.empty())
        {
            return nullptr;
        }
        Process* next_process = run_queue.front();
        run_queue.pop_front();
        return next_process;
    }
};

class SRTF_Scheduler : public Scheduler {
public:
    void add_process(Process* process) override
    {
        auto it = run_queue.begin();
        while (it != run_queue.end() && (*it)->remaining_execution_time <= process->remaining_execution_time)
        {
            ++it;
        }
        run_queue.insert(it, process);
    }
    Process* get_next_process() override
    {
        if(run_queue.empty())
        {
            return nullptr;
        }
        Process* next_process = run_queue.front();
        run_queue.pop_front();
        return next_process;
    }
};

class RoundRobin_Scheduler : public Scheduler {
public:
    void add_process(Process* process) override 
    {
        run_queue.push_back(process);
    }
    Process* get_next_process() override 
    {
        if(run_queue.empty()) 
        {
            return nullptr;
        }
        Process* next_process = run_queue.front();
        run_queue.pop_front();
        return next_process;
    }
};

class PRIO_Scheduler : public Scheduler {
public:
    int maxprio;
    deque<Process*>* activeQ;
    deque<Process*>* expiredQ;
    PRIO_Scheduler(int maxprios)
    {
        this->maxprio = maxprios;
        activeQ = new deque<Process*>[this->maxprio];
        expiredQ = new deque<Process*>[this->maxprio];
    }
    void add_process(Process* process) override 
    {
        int current_dynamic_priority = process->dynamic_priority;
        if(current_dynamic_priority < 0)
        {
            process->dynamic_priority = process->static_priority - 1;
            expiredQ[process->dynamic_priority].push_back(process);
        }
        else
        {
            activeQ[current_dynamic_priority].push_back(process);
        }
    }
    Process* get_next_process() override 
    {
        for(int i = maxprio - 1; i >= 0; --i)
        {
            if(!activeQ[i].empty())
            {
                Process* next_process = activeQ[i].front();
                activeQ[i].pop_front();
                return next_process;
            }
        }
        swap(activeQ, expiredQ);
        for(int i = maxprio - 1; i >= 0; --i)
        {
            if(!activeQ[i].empty())
            {
                Process* next_process = activeQ[i].front();
                activeQ[i].pop_front();
                return next_process;
            }
        }
        return nullptr;
    }
};

class PREPRIO_Scheduler : public Scheduler {
public:
    int maxprio;
    deque<Process*>* activeQ;
    deque<Process*>* expiredQ;
    PREPRIO_Scheduler(int maxprios)
    {
        this->maxprio = maxprios;
        activeQ = new deque<Process*>[this->maxprio];
        expiredQ = new deque<Process*>[this->maxprio];
    }
    void add_process(Process* process) override 
    {
        int current_dynamic_priority = process->dynamic_priority;
        if(current_dynamic_priority < 0)
        {
            process->dynamic_priority = process->static_priority - 1;
            expiredQ[process->dynamic_priority].push_back(process);
        }
        else
        {
            activeQ[current_dynamic_priority].push_back(process);
        }
    }
    Process* get_next_process() override 
    {
        for(int i = maxprio - 1; i >= 0; --i)
        {
            if(!activeQ[i].empty())
            {
                Process* next_process = activeQ[i].front();
                activeQ[i].pop_front();
                return next_process;
            }
        }
        swap(activeQ, expiredQ);
        for(int i = maxprio - 1; i >= 0; --i)
        {
            if(!activeQ[i].empty())
            {
                Process* next_process = activeQ[i].front();
                activeQ[i].pop_front();
                return next_process;
            }
        }
        return nullptr;
    }
};

int ofs;
int randvals_length;
vector<int> randvals;

int quantum = 10000;
int maxprios = 4;

string scheduler_choice = "";
Scheduler* scheduler;

bool verbose = false;

enum SchedulerType {
    FCFS, LCFS, SRTF, ROUND_ROBIN, PRIO, PREPRIO
};

SchedulerType schedulerType;

enum State {
    FINISHED,
    TRANS_TO_READY_FROM_CREATED,
    TRANS_TO_RUN_FROM_READY,
    TRANS_TO_BLOCK_FROM_RUN,
    TRANS_TO_READY_FROM_BLOCK,
    TRANS_TO_READY_FROM_RUN,
};

class Event {
public:
    int timestamp;
    Process* process;
    State state;
    Event(int timestamp, Process* process, State state)
    {
        this->timestamp = timestamp;
        this->process = process;
        this->state = state;
    }
};

vector<Process*> processes;
deque<Event*> eventQ;

void insertEvent(Event* e)
{
    int insertion_index = -1;
    for (int i = 0; i < (int) eventQ.size(); i++)
    {
        if (e->timestamp < eventQ[i]->timestamp) 
        {
            insertion_index = i;
            break;
        }
    }
    if(insertion_index == -1)
    {
        insertion_index = (int) eventQ.size();
    }
    eventQ.insert(eventQ.begin() + insertion_index, e);
}

int myrandom(int burst)
{
    if(ofs == randvals_length)
    {
        ofs = 0;
    }
    int random_number = randvals[ofs];
    ofs += 1;
    return 1 + (random_number % burst);
}

void readRandFile(const string& randfile)
{
    ifstream randfile_stream(randfile);
    if(!randfile_stream)
    {
        cerr << "Error Opening randfile." << endl;
        exit(1);
    }
    randfile_stream >> randvals_length;
    int value;
    while(randfile_stream >> value)
    {
        randvals.push_back(value);
    }
    randfile_stream.close();
}

void readInputFile(const string& inputFile)
{
    ifstream inputfile_stream(inputFile);
    if(!inputfile_stream)
    {
        cerr << "Error Opening inputFile." << endl;
        exit(1);
    }
    int ID = 0;
    int AT, TC, CB, IO;
    while(inputfile_stream >> AT >> TC >> CB >> IO)
    {
        int static_priority = myrandom(maxprios);
        Process* process = new Process(ID++, AT, TC, CB, IO, static_priority);
        processes.push_back(process);
        Event* event = new Event(AT, process, TRANS_TO_READY_FROM_CREATED);
        insertEvent(event);
    }
    inputfile_stream.close();
}

void printVerbose(int sim_time, Process* process, State state, int timeInPrevState) 
{
    cout << sim_time << " " << process->ID << " " << timeInPrevState << ": ";
    switch(state) 
    {
        case TRANS_TO_READY_FROM_CREATED:
            cout << "CREATED -> READY" << endl;
            break;
        case TRANS_TO_RUN_FROM_READY:
            cout << "READY -> RUNNG cb=" << process->cpu_burst 
                 << " rem=" << process->remaining_execution_time 
                 << " prio=" << process->dynamic_priority << endl;
            break;
        case TRANS_TO_BLOCK_FROM_RUN:
            cout << "RUNNG -> BLOCK ib=" << process->io_burst 
                 << " rem=" << process->remaining_execution_time << endl;
            break;
        case TRANS_TO_READY_FROM_BLOCK:
            cout << "BLOCK -> READY" << endl;
            break;
        case TRANS_TO_READY_FROM_RUN:
            cout << "RUNNG -> READY cb=" << process->cpu_burst 
                 << " rem=" << process->remaining_execution_time 
                 << " prio=" << process->dynamic_priority << endl;
            break;
        case FINISHED:
            cout << "DONE" << endl;
            break;
        default:
            break;
    }
}

void parseSchedulerChoice()
{
    char type = scheduler_choice[0];
    switch(type)
    {
        case 'F':
            schedulerType = FCFS;
            scheduler = new FCFS_Scheduler();
            break;
        case 'L':
            schedulerType = LCFS;
            scheduler = new LCFS_Scheduler();
            break;
        case 'S':
            schedulerType = SRTF;
            scheduler = new SRTF_Scheduler();
            break;
        case 'R': {
            schedulerType = ROUND_ROBIN;
            sscanf(scheduler_choice.c_str() + 1, "%d", &quantum);
            scheduler = new RoundRobin_Scheduler();}
            break;
        case 'P': {
            schedulerType = PRIO;
            sscanf(scheduler_choice.c_str() + 1, "%d:%d", &quantum, &maxprios);
            scheduler = new PRIO_Scheduler(maxprios);}
            break;
        case 'E': {
            schedulerType = PREPRIO;
            sscanf(scheduler_choice.c_str() + 1, "%d:%d", &quantum, &maxprios);
            scheduler = new PREPRIO_Scheduler(maxprios);}
            break;
        default:
            cerr << "Invalid Scheduler Choice!" << endl;
            exit(1);
    }
    switch(schedulerType)
    {
        case FCFS:
            cout << "FCFS" << endl;
            break;
        case LCFS:
            cout << "LCFS" << endl;
            break;
        case SRTF:
            cout << "SRTF" << endl;
            break;
        case ROUND_ROBIN:
            cout << "RR " << quantum << endl;
            break;
        case PRIO:
            cout << "PRIO " << quantum << endl;
            break;
        case PREPRIO:
            cout << "PREPRIO " << quantum << endl;
            break;
        default:
            cerr << "Impossible to Reach This Error!" << endl;
            exit(1);
    }
}

void output_info(vector<Process*> processes, int sim_time, int cpu_busy_time, int io_busy_time)
{
    for(auto process: processes)
    {
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
               process->ID,
               process->AT,
               process->TC,
               process->CB,
               process->IO,
               process->static_priority,
               process->FT,
               process->TT,
               process->IT,
               process->CW);
    }
    int num_processes = (int) processes.size();
    double cpu_utilization = (double)cpu_busy_time / sim_time * 100.0;
    double io_utilization = (double)io_busy_time / sim_time * 100.0;
    double total_tt = 0;
    double total_cw = 0;
    for(auto process: processes)
    {
        total_tt += process->TT;
        total_cw += process->CW;
    }
    double avg_tt = total_tt / num_processes;
    double avg_cw = total_cw / num_processes;
    double throughput = (double)num_processes / sim_time * 100.0;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
           sim_time,
           cpu_utilization,
           io_utilization,
           avg_tt,
           avg_cw,
           throughput);
}

void handlePreemption(int prem_time, Process* process_to_preempt)
{
    int index_val = 0;
    int events_length = (int) eventQ.size();
    while(index_val < events_length && eventQ[index_val]->process->ID != process_to_preempt->ID)
    {
        index_val += 1;
    }
    if(index_val < events_length)
    {
        eventQ.erase(eventQ.begin() + index_val);
    }
    insertEvent(new Event(prem_time, process_to_preempt, TRANS_TO_READY_FROM_RUN));
}

void simulation()
{
    int sim_time = 0;
    int cpu_busy_time = 0;
    int io_busy_time = 0;
    int io_active_ending = 0;
    bool CALL_SCHEDULER = false;
    Process* current_running_process = nullptr;

    while(!eventQ.empty())
    {
        Event* current_event = eventQ.front();
        eventQ.pop_front();

        Process* process = current_event->process;
        sim_time = current_event->timestamp;
        State state = current_event->state;

        int timeInPrevState = sim_time - process->state_ts;
        process->state_ts = sim_time;

        delete current_event;
        current_event = nullptr;

        switch(state)
        {
            case TRANS_TO_READY_FROM_CREATED:
            {
                CALL_SCHEDULER = true;

                if(schedulerType == PREPRIO && current_running_process != nullptr)
                {
                    if(process->dynamic_priority > current_running_process->dynamic_priority)
                    {
                        bool has_pending_event = false;
                        for (const auto& e : eventQ)
                        {
                            if (e->process->ID == current_running_process->ID && e->timestamp == sim_time)
                            {
                                has_pending_event = true;
                                break;
                            }
                        }
                        if (!has_pending_event)
                        {
                            int time_it_actually_ran = sim_time - current_running_process->state_ts;
                            int how_long_it_should_have_run = min(current_running_process->cpu_burst, quantum);
                            if(current_running_process->remaining_execution_time < how_long_it_should_have_run)
                            {
                                how_long_it_should_have_run = current_running_process->remaining_execution_time;
                            }
                            cpu_busy_time -= (how_long_it_should_have_run - time_it_actually_ran);
                            handlePreemption(sim_time, current_running_process);
                        }
                    }
                }

                scheduler->add_process(process);
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;

            case TRANS_TO_RUN_FROM_READY:
            {
                process->CW += timeInPrevState;
                
                if(process->cpu_burst == 0)
                {
                    process->cpu_burst = min(myrandom(process->CB), process->remaining_execution_time);
                }

                int how_long_it_can_run = min(process->cpu_burst, quantum);
                
                current_running_process = process;

                if(process->remaining_execution_time <= how_long_it_can_run)
                {
                    cpu_busy_time += process->remaining_execution_time;
                    insertEvent(new Event(sim_time + process->remaining_execution_time, process, FINISHED));
                }

                else if(how_long_it_can_run == process->cpu_burst)
                {
                    cpu_busy_time += process->cpu_burst;
                    insertEvent(new Event(sim_time + process->cpu_burst, process, TRANS_TO_BLOCK_FROM_RUN));
                }

                else if(how_long_it_can_run == quantum)
                {
                    cpu_busy_time += quantum;
                    handlePreemption(sim_time + quantum, current_running_process);
                }

                CALL_SCHEDULER = false;
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;

            case TRANS_TO_BLOCK_FROM_RUN:
            {
                process->remaining_execution_time -= process->cpu_burst;
                process->cpu_burst = 0;
                process->io_burst = myrandom(process->IO);
                process->IT += process->io_burst;

                if(sim_time > io_active_ending)
                {
                    io_active_ending = sim_time + process->io_burst;
                    io_busy_time += process->io_burst;
                }
                else
                {
                    int overlap_io_time = sim_time + process->io_burst - io_active_ending;
                    io_busy_time += max(0, overlap_io_time);
                    io_active_ending = max(io_active_ending, sim_time + process->io_burst);
                }

                insertEvent(new Event(sim_time + process->io_burst, process, TRANS_TO_READY_FROM_BLOCK));
                CALL_SCHEDULER = true;
                current_running_process = nullptr;
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;

            case TRANS_TO_READY_FROM_BLOCK:
            {

                process->dynamic_priority = process->static_priority - 1;
                CALL_SCHEDULER = true;

                if(schedulerType == PREPRIO && current_running_process != nullptr)
                {
                    if(process->dynamic_priority > current_running_process->dynamic_priority)
                    {
                        bool has_pending_event = false;
                        for (const auto& e : eventQ)
                        {
                            if (e->process->ID == current_running_process->ID && e->timestamp == sim_time)
                            {
                                has_pending_event = true;
                                break;
                            }
                        }
                        if (!has_pending_event)
                        {
                            int time_it_actually_ran = sim_time - current_running_process->state_ts;
                            int how_long_it_should_have_run = min(current_running_process->cpu_burst, quantum);
                            if(current_running_process->remaining_execution_time < how_long_it_should_have_run)
                            {
                                how_long_it_should_have_run = current_running_process->remaining_execution_time;
                            }
                            cpu_busy_time -= (how_long_it_should_have_run - time_it_actually_ran);
                            handlePreemption(sim_time, current_running_process);
                        }
                    }
                }
                
                scheduler->add_process(process);
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;

            case TRANS_TO_READY_FROM_RUN:
            {
                int cpu_time_used = timeInPrevState;
                process->cpu_burst -= cpu_time_used;
                process->remaining_execution_time -= cpu_time_used;
                if(process->dynamic_priority >= 0)
                {
                    process->dynamic_priority--;
                }
                scheduler->add_process(process);
                CALL_SCHEDULER = true;
                current_running_process = nullptr;
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;

            case FINISHED:
            {
                process->FT = sim_time;
                process->TT = process->FT - process->AT;
                current_running_process = nullptr;
                CALL_SCHEDULER = true;
                if (verbose) printVerbose(sim_time, process, state, timeInPrevState);
            }
                break;
        }
        if(CALL_SCHEDULER)
        {
            if(!eventQ.empty() && eventQ.front()->timestamp == sim_time)
            {
                continue;
            }
            CALL_SCHEDULER = false;
            if(current_running_process == nullptr)
            {
                current_running_process = scheduler->get_next_process();
                if(current_running_process != nullptr)
                {
                    insertEvent(new Event(sim_time, current_running_process, TRANS_TO_RUN_FROM_READY));
                }
            }
        }
    }
    output_info(processes, sim_time, cpu_busy_time, io_busy_time);
}

int main(int argc, char* argv[])
{
    char option;
    while((option = getopt(argc, argv, "hvs:")) != -1)
    {
        switch(option)
        {
            case 'v':
                verbose = true;
                break;
            case 's':
                scheduler_choice = optarg;
                break;
            case 'h':
                cout << "Usage: " << argv[0] << " [-h] [-v] [-s schedspec] inputfile randfile" << endl;
                exit(0);
            case '?':
                if(optopt == 's')
                {
                    cerr << "Option -" << optopt << " Requires an Argument!" << endl;
                }
                else if(isprint(optopt))
                {
                    cerr << "Unknown Option `-" << optopt << "`." << endl;
                }
                else
                {
                    cerr << "Unknown Option Character `" << optopt << "`." << endl;
                }
                return 1;
            default:
                abort();
        }
    }

    if(optind + 2 != argc)
    {
        cerr << "Missing inputfile or randfile!" << endl;
        exit(1);
    }

    string inputfile = argv[optind];
    string randfile = argv[optind + 1];

    if (!scheduler_choice.empty())
    {
        parseSchedulerChoice();
    }

    readRandFile(randfile);
    readInputFile(inputfile);

    simulation();

    return 0;
}