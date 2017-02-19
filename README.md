# simpleFTP
需要在Linux或者Mac OS下编译运行。

本FTP实现方式为被动方式。
### Server端
用法：gcc -o server -O2 server.cpp

编译完成后：./server <  port >

<  port >表示server端想使用的端口号。


### Client端
用法：gcc -o client -O2 client.cpp

编译完成后：./server < ip_address > <  port >

< ip_address >表示server端的ip地址。

<  port >表示server端使用的端口号。

具体命令如下：
- get: 取远方的一个文件。
- put: 传给远方的一个文件。
- pwd: 显示远方当前文件。
- dir: 列出远方当前目录。
- cd: 改变远方当前目录。
- ?: 获得帮助。
- quit: 退出返回。
