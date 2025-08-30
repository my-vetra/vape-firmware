#include <state_machine.h>
#include <Arduino.h>

// static struct phaseModel phases[NUM_PHASES]; 
// RTC_DATA_ATTR ActivePhaseModel curr_phase; 
// RTC_DATA_ATTR state_t current_state = /* some initial value, e.g. */ WAIT;


fsm::fsm(){
    curr_phase.phase_index = 0; 
    // Don't know what I shud put here
    curr_phase.phase_start_date = date_time{0, 0, 0}; 

    for(int i = 0; i < NUM_PHASES; i++){
        struct phaseModel new_pm; 
        new_pm.phaseIndex = i; 
        new_pm.phase_duration = PHASE_DURATION; 
        new_pm.maxPuffs = MAX_PUFFS; 
        new_pm.puffs_taken = 0; 
        new_pm.id = new_pm.phaseIndex; 
        phases[i] = new_pm; 
    }
    counter = 0; 
}

// Invariant: Enter wait state at the beginning of every new phase

// 20 minutes total --> finished puffs in 5, lockdown period = 15
// if pm duration elapsed, change phase regardless

void fsm::handle_state_rising(){
    switch(current_state){
        case(WAIT):
            pm = phases[curr_phase.phase_index]; 
            pm.puffs_taken = 0; 
            pm.phaseIndex = curr_phase.phase_index; 
            pm.id = pm.phaseIndex; 
            
            curr_puff.puffNumber = counter; 
            curr_puff.id = counter; 

            static unsigned long lockdown_start_time = 0; 
            static unsigned long lockdown_duration = 0; 
            unsigned long lockdown_period = 0; 

            current_state = PUFF_COUNTING; 
            index = 0; 
        break; 
        
        case(PUFF_COUNTING):
            // More time than phase_duration has
            // elapsed so reset current_state regardless
            if(micros() - pm.phase_time_start >= pm.phase_duration){
                curr_phase.phase_index = max(curr_phase.phase_index + 1, NUM_PHASES - 1); 
                current_state = WAIT; 
            }
            else{
                // Max puffs have been taken so enter LOCKDOWN stage
                if(pm.puffs_taken > pm.maxPuffs){
                    lockdown_start_time = micros(); 
                    lockdown_period = micros() - pm.phase_duration; 
                    current_state = LOCKDOWN; 
                }
                struct puffModel new_puff;
                
                new_puff.puffNumber = curr_puff.puffNumber + 1; 
                new_puff.id = new_puff.puffNumber;

                new_puff.phaseIndex = curr_phase.phase_index; 
                new_puff.puff_duration = micros(); 

                // Push new puff
                pm.puffs[index] = new_puff; 

                curr_puff = new_puff;   

                pm.puffs_taken++; 
                counter++; 
            }
            
        break; 

        case(LOCKDOWN):
            lockdown_duration = micros() - lockdown_start_time;
            if(lockdown_duration >= lockdown_period){
                curr_phase.phase_index = max(curr_phase.phase_index + 1, NUM_PHASES - 1); 
                // Dont need this?
                // pm.phase_time.end = micros(); 
                current_state = WAIT; 
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
            pm.puffs[index].puff_duration = micros() - curr_puff.puff_duration; 
            index++; 
            
            // Fractional puffs?
            // if(curr_puff.puff_duration >= ONE_PUFF_DURATION){
            //     pm.puffs_taken += ((curr_puff.puff_duration) / ONE_PUFF_DURATION);  
            // }

            // Needs to be done periodically, maybe RTOS?
            // FsmData data; 
            // pack_fsm_data(&data, current_state, puff_count, puff_start_time, puff_duration, 0, 0); 
            // send_fsm_data(&data); 
        break; 

        case(LOCKDOWN):
        break; 
    }
}
