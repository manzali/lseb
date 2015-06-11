#!/usr/bin/python
import socket
import fcntl
import struct
import array
import sys,getopt

def all_interfaces(iface):
    max_possible = 128  # arbitrary. raise if needed.
    bytes = max_possible * 32
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    names = array.array('B', '\0' * bytes)
    outbytes = struct.unpack('iL', fcntl.ioctl(
        s.fileno(),
        0x8912,  # SIOCGIFCONF
        struct.pack('iL', bytes, names.buffer_info()[0])
    ))[0]
    namestr = names.tostring()
    lst = []
    for i in range(0, outbytes, 40):
        name = namestr[i:i+16].split('\0', 1)[0]
        ip   = namestr[i+20:i+24]
        #print name
        if (name==iface[0]):
            ip = format_ip(ip)
            print ip
            return 0
        else:
            continue
    print "Cannot find ",iface," for this host"
    return -1
 
def format_ip(addr):
    return str(ord(addr[0])) + '.' + \
           str(ord(addr[1])) + '.' + \
           str(ord(addr[2])) + '.' + \
           str(ord(addr[3]))
 
def main(argv):                         
    #print "all_interfaces"
    all_interfaces(argv)


if __name__ == "__main__":
    #print "Main"
    main(sys.argv[1:]) 
