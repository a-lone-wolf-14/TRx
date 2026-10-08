import socket
import time

UDP_IP = "192.168.144.40" 	#sbc ip
UDP_PORT = 5005 		#must match same as in the receiver file
SEND_RATE = 0.02

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    while True:
        timestamp = int(time.time()*1000)    #ms
        print(f"{timestamp}\n")

        sock.sendto(str(timestamp).encode(), (UDP_IP, UDP_PORT))

        time.sleep(SEND_RATE)
except KeyboardInterrupt:
    print("\nStopped\n")
finally:
    sock.close()
