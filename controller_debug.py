import pygame
import time

pygame.init()
pygame.joystick.init()

if pygame.joystick.get_count() == 0:
    print("No joystick found")
    exit()

joy = pygame.joystick.Joystick(0)
joy.init()

print("Controller:", joy.get_name())
print("Move sticks / triggers to identify mapping...\n")

try:
    while True:
        pygame.event.pump()

        axes = []
        for i in range(joy.get_numaxes()):
            axes.append(round(joy.get_axis(i), 3))

        print(axes)
        time.sleep(0.1)

except KeyboardInterrupt:
    print("Stopped")
