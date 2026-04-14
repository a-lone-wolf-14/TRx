import socket
import serial
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 5005                     # must match sender
SERIAL_PORT = "/dev/ttyACM0"         # change as needed, use "ls /dev/tty*" command to see the external MCU port, you have to find it out, not specified there
BAUD = 115200

PWM_MIN = 1250
PWM_MAX = 1850
PWM_NEUTRAL = 1500

UDP_TIMEOUT = 0.1                   # socket timeout
FAILSAFE_TIMEOUT = 0.3              # neutral if no valid packet

# Mixing gain for horizontal thrusters
MIX_GAIN = 0.5

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(UDP_TIMEOUT)

# ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)

last_rx_time = time.time()

print(f"[INFO] UDP listening on port {UDP_PORT}")
# print(f"[INFO] Serial connected: {SERIAL_PORT} @ {BAUD}")

def clamp(val):
    return max(PWM_MIN, min(PWM_MAX, val))

def mix_thrusters(surge, sway, yaw, heave):
    """
    8-thruster mixing:
    T1-T4: horizontal vectored at 45°
    T5-T8: vertical
    """

    pwm = [PWM_NEUTRAL] * 8

    pwm[0] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge - sway - yaw)))  # FR
    pwm[1] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge + sway - yaw)))  # BR
    pwm[2] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge - sway + yaw)))  # BL
    pwm[3] = clamp(round(PWM_NEUTRAL + MIX_GAIN * (surge + sway + yaw)))  # FL

    for i in range(4, 8):
        pwm[i] = clamp(round(PWM_NEUTRAL + heave))

    return pwm

def send_pwm(pwm):
    out = "<" + ",".join(str(x) for x in pwm) + ">"
    # ser.write(out.encode())
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
                yaw   = float(parts[3])
                heave = float(parts[4])
            except ValueError:
                continue

            current_time = int(time.time() * 1000)

            if current_time - timestamp > 200:
                print("[WARN] Dropped stale packet")
                continue

            pwm = mix_thrusters(surge, sway, yaw, heave)

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
    # ser.close()
    print("[INFO] Socket and serial closed.")