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

int unuse;

int rev_buffer_size;
int send_buffer_size;
char* rev_buffer;
char* send_buffer;

int conmmand_socket;
int data_socket;

string response;
string current_path;
string filepath;

short get_data_socket_port(){
	sockaddr_in data_addr;
	memset(&data_addr , 0 , sizeof(sockaddr_in));
	data_addr.sin_family = AF_INET;
	data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	data_addr.sin_port = 0;
	data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(data_socket, (sockaddr*)&data_addr, sizeof(sockaddr));
	listen(data_socket,1);
	socklen_t len = sizeof(sockaddr);
	getsockname(data_socket, (sockaddr*)&data_addr, &len);
	short port = data_addr.sin_port;
	return port;
}

void get_buffer_size(const int& cli_data_socket){
	socklen_t optlen = sizeof(int);
	getsockopt(cli_data_socket, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &optlen);
	getsockopt(cli_data_socket, SOL_SOCKET, SO_RCVBUF, &rev_buffer_size, &optlen);
}

void send_help(const int& cli_conmm_socket){
	response = "Usage:\n";
	response += "get [filename]   : download a file from current directory.\n";
	response += "put [filename]   : upload a file to the current directory.\n";;
	response += "pwd              : get current directory.\n";
	response += "dir              : list the file in the current directory.\n";
	response += "cd [folder]/[..] : go into a sub folder or go to the parent folder.\n";
	response += "?                : print this help message.\n";
	response += "quit             : exit.";
	unuse = write(cli_conmm_socket, response.c_str(), response.length());
}

void send_path(const int& cli_conmm_socket){
	unuse = write(cli_conmm_socket, (current_path+"/").c_str(), current_path.length() + 1);
}

void send_dir(const int& cli_conmm_socket){
	response = "The dir is:\n";
	DIR *dir;
	dirent *ptr;
	dir = opendir((current_path + "/").c_str());
	while ((ptr=readdir(dir)) != NULL) {
		if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
			continue;
		response += (ptr->d_name);
		response += "\n";
	}
	unuse = write(cli_conmm_socket, response.c_str(), response.length() - 1);
	closedir(dir);
}

void quit_FTP(const int& cli_conmm_socket){
	response = "Bye!";
	unuse = write(cli_conmm_socket, response.c_str(), response.length());
  	shutdown(cli_conmm_socket, SHUT_RDWR);
	close(cli_conmm_socket);
  	close(conmmand_socket);
	cout<<"Disconnect! Bye!"<<endl;
}

void cd_dir(const string& data, const int& cli_conmm_socket) {
	if (data == ".."){
		int position = current_path.rfind("/");
		if(position >= 0)
			current_path.erase(position);
		response = "Now, the path is ";
		response += current_path.c_str();
		response += "/";
		unuse = write(cli_conmm_socket, response.c_str(), response.length());
		return;
	}

	string temp_dir;
	if (data.substr(data.length() - 1) == "/")
		temp_dir = data.substr(0, data.length() - 1).c_str();
	else temp_dir = data.c_str();

	DIR* dir = opendir((current_path+"/"+temp_dir).c_str());
	if (dir == NULL){
		response = "Dir is not exist!";
		unuse = write(cli_conmm_socket, response.c_str(), response.length());
	}
	else{
		current_path = current_path + "/" + temp_dir.c_str();
		response = "Now, the path is ";
		response += current_path.c_str();
		response += "/";
		unuse = write(cli_conmm_socket, response.c_str(), response.length());
	}
	temp_dir.~string();
}

void send_file(const string& data, const int& cli_conmm_socket){
	filepath = current_path + "/" + data.c_str();
	FILE *fp = fopen(filepath.c_str(), "rb");
	if (!fp){
		response = filepath + " is not exist!";
		unuse = write(cli_conmm_socket, response.c_str(), response.length());
		cout<<filepath<<" is not exist!"<<endl;
		return;
	}
	else {
		response = "Start to transport ";
		response += (filepath + ".");
		unuse = write(cli_conmm_socket, response.c_str(), response.length());
	}

	  char ival[10];
	  unuse = read(cli_conmm_socket, &ival, sizeof(ival));
	  short port = get_data_socket_port();
	  unuse = write(cli_conmm_socket, &port, sizeof(port));

	  int cli_data_socket = accept(data_socket, (sockaddr*)NULL, (socklen_t *)NULL);
	  cout<<"Start to transport "<<filepath<<" via port "<<ntohs(port)<<endl;

	int length = 0;
	long total_length = 0;
	get_buffer_size(cli_data_socket);
	send_buffer = new char[send_buffer_size];
	while( (length = fread(send_buffer, sizeof(char), send_buffer_size, fp)) > 0){
		unuse = write(cli_data_socket, send_buffer, length);
		total_length += length;
		cout<<"Have transported "<<total_length<<" bytes"<<endl;
	}
	cout<<filepath<<" has already been transported!"<<endl;
	fclose(fp);
	shutdown(cli_data_socket, SHUT_WR);
	close(cli_data_socket);
	close(data_socket);
	delete []send_buffer;
}

void get_file(const string& data, const int& cli_conmm_socket){
	filepath = current_path + "/" + data.c_str();
	FILE *fp = fopen(filepath.c_str(), "wb");
	response = "Start to transport ";
	response += (filepath + ".");
	unuse = write(cli_conmm_socket, response.c_str(), response.length());

	char ival[10];
	unuse = read(cli_conmm_socket, &ival, sizeof(ival));
	short port = get_data_socket_port();
	unuse = write(cli_conmm_socket, &port, sizeof(port));

	int cli_data_socket = accept(data_socket, (sockaddr*)NULL, (socklen_t *)NULL);
	cout<<"Start to transport "<<filepath<<" via port "<<ntohs(port)<<endl;

	int length = 0;
	unsigned long total_length = 0;
	get_buffer_size(cli_data_socket);
	rev_buffer = new char[rev_buffer_size];
	while( (length = read(cli_data_socket, rev_buffer, rev_buffer_size)) > 0){
		fwrite(rev_buffer, sizeof(char), length, fp);
		total_length += length;
		cout<<"Have received "<<total_length<<" bytes"<<endl;
	}
	cout<<filepath<<" has already been received!"<<endl;
	fclose(fp);
	close(cli_data_socket);
	close(data_socket);
	delete []rev_buffer;
}

void unkown_error(const int& cli_conmm_socket){
	cout<<"Unkown error!"<<endl;
	close(cli_conmm_socket);
  	close(conmmand_socket);
	cout<<"Disconnect! Bye!"<<endl;

}

int main(int argc, char const *argv[]){
	if (argc != 2){
		cout<<"Incorrect input format: ./server <port>"<<endl;
		return(0);
	}

	char buf[200];
    current_path = getcwd(buf, sizeof(buf));

	conmmand_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (conmmand_socket == -1){
		cout<<"Cannot create conmmand socket!"<<endl;
		return(0);
	}

	sockaddr_in conmmand_addr;
	memset(&conmmand_addr , 0 , sizeof(sockaddr_in));
	conmmand_addr.sin_family = AF_INET;
	conmmand_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	conmmand_addr.sin_port = htons(atoi(argv[1]));

	if (bind(conmmand_socket, (sockaddr*)&conmmand_addr, sizeof(sockaddr)) == -1){
		cout<<"Cannot bing!"<<endl;
		return(0);
	}

	if (listen(conmmand_socket,10) == -1){
		cout<<"Cannot listen!"<<endl;
		return(0);
	}
	cout<<"Listening..."<<endl;

	sockaddr_in cli_conmm_addr;
	unsigned int cli_conmm_addr_l = sizeof(sockaddr);
	int cli_conmm_socket = accept(conmmand_socket, (sockaddr*)&cli_conmm_addr, (socklen_t *)&cli_conmm_addr_l);
	if (cli_conmm_socket == -1){
		cout<<"Cannot get ConmmandSocket connected from"<<inet_ntoa(cli_conmm_addr.sin_addr)<<":"<<ntohs(cli_conmm_addr.sin_port)<<endl;
		return(0);
	}
	else	cout<<"Succecssfully get ConmmandSocket connected from"<<inet_ntoa(cli_conmm_addr.sin_addr)<<":"<<ntohs(cli_conmm_addr.sin_port)<<endl;
	cout<<"Start FTP service to "<<inet_ntoa(cli_conmm_addr.sin_addr)<<endl;

	char* command_line = new char[1024];
	int count = 0;
	string command_str;
	while(1){
	    count = read(cli_conmm_socket, command_line, 1024);
	    command_str = command_line;
	    command_str = command_str.substr(0,count);
	    cout<<"User command: "<<command_str<<endl;

	    if (command_str == "quit"){
	    	quit_FTP(cli_conmm_socket);
	    	break;
	    }
	    else if (count <= 3){
	    	if (command_str == "?")
	    		send_help(cli_conmm_socket);
	    	else if (command_str == "pwd")
	    		send_path(cli_conmm_socket);
	    	else if (command_str == "dir")
	    		send_dir(cli_conmm_socket);
			else {
				unkown_error(cli_conmm_socket);
				break;
			}
	    }
	    else {
	    	if (command_str.substr(0,3) == "cd ")
	    		cd_dir(command_str.substr(3,count), cli_conmm_socket);
	    	else if (command_str.substr(0,4) == "get ")
	    		send_file(command_str.substr(4,count), cli_conmm_socket);
	    	else if (command_str.substr(0,4) == "put ")
	    		get_file(command_str.substr(4,count), cli_conmm_socket);
			else {
				unkown_error(cli_conmm_socket);
				break;
			}
	    }
	}
	return 0;
}
