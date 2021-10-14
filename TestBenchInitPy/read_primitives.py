import socket
import time



UDP_IP = "192.168.1.8"
UDP_PORT = 58914



UDP_IP_PC = "192.168.1.20"
UDP_PORT_PC = 58914

sock1 = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP


print ("UDP target IP:", UDP_IP)
print ("UDP target port:", UDP_PORT)


sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
sock.bind(('0.0.0.0', UDP_PORT_PC))


#with open("export_file.bin", "rb") as binaryfile :
start_time = time.time()

lastaddress0=0

##############################################################
####################### PORT 0 ###############################
##############################################################
doit=1
nbytes=0;


with open("Run8764_burst500_IRC.bin", "rb") as binaryfile :
    while True:
        chunk = binaryfile.read(1024) #read 8 bytes
        strg = ""
	for ch in chunk:
		strg += hex(ord(ch))
                nbytes+=1
                if(nbytes%4==0):
                    strg += " [address:" + str(hex(nbytes-4)) + " - #prim: " + str(nbytes/4) +"]\n"
                    lastaddress0=nbytes-4
                    
        y = strg.replace('0x','')
	if(doit > 0 and doit < 2):
            print "packet number " + str(doit) + "\n"
            print y + "\n"
        time.sleep(20/10000.0) #wait 2 us
        sock.sendto(chunk, (UDP_IP, UDP_PORT))
        doit +=1
        if not chunk:
            break
print("port initialized...")

        
elapsed_time = time.time() - start_time

print("elapsed time: ",elapsed_time)

print("last address0: 0x", hex(lastaddress0))
