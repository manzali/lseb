#!/usr/bin/python
import sys,getopt
import os,subprocess
import datetime

def usage():
    print("usage: ./genconfig.py -H <host1,host2,...> -i <ibXY,ibXX,...>\n")
    print("-h, --host: list of hosts\n")
    print("-i, --interfaces: list of interfaces (must be the same number of hosts)\n")
    print("example: ./genconfig.py -H rd-xeon-02.cnaf.infn.it,rd-xeon-04.cnaf.infn.it -i ib0,ib0\n")
    sys.exit()
    
def parse_file():
    print "Parsing file"

def build_endpoints_string(ip_list,base_port):
  ip_list_ports = [str(ip)+":"+str(base_port + ip_list.index(ip)) for ip in ip_list ]
  s = ""
  for e in ip_list_ports:
	s+=e
	s+=","
  return s[0:(len(s)-1)]  

def create_config(ip_list):
    print "Creating config file"
    suffix = datetime.datetime.now().strftime("%y%m%d_%H%M%S.ini")
    filename = "_".join(["config", suffix])
    out_file = open(filename,"w")
    #Generator block 
    out_file.write("[GENERATOR]\n")
    out_file.write("MEAN = 200\n")
    out_file.write("STD_DEV = 20\n")
    out_file.write("FREQUENCY = 30000000\n")
    out_file.write("\n")
    #General stuff
    out_file.write("[GENERAL]\n")
    out_file.write("BULKED_EVENTS = 600\n")
    out_file.write("\n")
    #RU block
    out_file.write("[RU]\n")
    out_file.write("LOG_LEVEL = INFO\n")
    out_file.write("META_BUFFER = 57521880\n")
    out_file.write("DATA_BUFFER = 536870912\n")
    out_file.write("ENDPOINTS = "+build_endpoints_string(ip_list,6000)+"\n")
    out_file.write("\n")
    #BU block
    out_file.write("[BU]\n")
    out_file.write("LOG_LEVEL = INFO\n")
    out_file.write("DUMMY = 0\n")
    out_file.write("RECV_BUFFER = 8388608\n")
    out_file.write("MS_TIMEOUT = 50.0\n")
    out_file.write("ENDPOINTS = "+build_endpoints_string(ip_list,7000)+"\n")
    out_file.close()
    return 0





def get_ip_from_if(hosts,ifaces):
    remote_env = os.environ.copy()
    ip_list = []
    for i in range(0,len(hosts)):
      print "Getting ip for interface ",ifaces[i]," for host ",hosts[i]
      exe = remote_env['PWD']+"/all_interface.py"
      cmd = ["/usr/bin/ssh",hosts[i],exe,ifaces[i]]
      p = subprocess.Popen(cmd,env=remote_env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      out, err = p.communicate()
      if err:
        print "ERR: Error while contacting host ",hosts[i]
        print "ERR: Returning empty IP list"
        return []
      if hosts[i]:
        ip_list.append(out.replace("\n", ""))
    return ip_list
    
def main(argv):                         
    try:                                
        opts, args = getopt.getopt(argv, "hH:i:F:", ["help", "hosts=","interaces=","input_filename="])
    except getopt.GetoptError:           
        usage()                          
        sys.exit(2) 
    hosts = []
    ifaces = []
    filename=""
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
        elif opt in ("-H","--hosts"):
            hosts = arg.split(",")
        elif opt in ("-i","--interfaces"):
            ifaces = arg.split(",")
        elif opt in ("-F","--input_filename"):
            filename=arg
        else:
            usage()
    if (len(opts) == 0):
        print "len",len(opts)
        usage()
    if (len(hosts)!=0 and (len(hosts)==len(ifaces))):
        ip_list = get_ip_from_if(hosts,ifaces)
        create_config(ip_list)
    else:
       usage()
        
      


if __name__ == "__main__":
    #print sys.argv[1:]
    main(sys.argv[1:]) 
