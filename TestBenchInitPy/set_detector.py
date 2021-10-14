import socket
import time
import sys

print 'Number of arguments:', len(sys.argv), 'arguments.'
print 'Argument List:', str(sys.argv)

index = int(sys.argv[1])

print index 

UDP_IP = "192.168.1.11"
#UDP_IP = "192.168.1.13"
UDP_PORT = 58914
detectorUnderInit="5a5a5a5a5a5a5aff"
if index == 0:
    detectorUnderInit = "5a5a5a5a5a5a5a00".decode('hex')
elif index == 1:
    detectorUnderInit = "5a5a5a5a5a5a5a01".decode('hex')
elif index == 2:
    detectorUnderInit = "5a5a5a5a5a5a5a02".decode('hex')
elif index == 3:
    detectorUnderInit = "5a5a5a5a5a5a5a03".decode('hex')
elif index == 4:
    detectorUnderInit = "5a5a5a5a5a5a5a04".decode('hex')
elif index == 5:
    detectorUnderInit = "5a5a5a5a5a5a5a05".decode('hex')
elif index == 6:
    detectorUnderInit = "5a5a5a5a5a5a5a06".decode('hex')
elif index == 7:
    detectorUnderInit = "5a5a5a5a5a5a5a07".decode('hex')
elif index == 8:
    detectorUnderInit = "5a5a5a5a5a5a5a08".decode('hex')
elif index == 9:
    detectorUnderInit = "5a5a5a5a5a5a5a09".decode('hex')
elif index == 10:
    detectorUnderInit = "5a5a5a5a5a5a5a0a".decode('hex')
elif index == 11:
    detectorUnderInit = "5a5a5a5a5a5a5a0b".decode('hex')
elif index == 12:
    detectorUnderInit = "5a5a5a5a5a5a5a0c".decode('hex')
elif index == 13:
    detectorUnderInit = "5a5a5a5a5a5a5a0d".decode('hex')
elif index == 14:
    detectorUnderInit = "5a5a5a5a5a5a5a0e".decode('hex')
elif index == 15:
    detectorUnderInit = "5a5a5a5a5a5a5a0f".decode('hex')
else:
    print "wrong index"
    exit()
    
UDP_IP_PC = "192.168.1.20"
UDP_PORT_PC = 58914

sock1 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP


print ("UDP target IP:", UDP_IP)
print ("UDP target port:", UDP_PORT)
print ("detectorUnderInit:", detectorUnderInit)

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind(('0.0.0.0', UDP_PORT_PC))


##############################################################
####################### PORT 0 ###############################
##############################################################

sock.sendto(bytes(detectorUnderInit), (UDP_IP, UDP_PORT))

print "detectorUnderInit set"
