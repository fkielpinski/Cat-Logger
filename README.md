# Cat-Logger
University Project in collaboration with @szymonstanek

Cat Logger is a low-level C++ keylogger which captures keyboard strokes based on KBDHooks, communicate with server based on TCP connection via Windows Sockets 2.
It runs as background process, doesn't store any files in easy accesed data, because it is sending the buffer over the network. 

Server is GUI based application created with wxWidgets, has ability of live preview of target keyboard strokes. There is also function which filters the target input in search of e-mail addresses and PINs.

TODO:
Connection ecryption to hide plain messages
Client should regain connection after server restart
