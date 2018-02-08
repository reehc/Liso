## Explain !

### 1. Connection reset by peer
The reason for this problem is that client closes the socket while client's receive queue is not yet empty.
If I do not send any data to client, this error will disappear.
So it depends on client sometime:
```
shutdown(socket, SHUT_WR);
close(socket)
```
will send a SIGPIPE signal to server, which tells server no more transmissions and problem solved!
Remember to handle this signal cause it'll kill your process.
In fact it helped nothing...I'am still trapped in this nightmare. I just don't understand that testing script!
Anyway, I am fucked up with this script!

[TCP RST](http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/TCPRST.pdf)

### 2. Persistent Communication
