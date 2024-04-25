
It's a test project of the creation of a simple server and client. The server provides access to a simple key/value database. The client reads or writes values for random keys.

It can be built with CMake.

How to run server

<path_to_server>/Server -p <port> [-c <path_to_config>]

For example: ./Server -p 1234 -c ./config.txt
Test file 'config.txt' is placed in the directory 'testdata'.

How to run client

<path_to_client>/Client -s <server> -p <port>

For example: ./Client -s localhost -p 1234
