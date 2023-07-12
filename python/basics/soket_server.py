# /usr/bin/env python3

import socket

if __name__ == "__main__":
    listening_soket = socket.socket(
        socket.AF_INET, socket.SOCK_STREAM
    )  # AF_INET -- IPV4, SOCK_STREAM -- TCP

    listening_soket.bind(("127.0.0.1", 5000))

    listening_soket.listen()

    while True:
        server_soket, addr = listening_soket.accept()
        print("Connected by ", addr)
        data = server_soket.recv(1024)
        if data:
            print("received data from client: ", data)
            server_soket.sendall(bytes(reversed(data)))
        else:
            print("Error, while receiving data from socket.")
        server_soket.close()

    listening_soket.close()
