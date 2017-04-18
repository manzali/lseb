#!/usr/bin/python

import subprocess
import sys
from datetime import datetime

node_list = ["lab34", "lab35", "lab36", "lab37", "lab38", "lab39", "lab40", "lab41"]
#node_list = ["lab35", "lab37", "lab38", "lab39", "lab40", "lab41"]

mode = "ONEWAY" # ONEWAY or TWOWAY

fname = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
fdesc = open(fname + ".txt","w")

fdesc.write(" ".join(node_list) + "\n")

# function that retrives the ip of the ib0 interface from a given hostname

def get_ip_ib0(hostname):
    
    p = subprocess.Popen(["ssh", hostname, "/usr/sbin/ifconfig", "ib0"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    
    if err and err != "Infiniband hardware address can be incorrect! Please read BUGS section in ifconfig(8).\n":
        sys.exit(err)
        
    before, after = out.split("inet ")
    ip, after = after.split(" netmask")
    ip = ip.strip() # remove spaces (if any)
        
    return ip


# start script

for a in node_list:
    
    line = ""
    
    ipoip_a = get_ip_ib0(a)
    if not ipoip_a:
        sys.exit("error while parsing the output of ifconfig!")
    
    for b in node_list:
        
        bw = 0
        bw2 = 0
        
        if a != b:

            p = subprocess.Popen(["mpirun", "-x", "SCANNER_IPOIB=" + ipoip_a, "-np", "2", "-map-by", "ppr:1:node", "--host", a + "," + b, "python", "worker.py"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            if mode == "TWOWAY":
                
                ipoip_b = get_ip_ib0(b)
                if not ipoip_b:
                    sys.exit("error while parsing the output of ifconfig!")
                
                p2 = subprocess.Popen(["mpirun", "-x", "SCANNER_IPOIB=" + ipoip_b, "-np", "2", "-map-by", "ppr:1:node", "--host", b + "," + a, "python", "worker.py"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                out2, err2 = p2.communicate()

                if err2:
                    sys.exit(err2)
                    
                bw2 = float(out2.strip())

            out, err = p.communicate()
            
            if err:
                sys.exit(err)
            
            bw = float(out.strip())
            bw = bw + bw2

        print a + " " + b + " " + str(bw)
        
        line = line + "     " + str(bw)
        
    fdesc.write(line + "\n")
    
fdesc.close()