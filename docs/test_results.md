実行環境: ubuntu22.04
実行結果:

クライアント側
nanzan@nanzan-VirtualBox:~$ ./client 127.0.0.0 5000
connected; type a short line, Enter to send, Ctrl+D to quit
client > ADD 3 4
server -> OK 7
client > add 3 4
server -> ERROR invalid-argument
client > 

サーバ側
nanzan@nanzan-VirtualBox:~$ ./server 
LISTEN 0.0.0.0:5000
connected: listen_fd=3 client_fd=4
client -> ADD 3 4
client -> add 3 4

