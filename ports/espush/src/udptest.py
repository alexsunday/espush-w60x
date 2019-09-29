#!/ust/bin/env python

import time
import socket


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('', 21502))
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    addr = '255.255.255.255'
    while 1:
        s.sendto(b'BBBBEEEEEEP', (addr, 21502))
        print('SENT')
        time.sleep(2)


if __name__ == '__main__':
    main()
