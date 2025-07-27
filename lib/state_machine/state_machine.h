#pragma once

#include <BLE2902.h>
#include <GlobalVars.h>
#include <mem_sys.h>

#define HEAT_PIN 10
#define MAX_PUFFS 10
#define LOCKDOWN_PERIOD (10UL)
enum state_t {WAIT, PUFF_COUNTING, LOCKDOWN}; 

void init_state_machine(); 
void handle_state(); 

// extern state_t current_state; 

struct FsmData {
    state_t current_state;
    int puff_count;
    long puff_start_time;
    long puff_duration;
    long lockdown_start_time;
    long lockdown_duration;
} __attribute__((packed));  // ensure no padding

class fsm{
    public:
        fsm();
        ~fsm(); 
        static void handle_state_rising(); 
        static void handle_state_falling(); 
        
    private:
        state_t RTC_DATA_ATTR static current_state; 
        volatile RTC_DATA_ATTR static int puff_count; 
        volatile RTC_DATA_ATTR static long puff_start_time; 
        volatile RTC_DATA_ATTR static long puff_duration; 

        volatile RTC_DATA_ATTR static long lockdown_start_time; 
        volatile RTC_DATA_ATTR static long lockdown_duration; 


        static void pack_fsm_data(FsmData *data, state_t current_state, int puff_count, 
                           long puff_start_time, long puff_duration, long lockdown_start_time, 
                           long lockdown_duration); 
        
        static void send_fsm_data(FsmData *data); 
}; 