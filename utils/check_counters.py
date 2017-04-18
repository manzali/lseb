#!/usr/bin/python

import sys
import subprocess
import time
from datetime import datetime

interval = 5.0 # time interval in seconds

while True:

    # reset counters

    p = subprocess.Popen(["/usr/sbin/perfquery", "-x", "-R"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = p.communicate()

    if err:
        sys.exit(err)

    # sleep

    start = datetime.now()
    time.sleep(interval)
    end = datetime.now()
    delta_t = end - start

    # read counters

    p = subprocess.Popen(["/usr/sbin/perfquery", "-x", "-r"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = p.communicate()

    if err:
        sys.exit(err)

    # split output

    output_list = out.split("\n")

    # Get Xmit and Rcv
    
    xmit = long(output_list[3].split("PortXmitData:....................", 1)[1])
    rcv = long(output_list[4].split("PortRcvData:.....................", 1)[1])

    xmit = xmit / 1000000000.0 # to giga
    rcv = rcv / 1000000000.0 # to giga

    xmit = xmit * 4.0 # 4 lanes
    rcv = rcv * 4.0 # 4 lanes

    xmit = xmit * 8.0 # from byte to bit
    rcv = rcv * 8.0 # from byte to bit

    xmit = xmit / delta_t # in time interval
    rcv = rcv / delta_t # in time interval

    print "Xmit: {} Gb/s".format(xmit)
    print "Rcv:  {} Gb/s".format(rcv)
    print "Tot:  {} Gb/s".format(xmit + rcv)
    
    sys.stdout.flush()
