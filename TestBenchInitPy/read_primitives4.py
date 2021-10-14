import socket
import time



UDP_IP = "192.168.1.11"
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
npackets=0;
nbytesofpacket=0
breakflag=0

#with open("Run8764_burst500_IRC.bin", "rb") as binaryfile :
with open("/home/na62torino/Data/l0tribeinitandanalysis/TestBenchInitPy/Run9386/Run9386_burst100_IRC.bin", "rb") as binaryfile :
    while True:
        chunk = binaryfile.read(1024) #read 8 bytes
        strg = ""
	for ch in chunk:
            strg += hex(ord(ch))
            nbytes+=1
            nbytesofpacket+=1
            if(nbytes%4==0):
                strg += " [address:" + str(hex(nbytes-4)) + " - #prim: " + str(nbytes/4) +"]\n"
                lastaddress0=nbytes-4
                
        y = strg.replace('0x','')
	if(doit > 0 and doit < 2):
            print "packet number " + str(doit) + "\n"
            print y + "\n"
        time.sleep(0.1/10000.0) #wait
        
        #Fill the last packet up to 1024 bytes
        while(nbytesofpacket<1024):
            chunk= chunk+'\x00'
            breakflag=1
            nbytesofpacket+=1
            lastaddress0=lastaddress0+1
        sock.sendto(chunk, (UDP_IP, UDP_PORT))
        npackets+=1
        doit +=1
        nbytesofpacket=0
        if not chunk or breakflag==1:
            break


print "npackets containing primitives: " + str(npackets) 
zeroes=bytearray(1024)
debug=0

while True:
    if((lastaddress0 + 0x80) < 0x11800000):
        sock.sendto(zeroes, (UDP_IP, UDP_PORT))
        lastaddress0=lastaddress0+1024
        time.sleep(0.2/10000.0) #wait 2 us
        debug=debug+1
        if((debug)==25000):
            print (float(lastaddress0)/float(0x11800000))
            debug=0
            
    else:
        break

print("port initialized...")

detectorUnderInit = "5a5a5a5a5a5a5a00".decode('hex')
sock.sendto(bytes(detectorUnderInit), (UDP_IP, UDP_PORT))

elapsed_time = time.time() - start_time

print("elapsed time: ",elapsed_time)

print("last address0: 0x", hex(lastaddress0+0x80))

