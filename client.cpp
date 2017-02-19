#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>

using namespace std;

char mesage[4096];
char input_line[4096];

int unuse;
char* inputsat;

int rev_buffer_size;
int send_buffer_size;
char* rev_buffer;
char* send_buffer;

string conmmand;
string conmm_msg;
string current_path;
string filepath;
string ok = "ok";

int conmm_socket, data_socket;
sockaddr_in server_conmm_addr;
sockaddr_in server_data_addr;

void get_buffer_size(){
    socklen_t optlen = sizeof(int);
    getsockopt(data_socket, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &optlen);
    getsockopt(data_socket, SOL_SOCKET, SO_RCVBUF, &rev_buffer_size, &optlen);
}

void quit_FTP(){
    unuse = write(conmm_socket, conmmand.c_str(), conmmand.length());
    int count = read(conmm_socket, mesage, sizeof(mesage));
    conmm_msg = mesage;
    cout<<conmm_msg.substr(0,count)<<endl;
    shutdown(conmm_socket, SHUT_RDWR);
    close(conmm_socket);
}

void get_input(){
    inputsat = fgets(input_line, sizeof(input_line), stdin);
    conmmand = input_line;
    conmmand = conmmand.substr(0,conmmand.length() - 1);
}

void send_basic_conmmand(){
    unuse = write(conmm_socket, conmmand.c_str(), conmmand.length());
    int count = read(conmm_socket, mesage, sizeof(mesage));
    conmm_msg = mesage;
    cout<<conmm_msg.substr(0,count)<<endl;
}

void get_file(const string& data, const char* argv){
    send_basic_conmmand();
    if (mesage[0] == '/')	return;
    unuse = write(conmm_socket, ok.c_str(), ok.length());

    short port;
    unuse = read(conmm_socket, &port, sizeof(port));
    data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&server_data_addr , 0 , sizeof(sockaddr_in));
    server_data_addr.sin_family = AF_INET;
    server_data_addr.sin_addr.s_addr = inet_addr(argv);
    server_data_addr.sin_port = port;
    while(connect(data_socket, (struct sockaddr*) &server_data_addr, sizeof(server_data_addr)) == -1);

    filepath = current_path + "/" + data.c_str();
    FILE *fp = fopen(filepath.c_str(), "wb");

    int length = 0;
    int total_length = 0;
    get_buffer_size();
    rev_buffer = new char[rev_buffer_size];
    while( (length = read(data_socket, rev_buffer, rev_buffer_size)) > 0 ){
    	fwrite(rev_buffer, sizeof(char), length, fp);
    	total_length += length;
    	cout<<"Have received "<<total_length<<" bytes"<<endl;
    }
    cout<<filepath<<" has already been received!"<<endl;
    fclose(fp);
    close(data_socket);
    delete []rev_buffer;
}

void put_file(const string& data, const char* argv){
    filepath = current_path + "/" + data.c_str();
    FILE *fp = fopen(filepath.c_str(), "rb");
    if (fp == NULL){
    	cout << "File is not exist!"<<endl;
    	return;
    }

    send_basic_conmmand();
    if (mesage[0] == '/')	return;
    unuse = write(conmm_socket, ok.c_str(), ok.length())>0;

    short port;
    unuse = read(conmm_socket, &port, sizeof(port));
    data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&server_data_addr , 0 , sizeof(sockaddr_in));
    server_data_addr.sin_family = AF_INET;
    server_data_addr.sin_addr.s_addr = inet_addr(argv);
    server_data_addr.sin_port = port;
    while(connect(data_socket, (struct sockaddr*) &server_data_addr, sizeof(server_data_addr)) == -1);

    int length = 0;
    long total_length = 0;
    get_buffer_size();
    send_buffer = new char[send_buffer_size];
    while( (length = fread(send_buffer, sizeof(char), send_buffer_size, fp)) > 0){
    	unuse = write(data_socket, send_buffer, length);
    	total_length += length;
    	cout<<"Have transported "<<total_length<<" bytes"<<endl;
    }
    cout<<filepath<<" has already been transported!"<<endl;
    fclose(fp);
    shutdown(data_socket, SHUT_WR);
    close(data_socket);
    delete []send_buffer;
}

int main(int argc, char const *argv[])
{
    if (argc != 3){
    	cout<<"Incorrect input format: ./client <server_IP_address> port";
    	exit(0);
    }
    char buf[200];
    current_path = getcwd(buf, sizeof(buf));

    memset(&server_conmm_addr , 0 , sizeof(sockaddr_in));
    server_conmm_addr.sin_family = AF_INET;
    server_conmm_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_conmm_addr.sin_port = htons(atoi(argv[2]));

    conmm_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (conmm_socket == -1){
    	cout<<"Cannot create conmmand socket!"<<endl;
    	exit(0);
    }

    if (connect(conmm_socket, (struct sockaddr*) &server_conmm_addr, sizeof(server_conmm_addr)) == -1){
    	cout<<"Cannot get connected to"<<argv[1]<<":"<<argv[2]<<endl;
    	exit(0);
    }
    cout << "Successfully connect to" <<argv[1]<<":"<<argv[2]<<endl;

    while(1){
    	cout<<"FTP://"<<argv[1]<<":"<<argv[2]<<"  "<<"||";
    	get_input();
    	if (conmmand == "quit"){
    		quit_FTP();
    		break;
    	}
    	else if (conmmand.length() <= 3){
    		if (conmmand == "?" || conmmand == "pwd" || conmmand == "dir")
    			send_basic_conmmand();
    		else	cout<<"Your input format is wrong!"<<endl;
    	}
    	else {
    		if (conmmand.substr(0,3) == "cd ")
    		   send_basic_conmmand();
    		else if (conmmand.substr(0,4) == "get ")
    			 get_file(conmmand.substr(4), argv[1]);
    		else if (conmmand.substr(0,4) == "put ")
    			 put_file(conmmand.substr(4), argv[1]);
    		else	cout<<"Your input format is wrong!"<<endl;
    	}
    }

    return 0;
}
