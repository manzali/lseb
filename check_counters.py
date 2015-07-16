import subprocess
import time

subprocess.call(["perfquery", "-x", "-R"])

while True:

	time.sleep(3)

	output = subprocess.check_output(["perfquery", "-x", "-r"])
	output_list = output.split("\n")

	# Get Xmit and Rcv
	xmit = long(output_list[3].split("PortXmitData:....................", 1)[1])
	rcv = long(output_list[4].split("PortRcvData:.....................", 1)[1])

	gb_xmit = xmit * 8 * 4 / 1073741824.0 / 3.0
	gb_rcv = rcv * 8 * 4 / 1073741824.0 / 3.0

	print "Xmit: {} Gb/s".format(gb_xmit)
	print "Rcv: {} Gb/s".format(gb_rcv)
	print "Tot: {} Gb/s".format(gb_xmit + gb_rcv)
