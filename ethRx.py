import socket
import time

UDP_IP = "192.168.144.40"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.bind((UDP_IP,UDP_PORT))

try:
    while True:
        data, _ = sock.recvfrom(1024)
        msg = data.decode().strip()

        timestamp = int(msg)

        current_time = int(time.time()*1000) #ms

        print(f"Timestamp (RECEIVED): {timestamp} | Current Timestamp: {current_time} | LATENCY: {current_time - timestamp}\n")
except KeyboardInterrupt:
    print("\nStopped\n")
finally:
    sock.close()
