# scramcast
Small ditributed shared memory library that aims to be a replacement for most of the features of scramnet card.
This library is not using a method to guarantee that all the UDP packets arrives. It just assumes you have a good network infrastructure. The memory is spreaded through UDP broadcast packets on a specific IPv4 network, which should be defined in the header file netconfig.h .
It supports up to 8mb share. Currently it is working but yet many features are missing. The library is pre mature, but some may find it usable already, as it implements the key feature of sharing memory.
Sorry for the incomplete documentation. meanwhile I don't even know if someone is reading these lines because the lack of interest, so why bother?
