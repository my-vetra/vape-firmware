#include <Arduino.h>
#include "puffTime.h"
#include "memory_sys.h"
volatile static long start_time; 
volatile static long duration; 


#define HEAT_PIN 10

// This function is called via an interrupt.
// So can assume that puff is either starting or finishing 
// if this function is being called
void puff_data_controller(){
    bool puff_pin_state = digitalRead(HEAT_PIN); 
    if(puff_pin_state){
        start_time = micros(); 
    }
    else{
        duration = micros() - start_time; 
        // Memory Write and read will likely take a serialized argument. 
        // Will need to make a parser with different flags in memory. 
        // Need to discuss what kinds of data we will be storing and reading from memory. 
        // memory_write(); 
    }
} 
