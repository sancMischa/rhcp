#include "can.h"

#include <stdio.h>

int main(){

    int can_disconnected = 1;
    int socket_deinit = 1; // not initialized
    int nbytes = -1; // no new data read from can interface

    while(1){
        
        can_disconnected = rhcp::canIsConnected();

        
        if(can_disconnected){
            socket_deinit = 1;
            // printf("CAN Status (0 conn, 1 disconn, -1 err /proc/net/dev)): %d\n", can_disconnected);
            ImGui::Text("No CAN device found.\n");
            ImGui::End();
        }
        else{ // CAN connected
            ImGui::End();
            if(socket_deinit){ // initialize socket
                if(rhcp::canInitSocket(s)!=0)
                    return 1;
                socket_deinit = 0;
                printf("initialized socket\n");
            }      

            nbytes = rhcp::canReadFrame(s, read_frame);
            
        }
    }

    return 0;

}