#pragma once

#include <BLE2902.h>
#include <GlobalVars.h>
#include <mem_sys.h>

#define HEAT_PIN 10
#define MAX_PUFFS 10
#define LOCKDOWN_PERIOD (10UL)

#define MAX_PUFFS 10
#define NUM_PHASES 5

#define ONE_PUFF_DURATION 5

#define PHASE_DURATION 50

enum state_t {WAIT, PUFF_COUNTING, LOCKDOWN}; 

struct date_time{
    int year;
    int mins; 
    int secs; 
};

struct time_interval{
    unsigned long start; 
    unsigned long end; 
};

struct puffModel{
    int puffNumber; 
    date_time date; 
    unsigned long puff_duration; 
    int phaseIndex; 
    int id; 
};


// Shouldn't each phase should have a different LOCKDOWN_DURATION?
struct phaseModel{
    int phaseIndex; 
    unsigned long phase_duration; 
    unsigned long phase_time_start; 
    int maxPuffs; 
    struct puffModel puffs[MAX_PUFFS]; 
    int puffs_taken; 
    int id; 
};

struct ActivePhaseModel{
    int phase_index; 
    date_time phase_start_date; 
};


// extern state_t current_state; 

struct FsmData {
    state_t current_state;
    int puff_count;
    long puff_start_time;
    long puff_duration;
    long lockdown_start_time;
    long lockdown_duration;
} __attribute__((packed));  // ensure no padding

// class fsm{
//     public:
//         fsm();
//         ~fsm(); 
//         static void handle_state_rising(); 
//         static void handle_state_falling(); 
        
//     private:
//         state_t RTC_DATA_ATTR static current_state; 
//         volatile RTC_DATA_ATTR static int puff_count; 
//         volatile RTC_DATA_ATTR static long puff_start_time; 
//         volatile RTC_DATA_ATTR static long puff_duration; 

//         volatile RTC_DATA_ATTR static long lockdown_start_time; 
//         volatile RTC_DATA_ATTR static long lockdown_duration; 


//         static void pack_fsm_data(FsmData *data, state_t current_state, int puff_count, 
//                            long puff_start_time, long puff_duration, long lockdown_start_time, 
//                            long lockdown_duration); 
        
//         static void send_fsm_data(FsmData *data); 
// }; 


class fsm{
    public:
        fsm();
        ~fsm(); 
        void handle_state_rising(); 
        void handle_state_falling(); 
        
    private:
        struct phaseModel phases[NUM_PHASES]; 
        RTC_DATA_ATTR struct ActivePhaseModel curr_phase; 
        RTC_DATA_ATTR state_t current_state; 
        RTC_DATA_ATTR struct puffModel curr_puff; 
        RTC_DATA_ATTR struct phaseModel pm; 
        int puffNumber; 

        // Can overflow!! Maybe modulo with a big number?
        int counter; 
        int index; 

        static void pack_fsm_data(FsmData *data, state_t current_state, int puff_count, 
                           long puff_start_time, long puff_duration, long lockdown_start_time, 
                           long lockdown_duration); 
        
        static void send_fsm_data(FsmData *data); 
}; 