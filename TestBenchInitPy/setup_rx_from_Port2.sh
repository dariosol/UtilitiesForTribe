ifconfig eno2 192.168.1.6
ifconfig eno2 hw ether 00:01:02:03:04:06
arp -s 192.168.1.6 00:01:02:03:04:06
