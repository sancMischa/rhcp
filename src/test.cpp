#include "can.h"

#include <stdio.h>
#include <stdlib.h>


int main(){

    int can_connected = 0; // can not connected
    int nbytes = -1; // no new data read from can interface
    int can_sock_deinit = 1; // socket deinitialized

    int sock = 0;
    int *s = &sock; // can interface's socket
    struct canfd_frame frame = {.can_id = 0, .len = 0, .flags = 0, .__res0 = 0, .__res1 = 0, .data = {0}}; // without definition, won't work
    struct canfd_frame *read_frame = &frame;

    while(1){
        
        can_connected = rhcp::canIsConnected();
        if(!can_connected){
            printf("CAN not connected\n");
            can_sock_deinit = -1;
        }
        else{
            if(can_sock_deinit){
                can_sock_deinit = rhcp::canInitSocket(s);
                if(can_sock_deinit)
                    return 1; // didn't sucessfully initialize socket
            }
        }

        // check whether CAN connected or not
        // if CAN disconnected, set socket deinit
        // if CAN connected, check if socket deinit true
        // if socket deinit, init socket, continue
        // else, continue

        nbytes = rhcp::canReadFrame(s, read_frame);  
            
    }
 
    return 0;

}