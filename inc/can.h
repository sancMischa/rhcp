#ifndef CAN_H_
#define CAN_H_

#include <linux/can.h>

namespace rhcp {

    int canInitSocket(int *s);
    int canReadFrame(int *s, canfd_frame *frame);
    int canSendFrame(int *s);
    int canCloseSocket(int *s);
    int canIsConnected();

    extern const char* CAN_INTERFACE_NAME;

} // namespace rhcp


#endif // CAN_H_