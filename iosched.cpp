#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <climits>
#include <queue>
#include <deque>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <getopt.h>
#define endl "\n"
using namespace std;

string input_file;
string algo = "FIFO";
bool is_option_v = false;
bool is_option_q = false;
bool is_option_f = false;

int count_io_requests;

int head = 0;
int seek_direction = 1;

class BIO {
public:

    int ID;

    int AT;
    int TRACK;
    
    int SERVICE_START;
    int SERVICE_END;

    int TURNAROUND_TIME;
    int WAIT_TIME;

    BIO(int ID, int AT, int TRACK)
    {
        this->ID = ID;
        this->AT = AT;
        this->TRACK = TRACK;
        this->SERVICE_START = -1;
        this->SERVICE_END = -1;
        this->TURNAROUND_TIME = 0;
        this->WAIT_TIME = 0;
    }
};

vector<BIO*> bios;

class Scheduler {
public:
    deque<BIO*> io_queue;
    virtual void add_bio(BIO* bio) = 0;
    virtual BIO* get_next_bio() = 0;
    virtual ~Scheduler() {}
};

class FIFO_Scheduler : public Scheduler {
public:
    void add_bio(BIO* bio) override
    {
        io_queue.push_back(bio);
    }
    BIO* get_next_bio() override
    {
        if(io_queue.empty())
        {
            return nullptr;
        }
        BIO* next_bio = io_queue.front();
        io_queue.pop_front();
        return next_bio;
    }
};

class SSTF_Scheduler : public Scheduler {
public:
    void add_bio(BIO* bio) override
    {
        io_queue.push_back(bio);
    }
    BIO* get_next_bio() override
    {
        if(io_queue.empty())
        {
            return nullptr;
        }
        
        auto closest_iterator = io_queue.begin();
        int min_distance = abs((*closest_iterator)->TRACK - head);

        for(auto it = io_queue.begin(); it != io_queue.end(); ++it)
        {
            int distance = abs((*it)->TRACK - head);
            if(distance < min_distance)
            {
                min_distance = distance;
                closest_iterator = it;
            }
        }

        BIO* closest_bio = *closest_iterator;
        io_queue.erase(closest_iterator);
        return closest_bio;
    }
};

class LOOK_Scheduler : public Scheduler {
public:
    void add_bio(BIO* bio) override
    {
        io_queue.push_back(bio);
    }
    BIO* get_next_bio() override
    {
        if(io_queue.empty())
        {
            return nullptr;
        }

        auto closest_iterator = io_queue.end();
        int min_distance = INT_MAX;

        for(auto it = io_queue.begin(); it != io_queue.end(); ++it)
        {
            int distance = (*it)->TRACK - head;
            if((seek_direction == 1 && distance >= 0) || (seek_direction == -1 && distance <= 0))
            {
                distance = abs(distance);
                if(distance < min_distance)
                {
                    min_distance = distance;
                    closest_iterator = it;
                }
            }
        }

        if(closest_iterator == io_queue.end())
        {
            seek_direction *= -1;
            return get_next_bio();
        }

        BIO* closest_bio = *closest_iterator;
        io_queue.erase(closest_iterator);
        return closest_bio;
    }
};

class CLOOK_Scheduler : public Scheduler {
public:
    void add_bio(BIO* bio) override
    {
        io_queue.push_back(bio);
    }
    BIO* get_next_bio() override
    {
        if(io_queue.empty())
        {
            return nullptr;
        }

        BIO* closest_bio = nullptr;
        int min_distance = INT_MAX;

        for(auto bio : io_queue)
        {
            if(bio->TRACK >= head)
            {
                int distance = bio->TRACK - head;
                if(distance < min_distance)
                {
                    min_distance = distance;
                    closest_bio = bio;
                }
            }
        }

        if(closest_bio == nullptr)
        {
            min_distance = INT_MAX;
            for(auto bio : io_queue)
            {
                int distance = bio->TRACK;
                if(distance < min_distance)
                {
                    min_distance = distance;
                    closest_bio = bio;
                }
            }
        }

        auto it = find(io_queue.begin(), io_queue.end(), closest_bio);
        if(it != io_queue.end())
        {
            io_queue.erase(it);
        }

        return closest_bio;
    }
};

class FLOOK_Scheduler : public Scheduler {
public:
    deque<BIO*> active_queue;
    deque<BIO*> add_queue;
    void add_bio(BIO* bio) override
    {
        add_queue.push_back(bio);
    }
    BIO* get_next_bio() override
    {
        if(active_queue.empty())
        {
            active_queue.swap(add_queue);
            seek_direction = 1;
        }

        if(active_queue.empty())
        {
            return nullptr;
        }

        auto closest_iterator = active_queue.end();
        int min_distance = INT_MAX;

        for(auto it = active_queue.begin(); it != active_queue.end(); ++it)
        {
            int distance = (*it)->TRACK - head;
            if((seek_direction == 1 && distance >= 0) || (seek_direction == -1 && distance <= 0))
            {
                distance = abs(distance);
                if(distance < min_distance)
                {
                    min_distance = distance;
                    closest_iterator = it;
                }
            }
        }

        if(closest_iterator == active_queue.end())
        {
            seek_direction *= -1;
            return get_next_bio();
        }

        BIO* closest_bio = *closest_iterator;
        active_queue.erase(closest_iterator);
        return closest_bio;
    }
};

Scheduler* scheduler;

void parse_arguments(int argc, char* argv[])
{
    char option;
    while((option = getopt(argc, argv, "vqfs:")) != -1)
    {
        switch(option)
        {
            case 's':
                switch(optarg[0])
                {
                    case 'N':
                        algo = "FIFO";
                        break;
                    case 'S':
                        algo = "SSTF";
                        scheduler = new SSTF_Scheduler();
                        break;
                    case 'L':
                        algo = "LOOK";
                        scheduler = new LOOK_Scheduler();
                        break;
                    case 'C':
                        algo = "CLOOK";
                        scheduler = new CLOOK_Scheduler();
                        break;
                    case 'F':
                        algo = "FLOOK";
                        scheduler = new FLOOK_Scheduler();
                        break;
                    default:
                        break;
                }
                break;
            case 'v':
                is_option_v = true;
                break;
            case 'q':
                is_option_q = true;
                break;
            case 'f':
                is_option_f = true;
                break;
            default:
                break;
        }
    }
    if(optind + 1 != argc)
    {
        cerr << "Missing Input File!" << endl;
        exit(1);
    }
    input_file = argv[optind];
}

void process_inputs()
{
    string buffer;
    ifstream inputfile_stream(input_file);
    if(!inputfile_stream)
    {
        cerr << "Error Opening File: " << input_file << endl;
        exit(1);
    }
    int ID = 0;
    while(getline(inputfile_stream, buffer))
    {
        if(buffer[0] == '#')
        {
            continue;
        }
        int temp_AT;
        int temp_TRACK;
        sscanf(buffer.c_str(), "%d %d", &temp_AT, &temp_TRACK);
        BIO* bio = new BIO(ID++, temp_AT, temp_TRACK);
        bios.push_back(bio);
    }
    inputfile_stream.close();
    count_io_requests = ID;
}

void check_parsing(bool print)
{
    if(!print)
    {
        return;
    }
    cout << "\nInput File: " << input_file << "\n" << "\n";
    cout << "Algorithm: " << algo << "\n" << "\n";
    cout << "Number of IO Requests: " << count_io_requests << "\n" << "\n";
    for(auto bio : bios)
    {
        cout << bio->ID << " " << bio->AT << " " << bio->TRACK << " " << bio->SERVICE_START << " " << bio->SERVICE_END << " " << bio->TURNAROUND_TIME << " " << bio->WAIT_TIME << "\n";
    }
    cout << "\n";
}

void output_info(int total_time, int tot_movement, int time_io_was_busy)
{
    for(auto bio: bios)
    {
        printf("%5d: %5d %5d %5d\n", bio->ID, bio->AT, bio->SERVICE_START, bio->SERVICE_END);
    }
    int num_operations = (int) bios.size();
    double io_utilization = (double)time_io_was_busy / total_time;
    double total_tt = 0;
    double total_wt = 0;
    int max_waittime = INT_MIN;
    for(auto bio: bios)
    {
        total_tt += bio->TURNAROUND_TIME;
        total_wt += bio->WAIT_TIME;
        max_waittime = max(max_waittime, bio->WAIT_TIME);
    }
    double avg_turnaround = total_tt / num_operations;
    double avg_waittime = total_wt / num_operations;
    printf("SUM: %d %d %.4lf %.2lf %.2lf %d\n", total_time, tot_movement, io_utilization, avg_turnaround, avg_waittime, max_waittime);
}

int next_operation_index = 0;
BIO* check_next(int t)
{
    if(next_operation_index == count_io_requests || bios[next_operation_index]->AT != t)
    {
        return nullptr;
    }
    BIO* next_operation = bios[next_operation_index];
    next_operation_index += 1;
    return next_operation;
}

void simulation()
{
    int sim_time = 0;
    int io_busy_time = 0;
    int tot_movement = 0;

    int completed_io_requests = 0;

    BIO* current_running_bio = nullptr;

    while(true)
    {

        BIO* potentially_next_bio = check_next(sim_time);
        if(potentially_next_bio != nullptr)
        {
            scheduler->add_bio(potentially_next_bio);
        }
        
        if(current_running_bio != nullptr && head == current_running_bio->TRACK)
        {
            current_running_bio->SERVICE_END = sim_time;
            current_running_bio->TURNAROUND_TIME = sim_time - current_running_bio->AT;
            current_running_bio = nullptr;
            completed_io_requests += 1;
        }

        if(current_running_bio == nullptr)
        {
            BIO* next_bio_to_run = scheduler->get_next_bio();
            if(completed_io_requests != count_io_requests && next_bio_to_run != nullptr)
            {
                next_bio_to_run->SERVICE_START = sim_time;
                next_bio_to_run->WAIT_TIME = sim_time - next_bio_to_run->AT;
                current_running_bio = next_bio_to_run;

                int target = next_bio_to_run->TRACK;
                if(target > head)
                {
                    seek_direction = 1;
                }
                else if(target < head)
                {
                    seek_direction = -1;
                }
                else
                {
                    continue;
                }
            }
            else if(completed_io_requests == count_io_requests)
            {
                break;
            }
        }

        if(current_running_bio != nullptr)
        {
            head = head + seek_direction;
            tot_movement += 1;
            io_busy_time += 1;
        }

        sim_time += 1;
    }

    output_info(sim_time, tot_movement, io_busy_time);
}

int main(int argc, char* argv[])
{
    scheduler = new FIFO_Scheduler();

    parse_arguments(argc, argv);
    process_inputs();
    check_parsing(false);

    simulation();

    return 0;
}