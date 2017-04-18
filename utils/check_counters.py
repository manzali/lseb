import sys
import subprocess
import time

subprocess.call(["/usr/sbin/perfquery", "-x", "-R"])

while True:

	time.sleep(5)

	output = subprocess.check_output(["/usr/sbin/perfquery", "-x", "-r"])
	output_list = output.split("\n")

	# Get Xmit and Rcv
	xmit = long(output_list[3].split("PortXmitData:....................", 1)[1])
	rcv = long(output_list[4].split("PortRcvData:.....................", 1)[1])

	gb_xmit = xmit * 8 * 4 / 1000000000.0 / 5.0
	gb_rcv = rcv * 8 * 4 / 1000000000.0 / 5.0

	print "Xmit: {} Gb/s".format(gb_xmit)
	print "Rcv:  {} Gb/s".format(gb_rcv)
	print "Tot:  {} Gb/s".format(gb_xmit + gb_rcv)
    
        sys.stdout.flush()
