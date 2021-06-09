# Simple-Chat-App
A chat app that includes a Client and a Server both written in C.

## The Server
The server is single threaded and uses the `select` system call to handle connecting new clients and receving messages.

## The Client
The Client is a GUI made with GTK 3.0. It has two threads, one handles the GUI and the other handles receving messages.
