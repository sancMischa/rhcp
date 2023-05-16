import can
import time

# https://python-can.readthedocs.io/en/2.1.0/interfaces/socketcan.html

bus = can.interface.Bus(bustype='socketcan', channel='vcan0')

# def canSend():
#     for i in range(3):
#         msg = can.Message(arbitration_id=0x44, data=[i, 0x65, 0x31, 0x28, 0x62], is_extended_id=False)
#         bus.send(msg)
#     time.sleep(1)

# canSend()
# bus.shutdown()
count = 1

while True:
    msg = can.Message(arbitration_id=0x44, data=[count, count+1, count+2, count+3, count+4, count+5], is_extended_id=False)
    bus.send(msg)
    count+=6
    
    if (count >= 200): # prevent overflow
        count = 1
    
    time.sleep(0.5)
