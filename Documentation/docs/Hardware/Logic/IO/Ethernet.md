# Ethernet (W5500)
The Wiznet W5500 is used for Ethernet. It is connected to one of the SPI modules of the MU. It has 8 sockets and contains hardware for the most used protocols like TCP, UDP and ICMP, making it very easy to use. Currently the interrupt pin is not used.

In the far future, when the FPGC is a lot faster, I might implement the network stack in software and use the simpler enc28j60.