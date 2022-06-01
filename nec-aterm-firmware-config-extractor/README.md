# NEC Aterm firmware config extractor
This script can extract the HW_RANDOM_KEY from NEC Aterm firmware images.
This key is used by `pwcrypt` alongside the name of the device and the wan mac address to generate the password that is used to control the access to the root shell available from the UART interface.