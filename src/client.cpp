#include "protocol.hpp"
#include "client_cmd_handler.hpp"
#include "myUI.hpp"

UI *myUI = new UI;
User user;
mutex std_mtx; //處理stdout, stderr同時使用的mutex
mutex sock_mtx; //處理send, recv同時使用的mutex
mutex in_mtx;
mutex UI_mtx;
char Sock_Buf[SOCK_BUF_SIZE];
int Buf_Pointer = 0;

int server_connect(string domain, string port){
	int sockfd = 0;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		cerr<<"Fail to create a socket.\n";
		exit(0);
	}

	struct sockaddr_in info;
	bzero(&info, sizeof(info));
	info.sin_family = PF_INET;

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(domain.c_str(), port.c_str(), &hints, &res) != 0){
		cerr<<"get info error.\n";
		exit(1);
	}

	string s_ip = inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr);
	info.sin_addr.s_addr = inet_addr(s_ip.c_str());
	info.sin_port = htons(atoi(port.c_str()));
	while(connect(sockfd, (struct sockaddr *)&info, sizeof(info)) == -1);
	return sockfd;
}

int main(int argc, char **argv){
	memset(Sock_Buf, 0, sizeof(Sock_Buf));
	if(argc != 3){
		cerr<<"usage: ./client [host] [port]\n";
		exit(-1);
	}
	string domain = argv[1];
	string port = argv[2];
	int sockfd = server_connect(domain, port);	
	thread stdin_handle(cmd_handle, sockfd);

	while(1){
		char recv_buf[sizeof(Request)];
		memset(recv_buf, 0, sizeof(Request));

		ssize_t recv_val = recv(sockfd, recv_buf, sizeof(Request), 0);
		if(recv_val <= 0){
			sockfd = server_connect(domain, port);
		}
		memcpy(&Sock_Buf[Buf_Pointer], recv_buf, recv_val);
		Buf_Pointer += recv_val;

		bool Start_Correct = false, End_Correct = false;
		char start_cmp[4], end_cmp[4];
		memset(start_cmp, START_MARK, MARK_SIZE);
		memset(end_cmp, END_MARK, MARK_SIZE);

		if(memcmp(Sock_Buf, start_cmp, MARK_SIZE) == 0)
			Start_Correct = true;
		if(memcmp(&Sock_Buf[sizeof(Request)-MARK_SIZE], end_cmp, MARK_SIZE) == 0)
			End_Correct = true;
		if(Start_Correct && End_Correct){
			Request *recv_msg = new Request(0, 0, 0);

			memcpy(recv_msg, Sock_Buf, sizeof(Request));
			req_handle(recv_msg, sockfd);

			delete recv_msg;
			recv_msg = NULL;

			char tmp_buf[SOCK_BUF_SIZE - sizeof(Request)];
			memset(tmp_buf, 0, sizeof(tmp_buf));
			memcpy(tmp_buf, &Sock_Buf[sizeof(Request)], sizeof(tmp_buf));
			memset(Sock_Buf, 0, sizeof(Sock_Buf));
			memcpy(Sock_Buf, tmp_buf, sizeof(tmp_buf));

			Buf_Pointer -= sizeof(Request);
		}
	}
	stdin_handle.join();
	close(sockfd);
	delete myUI;
	return 0;
}
