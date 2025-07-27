#include <state_machine.h>
#include <Arduino.h>


RTC_DATA_ATTR state_t fsm::current_state = /* some initial value, e.g. */ WAIT;
RTC_DATA_ATTR volatile int fsm::puff_count = 0;
RTC_DATA_ATTR volatile long fsm::puff_start_time = 0;
RTC_DATA_ATTR volatile long fsm::puff_duration = 0;
RTC_DATA_ATTR volatile long fsm::lockdown_start_time = 0;
RTC_DATA_ATTR volatile long fsm::lockdown_duration = 0;

fsm::fsm(){
    
}

fsm::~fsm(){
    // Deconstructor logic
}

// Static sized unordered map based on memory

void fsm::handle_state_rising(){
    switch(current_state){
        case(WAIT):
            puff_count = 0; 
            puff_start_time = 0; 
            current_state = PUFF_COUNTING; 
            Serial.println("Channging state tp PUFF_COUNTING");
        break; 
        
        case(PUFF_COUNTING):
            if(puff_count >= MAX_PUFFS){
                current_state = LOCKDOWN; 
                lockdown_start_time = micros(); 
            }
            puff_start_time = micros(); 
        break; 

        case(LOCKDOWN):
            lockdown_duration = micros() - lockdown_start_time;
            if(lockdown_duration >= LOCKDOWN_PERIOD){
                current_state = PUFF_COUNTING; 
                puff_count = 0; 
                puff_start_time = 0; 
            }
        break; 
    }
}

void fsm::handle_state_falling(){
    switch(current_state){
        case(WAIT):
            // Can't be here
        break; 
        
        case(PUFF_COUNTING):
            puff_duration = micros() - puff_start_time; 
            puff_start_time = 0; 
            puff_count++;
            Serial.println("Increasing puff count to: " + String(puff_count));


            // Needs to be done periodically, maybe RTOS?
            FsmData data; 
            pack_fsm_data(&data, current_state, puff_count, puff_start_time, puff_duration, 0, 0); 
            send_fsm_data(&data); 
        break; 

        case(LOCKDOWN):
        break; 
    }
}

void fsm::pack_fsm_data(FsmData *data, state_t current_state, int puff_count, long puff_start_time, long puff_duration, long lockdown_start_time, long lockdown_duration){
    data->current_state = current_state; 
    data->puff_count = puff_count; 
    data->puff_start_time = puff_start_time; 
    data->puff_duration = puff_duration; 
    data->lockdown_start_time = lockdown_start_time; 
    data->lockdown_duration = lockdown_duration;         
}


void fsm::send_fsm_data(FsmData *data){
    static uint8_t pkt_out[MSG_LEN]; 
    memcpy(pkt_out, data, MSG_LEN);
    mem_send(pkt_out); 
}
