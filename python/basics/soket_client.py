# /usr/bin/env python3

import socket

if __name__ == "__main__":
    client_soket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_soket.connect(("127.0.0.1", 5000))
    while True:
        send_data = input("Enter: ")
        client_soket.sendall(bytes(send_data, "utf-8"))
        data = client_soket.recv(1024)
        print("Recieved: ", repr(data))
    client_soket.close()
