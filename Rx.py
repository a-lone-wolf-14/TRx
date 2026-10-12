import socket
import serial
import time
import os
from dotenv import load_dotenv

load_dotenv()

UDP_IP = os.getenv("UDP_IP")  #sbc ip only (same as the Tx.py) 
UDP_PORT = int(os.getenv("UDP_PORT"))        # must match sender
SERIAL_PORT = os.getenv("SERIAL_PORT")         # change as needed, use "ls /dev/tty*" command to see the external MCU port, you have to find it out, not specified there
BAUD = 115200
USE_SERIAL = False                     # set to False to disable serial output

PWM_MIN = 1300
PWM_MAX = 1700
PWM_NEUTRAL = 1500

UDP_TIMEOUT = 0.1                   # socket timeout
FAILSAFE_TIMEOUT = 0.3              # neutral if no valid packet
ETH_LATENCY = 50000		    # millisec
# Mixing gain for horizontal thrusters
MIX_GAIN = 0.5

#SOCKET
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(UDP_TIMEOUT)

if True:
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)              #SERIAL

last_rx_time = time.time()

print(f"[INFO] UDP listening on port {UDP_PORT}")
if True:
    print(f"[INFO] Serial connected: {SERIAL_PORT} @ {BAUD}")

def clamp(val):
    return max(PWM_MIN, min(PWM_MAX, val))

def mix_thrusters(surge, sway, heave, yaw):
    """
    8-thruster mixing:
    T1-T4: horizontal vectored at 45°
    T5-T8: vertical
    """

    pwm = [PWM_NEUTRAL] * 8

    pwm[5] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge - sway - yaw)))  # FL
    pwm[4] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge + sway - yaw)))  # FR
    pwm[7] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge - sway + yaw)))  # BL
    pwm[6] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (-surge + sway + yaw)))  # BR

   #H(FL,FR,BL,BR)
        # if i == x:
        # pwm[i] = clamp(round(PWM_NEUTRAL - heave))
        # if thrusters reversed
    pwm[0] = clamp(round(PWM_NEUTRAL + heave))  #H
    pwm[1] = clamp(round(PWM_NEUTRAL - heave))  #H
    pwm[2] = clamp(round(PWM_NEUTRAL + heave))  #H
    pwm[3] = clamp(round(PWM_NEUTRAL + heave))  #H

    return pwm

def send_pwm(pwm):
    out = "<" + ",".join(str(x) for x in pwm) + ">\n"
    if USE_SERIAL:
        ser.write(out.encode())
    print("[PWM]", out)

def send_neutral():
    neutral = [PWM_NEUTRAL] * 8
    send_pwm(neutral)

try:
    while True:
        try:
            data, _ = sock.recvfrom(1024)
            msg = data.decode().strip()

            if not (msg.startswith("<") and msg.endswith(">")):
                continue

            msg = msg[1:-1]
            parts = msg.split(',')

            if len(parts) < 5:
                continue

            try:
                timestamp = int(parts[0])
                surge = float(parts[1])
                sway  = float(parts[2])
                heave   = float(parts[3])
                yaw = float(parts[4])
            except ValueError:
                continue

            current_time = int(time.time() * 1000)
            #print(f"{current_time}\n")

            if current_time - timestamp > ETH_LATENCY:
                print("[WARN] Dropped stale packet")
                continue

            pwm = mix_thrusters(surge, sway, heave, yaw)

            send_pwm(pwm)

            last_rx_time = time.time()

        except socket.timeout:
            if time.time() - last_rx_time > FAILSAFE_TIMEOUT:
                print("[FAILSAFE] No UDP command received")
                send_neutral()

except KeyboardInterrupt:
    print("\n[INFO] Stopping safely...")

    # Send neutral before exit
    send_neutral()

finally:
    sock.close()
    if USE_SERIAL:
        ser.close()                                            #---------------------------------
    print("[INFO] Socket and serial closed.")
