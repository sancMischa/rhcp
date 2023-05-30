#ifndef CAN_H_
#define CAN_H_

#include <linux/can.h>

namespace rhcp {

    int canInitSocket(int *s);
    int canReadFrame(int *s, canfd_frame *frame);
    int canSendFrame(int *s);
    int canCloseSocket(int *s);
    int canIsConnected();

    extern const char* can_interface_name;

} // namespace rhcp


#endif // CAN_H_