#include <state_machine.h>
#include <Arduino.h>

// enum state_t {WAIT, PUFF_COUNTING, LOCKDOWN}; 


void init_state_machine(){
    current_state = WAIT; 
}


void handle_state(){
    volatile static long puff_start_time; 
    volatile static long puff_duration; 
    volatile static long lockdown_start_time; 
    volatile static long lockdown_duration; 
    volatile int puff_count; 

    switch(current_state){
        case WAIT:
            // If first puff is taken, every puff after this counts

            // NOTE: THIS IS NOT RIGHT, PUFF_COUNT SHOULD BE READ FROM MEMORY
            // THIS JUST RESETS puff_count every time and starts from the beginning
            puff_count = 0; 
            current_state = PUFF_COUNTING; 
            break;
        
        case PUFF_COUNTING:
            // PROBABLY NEED TO OPEN NICOTINE GATE HERE AS WELL
            // NOT SURE HOW TO DO THAT THOUGH

            // If HEAT_PIN is high, puff has just started
            if(digitalRead(HEAT_PIN)){
                puff_start_time = micros(); 
            }
            // Else puff has ended, calculate the puff duration 
            // and reset start_time
            else{
                puff_duration = micros() - puff_start_time; 
                puff_start_time = 0; 
                puff_count++; 
                // Write duration to memory or increment puffs, need to discuss what happens here
            }

            if(puff_count >= MAX_PUFFS){
                current_state = LOCKDOWN;
                lockdown_start_time = micros(); 
            }

            break; 

        case LOCKDOWN:
            lockdown_duration = micros() - lockdown_start_time;
            if(lockdown_duration >= LOCKDOWN_PERIOD){
                current_state = PUFF_COUNTING; 
                puff_count = 0; 
            }
            break; 
    }
}