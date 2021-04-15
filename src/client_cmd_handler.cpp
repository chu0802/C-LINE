#include "client_cmd_handler.hpp"
#include "protocol.hpp"
#include "myUI.hpp"

void my_cout(string str){
	std_mtx.lock();
	cout<<str;
	cout.flush();
	std_mtx.unlock();
}

void my_cerr(string str){
	std_mtx.lock();
	cerr<<str;
	std_mtx.unlock();
}

void send_file(int sockfd, string file_name, string target){
    ifstream fi(file_name, fstream::in | fstream::binary);
    if (fi.is_open()){
    	fi.seekg (0, fi.end);
    	int total_size = fi.tellg();
    	fi.seekg (0, fi.beg);
    	int count = 0;
    	int SIZE = DATA_MAX - 3*NAME_LEN_MAX;
    	while (count != total_size){
        	char buffer[SIZE];
        	memset(buffer,0,sizeof(buffer));
        	Request req(CLIENT, OP_SENDFILE, ST_REQ, user.getName());
        	fi.read(buffer,SIZE);
        	int get_size = fi.gcount();
        	count += get_size;

			if(count == get_size) //第1組
				req.header.status = ST_START_SEND;
			else if(get_size == SIZE) //還沒傳完
            	req.header.status = ST_SENDING;
        	else if(get_size < (SIZE) || count == total_size)//傳完了
            	req.header.status = ST_SEND_FIN;
            
        	req.setSendFile(file_name, target, buffer, get_size);
	    	sock_mtx.lock();
	    	int retval = send(sockfd, &req, sizeof(req), 0);
	    	sock_mtx.unlock();
	    	assert(retval == sizeof(req));
	    
    	}
    }
    else{
    	my_cout("no corresponding file\n");
    }
	fi.close();
    
	return;
}

string rot13(string str){
	string rot;
	for(int i = 0; i < str.length(); i++){
		char a = str[i];
		if (islower(a))
			a = (a - 'a' + 13) % 26 + 'a';
		else if (isupper(a))
			a = (a - 'A' + 13) % 26 + 'A';
		rot += a;
	}
	return rot;
}

void cmd_handle(int sockfd){
	int op;
	op = OP_NONE;
	while(1){
		if(myUI->status() == UI_OFFLINE){
			myUI->status(UI_UNLOGIN);
			myUI->to_unlogin();
			if(op == OP_NONE){
				string t_op;
				cin>>t_op;
				if(isdigit(t_op[0]))
					op = atoi(t_op.c_str());
				else{
					myUI->to_Error("No such option");
					myUI->status(UI_OFFLINE);
					op = OP_NONE;
					continue;
				}
			}
			if(op == UNLOGIN_REGISTER){
				myUI->status(UI_UNLOGIN_REGISTER_NAME);
				myUI->to_register();
				string n, pw, repw;
				cin>>n;
				myUI->status(UI_UNLOGIN_REGISTER_PASSWD);
				myUI->update();
				cin>>pw;
				myUI->status(UI_UNLOGIN_REGISTER_REPASSWD);
				myUI->update();
				cin>>repw;

				Request req(CLIENT, OP_REGISTER, ST_REQ);
				myUI->status(UI_WAITING);
				myUI->update();
				int success = req.setRegister(n, pw, repw);

				if(success == UI_REGISTER_SUCCESS){
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}
				else
					myUI->to_Error(UI::get_ErrMsg(success));
				while(!myUI->have_server_msg());
				myUI->status(UI_OFFLINE);
			}
			else if(op == UNLOGIN_LOGIN){
				myUI->status(UI_TRYLOGIN_NAME);
				myUI->to_try_login();
				string n, pw;
				cin>>n;
				myUI->status(UI_TRYLOGIN_PASSWD);
				myUI->update();
				cin>>pw;
				Request req(CLIENT, OP_LOGIN, ST_REQ, user.getName());
				
				myUI->status(UI_WAITING);
				myUI->update();
				int success = req.setLogin(n, pw);
				
				if(success == UI_LOGIN_SUCCESS){
					user.setName(n);
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}
				else
					myUI->to_Error(UI::get_ErrMsg(success));
				while(!myUI->have_server_msg());
			}
			else if(op == UNLOGIN_EXIT){
				myUI->to_Error("Goodbye~");
				exit(0);
			}
			else{
				myUI->to_Error("No such option");
				myUI->status(UI_OFFLINE);
				op = OP_NONE;
			}

			op = OP_NONE;
			
		}
		else if(myUI->status() == UI_ONLINE){
			if(op == OP_NONE){
				myUI->status(UI_LOGIN);
				myUI->to_login();
	
				if(op == OP_NONE){
					string t_op;
					cin>>t_op;
					if(isdigit(t_op[0]))
						op = atoi(t_op.c_str());
					else{
						myUI->to_Error("No such option");
						myUI->status(UI_ONLINE);
						op = OP_NONE;
						continue;
					}
				}

			}

			if(op == LOGIN_SEND_MSG){
				string n, msg;
				myUI->status(UI_LOGIN_SENDMSG_NAME);
				myUI->update();
				cin>>n;
				myUI->status(UI_LOGIN_SENDMSG);
				myUI->update();
				cin.ignore(100, '\n');
				getline(cin, msg);

				msg = rot13(msg);

				myUI->status(UI_WAITING);
				myUI->update();

				Request req(CLIENT, OP_SEND, ST_REQ, user.getName());
				int success = req.setSend(n, msg);

				if(success == UI_SEND_MSG_SUCCESS){
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}
				else{
					myUI->to_Error(UI::get_ErrMsg(success));
					op = OP_NONE;
					myUI->status(UI_ONLINE);
					continue;
				}
				while(!myUI->have_server_msg());
				op = OP_NONE;
				myUI->status(UI_ONLINE);
			}
			else if(op == LOGIN_FRIEND){
				if(op == LOGIN_FRIEND){
					myUI->status(UI_WAITING);
					myUI->update();
					Request req(CLIENT, OP_LISTFRIEND, ST_REQ, user.getName());
					int success = req.setListFriend(user.getName());
					if(success){
						sock_mtx.lock();
						int retval = send(sockfd, &req, sizeof(req), 0);
						sock_mtx.unlock();
						assert(retval == sizeof(req));
					}
					while(!myUI->have_server_msg());
					myUI->status(UI_LOGIN_FRIEND);
					myUI->to_friend(user.myFriend);		
					op = FRIEND_REQUEST;
				}
				if(op == FRIEND_REQUEST){
					string tmp_op;
					cin>>tmp_op;

					bool is_digit = false;
					for(int i = 0; i < user.myFriend.size(); i++){
						if(tmp_op == to_string(i+1)){
							Request req(CLIENT, OP_HISTORY, ST_REQ, user.getName());
							int success = req.setHistory(user.myFriend[i]);
							if(success){
								sock_mtx.lock();
								int retval = send(sockfd, &req, sizeof(req), 0);
								sock_mtx.unlock();
								assert(retval == sizeof(req));
							}
							while(!myUI->have_server_msg());
							myUI->to_history(user.getName(), user.myFriend[i], user.Friend_Msg);
							op = FRIEND_HISTORY;
							is_digit = true;
							break;
						}
					}

					if(tmp_op == "a"){
						string add_fr;
						myUI->status(UI_LOGIN_FRIEND_ADD);
						myUI->update();
						cin>>add_fr;
						Request req(CLIENT, OP_ADDFRIEND, ST_REQ, user.getName());
						int success = req.setAddFriend(user.getName(), add_fr);

						if(success == UI_ADD_FRIEND_SUCCESS){
							sock_mtx.lock();
							int retval = send(sockfd, &req, sizeof(req), 0);
							sock_mtx.unlock();
							assert(retval == sizeof(req));
						}
						else{
							myUI->to_Error(UI::get_ErrMsg(success));
							op = OP_NONE;
							myUI->status(UI_ONLINE);
							continue;
						}
						while(!myUI->have_server_msg());
						myUI->status(UI_ONLINE);
						op = LOGIN_FRIEND;
						continue;
					}
					else if(tmp_op == "d"){
						string del_fr;
						myUI->status(UI_LOGIN_FRIEND_DELETE);
						myUI->update();
						cin>>del_fr;
						Request req(CLIENT, OP_DELETEFRIEND, ST_REQ, user.getName());

						int success = req.setDeleteFriend(user.getName(), del_fr);
						if(success == UI_DELETE_FRIEND_SUCCESS){
							sock_mtx.lock();
							int retval = send(sockfd, &req, sizeof(req), 0);
							sock_mtx.unlock();
							assert(retval == sizeof(req));
						}
						else{
							myUI->to_Error(UI::get_ErrMsg(success));
							op = OP_NONE;
							myUI->status(UI_ONLINE);
							continue;
						}
						while(!myUI->have_server_msg());
						myUI->status(UI_ONLINE);
						op = LOGIN_FRIEND;
						continue;
					}
					else if(tmp_op == "c"){
						myUI->status(UI_WAITING);
						myUI->update();
						Request req(CLIENT, OP_CHECKFRIEND, ST_REQ, user.getName());
						int success = req.setCheckFriend(user.getName());
						if(success){
							sock_mtx.lock();
							int retval = send(sockfd, &req, sizeof(req), 0);
							sock_mtx.unlock();
							assert(retval == sizeof(req));
						}
						while(!myUI->have_server_msg());
						while(1){
							myUI->status(UI_LOGIN_FRIEND_CHECK);
							myUI->to_checking(user.myFriend);
							string tmp_op;
							cin>>tmp_op;
							for(int i = 0; i < user.myFriend.size(); i++){
								if(tmp_op == to_string(i+1)){
									Request req(CLIENT, OP_ADDFRIEND, ST_REQ, user.getName());
									int success = req.setAddFriend(user.getName(), user.myFriend[i]);
									if(success){
										sock_mtx.lock();
										int retval = send(sockfd, &req, sizeof(req), 0);
										sock_mtx.unlock();
										assert(retval == sizeof(req));
									}
									while(!myUI->have_server_msg());
									break;
								}
							}

							if(tmp_op == "q"){
								op = LOGIN_FRIEND;
								myUI->status(UI_ONLINE);
								break;
							}
						}

					}
					else if(tmp_op == "q"){
						op = OP_NONE;
						myUI->status(UI_ONLINE);
						continue;
					}
					else{
						if(!is_digit){
							myUI->to_Error("No such option");
							myUI->status(UI_ONLINE);
							op = LOGIN_FRIEND;
							continue;
						}
					}

				}
				if(op == FRIEND_HISTORY){
					user.Friend_Msg.clear();
					while(1){
						myUI->status(UI_LOGIN_FRIEND_HISTORY);
						myUI->update();
						string tmp_op;
						cin>>tmp_op;
						if(tmp_op == "q"){
							op = LOGIN_FRIEND;
							myUI->status(UI_ONLINE);
							break;
						}
					}
					continue;
				}
			}
			else if(op == LOGIN_SEND_FILE){
				string file_name, target;
				myUI->status(UI_LOGIN_SENDFILE_NAME);
				myUI->update();
				cin >> target;
				if(target.length() > NAME_LEN_MAX - 1){
					myUI->to_Error(UI::get_ErrMsg(UI_NAME_LEN_LIMIT));
					op = OP_NONE;
					myUI->status(UI_ONLINE);
					continue;
				}
				myUI->status(UI_LOGIN_SENDFILE_FILE);
				myUI->update(); 
				cin >> file_name;
				if(file_name.length() > NAME_LEN_MAX - 1){
					myUI->to_Error(UI::get_ErrMsg(UI_NAME_LEN_LIMIT));
					op = OP_NONE;
					myUI->status(UI_ONLINE);

					continue;
				}
				
				thread thread_send_file(send_file, sockfd, file_name, target);
				thread_send_file.detach();

				op = OP_NONE;
				myUI->status(UI_ONLINE);
				continue;
			}
			else if(op == LOGIN_PERSONAL_SIGN){
				if(op == LOGIN_PERSONAL_SIGN){
					myUI->status(UI_WAITING);
					myUI->update();
					Request req(CLIENT, OP_WATCHPERSONAL, ST_REQ, user.getName());
					int success = req.watchPersonal(user.getName(), user.getName());
					if(success){
						sock_mtx.lock();
						int retval = send(sockfd, &req, sizeof(req), 0);
						sock_mtx.unlock();
						assert(retval == sizeof(req));
					}
					else{
						op = OP_NONE;
						myUI->status(UI_ONLINE);
						continue;
					}
					while(!myUI->have_server_msg());

					myUI->status(UI_LOGIN_PERSONAL);
					myUI->to_personal(user.per_sign);
					string tmp_c;
					cin>>tmp_c;
					if(tmp_c == "1")
						op = PERSONAL_WATCH;
					else if(tmp_c == "2")
						op = PERSONAL_SET;
					else if(tmp_c == "q"){
						op = OP_NONE;
						myUI->status(UI_ONLINE);
						continue;
					}
					else{
						myUI->status(UI_ONLINE);
						op = LOGIN_PERSONAL_SIGN;
						continue;
					}
				}

				if(op == PERSONAL_WATCH){
					myUI->update();
					Request req(CLIENT, OP_LISTFRIEND, ST_REQ, user.getName());
					int success = req.setListFriend(user.getName());
					if(success){
						sock_mtx.lock();
						int retval = send(sockfd, &req, sizeof(req), 0);
						sock_mtx.unlock();
						assert(retval == sizeof(req));
					}
					else{
						op = OP_NONE;
						myUI->status(UI_ONLINE);
						continue;
					}
					while(!myUI->have_server_msg());
					myUI->status(UI_LOGIN_PERSONAL_WATCH);
					myUI->to_watch_personal(user.myFriend, user.per_sign);

					string tmp_per_c;
					cin>>tmp_per_c;
					for(int i = 0; i < user.myFriend.size(); i++)
						if(tmp_per_c == to_string(i+1)){
							Request req(CLIENT, OP_WATCHPERSONAL, ST_REQ, user.getName());
							int success = req.watchPersonal(user.getName(),user.myFriend[i]);
							if(success){
								sock_mtx.lock();
								int retval = send(sockfd, &req, sizeof(req), 0);
								sock_mtx.unlock();
								assert(retval == sizeof(req));
								break;
							}
						}
					if(tmp_per_c == "q"){
						op = LOGIN_PERSONAL_SIGN;
						myUI->status(UI_ONLINE);
						continue;
					}
					while(!myUI->have_server_msg());
					op = LOGIN_PERSONAL_SIGN;
					myUI->status(UI_ONLINE);
					continue;
				}
				else if(op == PERSONAL_SET){
					myUI->status(UI_LOGIN_PERSONAL_SET);
					myUI->update();

					string signature;
					cin.ignore(100, '\n');
					getline(cin, signature);
					Request req(CLIENT, OP_SETPERSONAL, ST_REQ, user.getName());
					int success = req.setPersonal(user.getName(),signature);
					if(success){
						sock_mtx.lock();
						int retval = send(sockfd, &req, sizeof(req), 0);
						sock_mtx.unlock();
						assert(retval == sizeof(req));
					}

					while(!myUI->have_server_msg());
					op = LOGIN_PERSONAL_SIGN;
					myUI->update();
					myUI->status(UI_ONLINE);
					continue;
				}
			}
			else if(op == LOGIN_LOGOUT){
				Request req(CLIENT, OP_LOGOUT, ST_REQ, user.getName());
				int success = req.setLogout(user.getName());
				if(success){
					sock_mtx.lock();
					int retval = send(sockfd, &req, sizeof(req), 0);
					sock_mtx.unlock();
					assert(retval == sizeof(req));
				}

				
				user.setLogout();
				op = OP_NONE;
				myUI->status(UI_OFFLINE);
				myUI->logout_init();
				continue;
			}
			else{
				myUI->to_Error("No such option");
				myUI->status(UI_ONLINE);
				op = OP_NONE;
			}

		}

	}
}

int req_handle(Request *req,int sockfd){
	char data[DATA_MAX];
	memcpy(data, req->getData(), DATA_MAX);
	//不太可能遇到的問題
	if(req->header.from != SERVER){
		myUI->to_Error("SERVER_WRONG\n");
		return 0;
	}


	if(req->header.op == OP_REGISTER_ANS){
		myUI->server_msg.push_back(string("SERVER: ") + data);
		myUI->have_server_msg(true);
		if(req->header.status == ST_OK){
			//成功就成功，甚麼事都不用做（也不用輸出錯誤訊息，server會回傳)
			return 1;
		}
	}
	else if(req->header.op == OP_LOGIN_ANS){
		myUI->server_msg.push_back(string("SERVER: ") + data);
		myUI->have_server_msg(true);
		if(req->header.status == ST_OK){  //多餘的if判斷式
			myUI->status(UI_ONLINE);
			user.setLogin();
			return 1;
		}
		else
			myUI->status(UI_OFFLINE);
	}
	else if (req->header.op == OP_SEND_ANS){
		myUI->server_msg.push_back(string("SERVER: ") + data);
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}	
	else if (req->header.op == OP_SERVER_SEND){
		if (req->header.status == ST_OK){
			char name[NAME_LEN_MAX];
			char msg[DATA_MAX - NAME_LEN_MAX];
			memcpy(name,data,NAME_LEN_MAX);
			memcpy(msg,&data[NAME_LEN_MAX],sizeof(msg));
			string final_msg = rot13(msg);
			string m = string("from: ") + name + string(", msg: ") + final_msg + string("\n");
		//	string m = string("SERVER: new message send from ")+name;
			myUI->server_msg.push_back(m);
			myUI->have_server_msg(true);
			myUI->update();

			return 1;					
		}
	}
	else if (req->header.op == OP_ADDFRIEND_ANS){
		//加朋友成功
		myUI->server_msg.push_back(string("SERVER: ") + data);
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_HISTORY_ANS){
		if(req->header.status == ST_OK){
			string str_data = string(data);
			string tmp_msg = rot13(str_data.substr(str_data.find("msg: ")+5));
			string payload = str_data.substr(0, str_data.find("msg: ")+5);
			user.Friend_Msg.push_back(payload + tmp_msg);
		}
		else if(req->header.status == ST_FIN){
			myUI->have_server_msg(true);
		}
		return 1;
	}
	else if(req->header.op == OP_SERVER_SENDING){
		if(!myUI->is_getting){
			myUI->server_msg.push_back(string("SERVER: ") + "get file....");
			myUI->is_getting = true;
			myUI->update();
		}
	    char file_name[NAME_LEN_MAX];
        memcpy(file_name, data, NAME_LEN_MAX);
        ofstream f((user.getName() + "_" + file_name), fstream::out | fstream::app | fstream::binary);
		int size = atoi(&data[NAME_LEN_MAX*2]);

		f.write(&data[NAME_LEN_MAX*3], size);
	    f.close();
	    return 1;
	}
	else if(req->header.op == OP_SERVER_SEND_FIN){
	    char file_name[NAME_LEN_MAX];
        memcpy(file_name, data, NAME_LEN_MAX);
        ofstream fr((user.getName() + "_" + file_name), fstream::out | fstream::app | fstream::binary);
		
		int size = atoi(&data[NAME_LEN_MAX*2]);

		fr.write(&data[NAME_LEN_MAX*3], size);
	    fr.close();
		myUI->server_msg.push_back("SERVER: you get a file!");
		myUI->is_getting = false;
		myUI->update();
	    return 1;
	}
	else if(req->header.op == OP_SENDING_ANS){
		if(!myUI->is_sending){
			myUI->server_msg.push_back("SERVER: sending...");
			myUI->is_sending = true;
			myUI->update();
		}
		
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_SEND_FIN_ANS){
		myUI->server_msg.push_back("SERVER: sending finish!");
		myUI->is_sending = false;
		myUI->update();
		if (req->header.status == ST_OK){
			return 1;
		}

	}
	else if(req->header.op == OP_ERR_MSG){//單純server想跟client聊聊天
		myUI->server_msg.push_back(string("SERVER: ") + data);
		myUI->update();
		return 1;
	}

	else if(req->header.op == OP_FRIEND_REQUEST){
		if(req->header.status == ST_NEW_FRIEND){
			myUI->server_msg.push_back("New friend request!");
			myUI->update();
		}
		return 1;
	}

	else if(req->header.op == OP_SETPERSONAL_ANS){
		myUI->server_msg.push_back(string(data));
		myUI->have_server_msg(true);
		string payload = "your signature is ";
		string sign = string(data).substr(payload.length());
		user.per_sign = sign;
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_WATCHPERSONAL_ANS){
		char target_name[NAME_LEN_MAX];
		char per[DATA_MAX-NAME_LEN_MAX];
		memcpy(target_name, data, NAME_LEN_MAX);
		memcpy(per, &data[NAME_LEN_MAX], DATA_MAX-NAME_LEN_MAX);
		if(user.getName() == string(target_name)){
			if(req->header.status == ST_OK)
				user.per_sign = per;
			myUI->have_server_msg(true);
			return 1;
		}
		myUI->server_msg.push_back(string(target_name) + ": " + string(per));
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_DELETEFRIEND_ANS){
		//刪朋友成功
		myUI->server_msg.push_back(string("SERVER: delete successed!"));
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if (req->header.op == OP_LISTFRIEND_ANS){
		string str;
		string line;
		line = string(data);
		user.myFriend.clear();
		while(line.size()>0){
			str = line.substr(0, line.find(":"));
			line = line.substr(line.find(":") + 1);
			user.myFriend.push_back(str);
		}
		
		myUI->server_msg.push_back("SERVER: These are your friends.");
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_CHECKFRIEND_ANS){
		string str;
		string line;
		line = string(data);
		user.myFriend.clear();
		while(line.size() > 0){
			str = line.substr(0, line.find(":"));
			line = line.substr(line.find(":") + 1);
			user.myFriend.push_back(str);
		}
		myUI->server_msg.push_back("SERVER: Your Friend Request.");
		myUI->have_server_msg(true);
		if (req->header.status == ST_OK){
			return 1;
		}
	}
	else if(req->header.op == OP_LOGOUT_ANS){
		myUI->server_msg.push_back("SERVER: " + string(data));
		myUI->have_server_msg(true);

		if(req->header.status == ST_OK){
			return 1;
		}
	}


}

