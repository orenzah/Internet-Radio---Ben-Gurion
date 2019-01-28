# Welcome to The Internet Radio Application Project
Computers Networks 2 - 371-1-0211
[Moodle](https://moodle2.bgu.ac.il/moodle/course/view.php?id=23418).
[Book Manual](https://github.com/orenzah/Internet-Radio---Ben-Gurion/blob/master/Internet%20Radio%20Application%202018.pdf)
[Presentation Manaul](https://github.com/orenzah/Internet-Radio---Ben-Gurion/blob/master/Internet%20Radio%20Application%20Presentation.pdf)
## Description

The project may be divided to two parts:
1.	Server
2.	Client

### Server

The server part includes:
A thread for UDP multicast transmitter.
The main proccess is responsible for the 'welcome socket', each new new accepted client socket is delegated to a new thread.
Each new thread is responsible for whole communication with its client.

### Client

The client part includes:
A thread for the UDP multicast listener, while the mutlicast group can be changed using IPC Messages API.
The client socket is implemented downto Layer 3 (IP Layer), due to binding restrictions.

The client main process will check changes in the sockets of the stdin and the tcp against the server, using select().
