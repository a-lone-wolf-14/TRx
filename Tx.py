import pygame
import socket
import time

UDP_IP = "127.0.0.1"   # sbc ip
UDP_PORT = 5005

# MAX_CHANNELS = 8

# PWM_NEUTRAL = 1500
# PWM_MIN = 1250
# PWM_MAX = 1850

#adjust these values for finer control over each DOFs
SURGE_SCALE = 300
SWAY_SCALE = 250
YAW_SCALE = 200
HEAVE_SCALE = 280

SEND_RATE = 0.02

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("No joystick detected")
    exit()

joystick = pygame.joystick.Joystick(0)
joystick.init()

print(f"Connected to: {joystick.get_name()}")

# def clamp(val):
#     return max(PWM_MIN, min(PWM_MAX, val))

# def axis_to_pwm(axis_val):
#     return int(PWM_NEUTRAL + axis_val * SCALE)

def apply_deadzone(val, dz=0.05):
    return 0 if abs(val)<dz else val

try:
    while True:
        pygame.event.pump()

        # read axes
        axis_x = apply_deadzone(joystick.get_axis(0))
        axis_y = apply_deadzone(joystick.get_axis(1))
        axis_yaw = apply_deadzone(joystick.get_axis(3))
        axis_heave = apply_deadzone(joystick.get_axis(4))

        #cmd order : "<surge,sway,yaw,heave>"
        cmd = [-(axis_y**3)*SURGE_SCALE , (axis_x**3)*SWAY_SCALE , (axis_yaw**3)*YAW_SCALE , -(axis_heave**3)*HEAVE_SCALE]

        #pwm array
        # pwm = [PWM_NEUTRAL] * MAX_CHANNELS

        # pwm[0] = clamp(surge + yaw - PWM_NEUTRAL)
        # pwm[1] = clamp(surge - yaw - PWM_NEUTRAL)
        # pwm[2] = clamp(sway + yaw - PWM_NEUTRAL)
        # pwm[3] = clamp(sway - yaw - PWM_NEUTRAL)
        # pwm[4] = clamp(heave)
        # pwm[5] = clamp(heave)
        # pwm[6] = PWM_NEUTRAL
        # pwm[7] = PWM_NEUTRAL

        # timestamp
        timestamp = int(time.time() * 1000)  # ms

        #pwm string
        cmd_string = "<" + str(timestamp)

        for val in cmd:
            cmd_string += "," + str(val)

        cmd_string += ">"

        #forwarding to UDP
        sock.sendto(cmd_string.encode(), (UDP_IP, UDP_PORT))

        print(cmd_string+"\n")

        time.sleep(SEND_RATE)

except KeyboardInterrupt:
    print("\nStopped by user.")

finally:
    sock.close()
    pygame.quit()
