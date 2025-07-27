#include <mem_sys.h>

void mem_recv(uint8_t *data_in, uint8_t *buf){
    memcpy(buf, data_in, 20); 
}

void mem_send(uint8_t *data_out){
    return;
}
