#include "can.h"

// Standard libraries
#include <string.h>
#include <stdio.h>

//Networking libraries
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

const char* rhcp::CAN_INTERFACE_NAME = "can0";

int rhcp::canInitSocket(int *s){
    
    struct ifreq ifr;
    struct sockaddr_can addr;
    int enable_canfd = 1;
    int flags;
    
    //create socket
    if ((*s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0){
        perror("Socket");
        return 1;
    }

    //retrieve interface index
    strcpy(ifr.ifr_name, rhcp::CAN_INTERFACE_NAME);
    ioctl(*s, SIOCGIFINDEX, &ifr);

    //bind socket to CAN interface
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(*s, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("Bind");
        return 1;
    }

    // enable CAN FD support
    if (setsockopt(*s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd))){
        printf("Error when enabling CAN FD support\n");
        return 1;
    }

    // set socket as non-blocking
    flags = fcntl(*s, F_GETFL, 0);
    if (flags == -1){
        return 1;
    }

    if (fcntl(*s, F_SETFL, (flags | O_NONBLOCK))!=0){
        return 1;
    }

    return 0;
}

int rhcp::canReadFrame(int *s, canfd_frame *frame){
    int nbytes;

    nbytes = read(*s, frame, sizeof(struct canfd_frame));

    // debugging purposes    
    if(nbytes == -1){
        // printf("No data to read, would block.\n");
    }
    else{
        printf("0x%03X [%d] ",frame->can_id, frame->len); // can_dlc instead of len for std can frames

        for (int i = 0; i < frame->len; i++)
            printf("%02X ",frame->data[i]);
        printf("\r\n");
    }
    
    return nbytes;

}

// TODO: modify this to take in data and datalength
int rhcp::canSendFrame(int *s){
    
    struct canfd_frame frame;
    frame.can_id = 0x555;
    frame.len = 17; //can)dlc with std can
     
    frame.data[0] = 0x44;
    frame.data[1] = 0x66;

    if (write(*s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write");
        return 1;
    }
    
    return 0;
}

int rhcp::canCloseSocket(int *s){
    if (close(*s) < 0) {
		perror("Close");
		return 1;
	}

    return 0;
}

// returns 0 if can connected, 1 if can not connected, -1 if issue opening file
int rhcp::canIsConnected(){
    FILE *fp;
    char *fname = "/proc/net/dev";
    const char *search = rhcp::CAN_INTERFACE_NAME;
    char temp[512];

    // open file
    if((fp = fopen(fname, "r")) == NULL){
        return -1;
    }

    // read each line, search for string
    while (fgets(temp, sizeof(temp), fp) != NULL){
        if (strstr(temp, search) != NULL){
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    
    return 1;
}