#include <mem_sys.h>

void mem_recv(uint8_t *data_in, uint8_t *buf){
    memcpy(buf, data_in, MSG_LEN); 
}