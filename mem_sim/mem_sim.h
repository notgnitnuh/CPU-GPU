#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <vector>
#include <iomanip>
#include <math.h>

using namespace std;

// Global Settings
size_t system_timeout = 100000;
size_t max_memory = 30000;

// Custom Types
enum policy_ID{VSP = 1, PAG = 2, SEG = 3};
enum fit_algorithm{FF = 1, BF = 2, WF = 3};
struct process{
    size_t ID;
    size_t arrival_t;
    size_t lifetime;
    size_t end_t = system_timeout; 
    size_t next_event; 
    size_t num_addrs; 
    size_t addrs_total=0; 
    vector<size_t> addrs;
};
struct mem_block{
    size_t start;
    size_t end;
    size_t part; 
    size_t ID=0;
};

// Function Definitions
bool BestFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID, const size_t& part);
bool FirstFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID, const size_t& part);
bool OutputPrompt(size_t& mem_size, size_t& mm_policy, size_t& mm_param, string& file_name);
bool ReadWorkloadFile(const string& filename, size_t& num_proccess, vector<process>& processes);
bool RunProcesses(const size_t& mem_size, const size_t &mm_param, const size_t& num_procs, const size_t& mm_policy, vector<process>& procs);
bool StringToInt(const string& str, size_t& int_val);
bool WorstFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID, const size_t& part);
void CheckArrivals(const size_t& num_procs, const size_t& sys_clock, const vector<process> &procs, vector<process>& queue);
void CheckCompletion( const vector<process> &procs, const size_t& sys_clock, const size_t& num_procs, vector<mem_block>& memory, const size_t& mm_policy);
void CleanOther(const process& proc, vector<mem_block>& memory);
void CleanPages(const process& proc, vector<mem_block>& memory);
void PrintQueue(const vector<process>& queue);
void PrintMemMap(const vector<mem_block>& memory, const size_t& mm_policy);
void PrintTurnaround(const vector<process>& procs);
void StorePAG(vector<mem_block>& memory, const size_t &page_size, vector<process>& queue, vector<process>& procs, const size_t& sys_clock);
void StoreSEG(vector<mem_block>& memory, const size_t &fit_alg, vector<process>& queue, vector<process>& procs, const size_t& sys_clock);
void StoreVSP(vector<mem_block>& memory, const size_t &fit_alg, vector<process>& queue, vector<process>& procs, const size_t& sys_clock);