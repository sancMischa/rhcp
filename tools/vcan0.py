import can
import time
import random

# https://python-can.readthedocs.io/en/2.1.0/interfaces/socketcan.html

bus = can.interface.Bus(bustype='socketcan', channel='vcan0')


def generate_random_list(length, min_value, max_value):
    random_list = []
    for _ in range(length):
        random_value = random.randint(min_value, max_value)
        random_list.append(random_value)
    return random_list


while True:
    msg = can.Message(arbitration_id=0x44, data=generate_random_list(6, 0, 200), is_extended_id=False)
    bus.send(msg)
    
    time.sleep(0.1)
