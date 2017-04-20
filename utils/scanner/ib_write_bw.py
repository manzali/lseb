#!/usr/bin/python

import subprocess
import sys
import time
import os

# start script

# retrieve IPOIB
ipoib = os.environ["SCANNER_IPOIB"]

# retrieve RANK
rank = os.environ["OMPI_COMM_WORLD_RANK"]

command_list = ["ib_write_bw", "-F", "--report_gbits", "-d", "mlx5_0", "-D", "7", "-f", "2"]

if rank == "0":

    # process a (server)

    p = subprocess.Popen(command_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = p.communicate()

    if err:
        sys.exit(err)

    # parse output of ib_write_bw
    before, after = out.split("65536") # buffer size
    after = after.lstrip()
    iterations, after = after.split(" ", 1)
    after = after.lstrip()
    bw_peak, after = after.split(" ", 1)
    after = after.lstrip()
    bw_average, after = after.split(" ", 1)

    print bw_average

elif rank == "1":
    
    # process b (client)

    time.sleep(1) # delays for 1 second

    # append server ip
    command_list.append(ipoib)

    p = subprocess.Popen(command_list, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = p.communicate()

    if err:
        sys.exit(err)

else:
    
    sys.exit("wrong rank!")