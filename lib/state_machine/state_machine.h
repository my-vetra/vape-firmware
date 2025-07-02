enum state_t {WAIT, PUFF_COUNTING, LOCKDOWN}; 

#define HEAT_PIN 10
#define MAX_PUFFS 10
#define LOCKDOWN_PERIOD (10UL)

void init_state_machine(); 
void handle_state(); 

extern state_t current_state; 