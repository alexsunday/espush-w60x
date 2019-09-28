#!/ust/bin/env python

import socket


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    addr = '192.168.2.114'
    s.sendto(b'BBBBEEEEEEP', (addr, 21502))


if __name__ == '__main__':
    main()
