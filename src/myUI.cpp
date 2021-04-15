#include "myUI.hpp"
#include "protocol.hpp"

int HEIGHT;
int WIDTH;

void getSize(){
	struct winsize size;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&size);
	HEIGHT = size.ws_row - 6;
	WIDTH = size.ws_col;
}

UI::UI(){
	logout_init();
	getSize();

}

void UI::logout_init(){
	init();
	_status = UI_OFFLINE;
	_have_server_msg = false;
	server_msg.clear();
	for(int i = 0; i < 3; i++)
		server_msg.push_back("\n");
	is_sending = false;
	is_getting = false;
}

void UI::init(){
	getSize();
	line.clear();
	string tmp = "\n";
	for(int i = 0; i < HEIGHT; i++)
		line.push_back(tmp);
	line[0] = setCenter("YOYOYO~~CN_LINE") + "\n";

	title.clear();
	option.clear();
}

void UI::status(uint8_t n){
	 _status = int(n);
}

int UI::status(){
	 return _status;
}

string UI::setCenter(string str, char delim){
	getSize();
	int n = (WIDTH - str.length()) / 2;
	string l_space, r_space;
	for(int i = 0; i < n; i++)
		l_space += delim;
	for(int i = 0; i < n; i++)
		r_space += delim;
	if((l_space + str + r_space).length() != WIDTH)
		l_space += delim;
	return l_space + str + r_space;
}

string UI::setCenter(string str){
	 return setCenter(str, ' ');
}
string UI::setRight(string str){
	getSize();
	int n = WIDTH - str.length();
	string ret = "";
	for(int i = 0; i < n; i++)
		ret += " ";
	ret += str;
	return ret;
}

void UI::update(){
	lock_guard<mutex> mLock(UI_mtx);
	getSize();
	//pop first
	while(server_msg.size() > 3)
		server_msg.erase(server_msg.begin());

	cout<<"\n";
	for(auto &it:line)
		cout<<it;
	cout<<setCenter(" Server message ", '=')<<"\n";
	for(auto &it:server_msg){
		cout<<it;
		if(it[it.length()-1] != '\n')
			cout<<"\n";
	}
	cout<<setCenter(" Command ", '=')<<"\n";

	cout<<getCmd(_status);
	cout.flush();
	_have_server_msg = false;
}

void UI::setLine(){
	getSize();
	int start = 2;
	start = (HEIGHT - title.size() - option.size())/2;
	for(int i = 0; i < option.size(); i++){
		line[start+i] = setCenter(option[i]) + "\n";
	}
}

void UI::setLine(string str, int n){
	line[n] = str;
}


void UI::to_Error(string str){
	getSize();
	while(1){
		init();
		setLine(setCenter(str)+"\n", HEIGHT/2);
		setLine(setCenter("Type q to continue")+"\n", HEIGHT-1);
		update();
		string cmd;
		cin>>cmd;
		if(cmd == "q")
			return;
	}
}

void UI::to_unlogin(){
	init();
	option.push_back("1 for register");
	option.push_back("2 for login");
	option.push_back("3 for exit");

	setLine();
	update();
}

void UI::to_register(){
	init();
	setLine(setCenter("Register") + "\n", 2);
	update();
}

void UI::to_try_login(){
	init();
	setLine(setCenter("Login") + "\n", 2);
	update();
}

void UI::to_login(){
	init();
	option.push_back("1 for send message");
	option.push_back("2 for list your friend");
	option.push_back("3 for send file");
	option.push_back("4 for edit personal information");
	option.push_back("5 for logout");

	setLine();
	update();
}

void UI::to_personal(string str){
	init();
	setLine(setCenter(" "+str+" ", '~')+"\n", 2);
	option.push_back("1 for watching other's personal signature");
	option.push_back("2 for setting your personal signature");
	option.push_back("q for quit");

	setLine();
	update();
}

void UI::to_watch_personal(vector<string> fri, string str){
	init();
	setLine(setCenter(" "+str+" ", '~')+"\n", 2);
	for(int i = 1; i <= fri.size(); i++)
		option.push_back(to_string(i)+" for your friend: " + fri[i-1]);
	option.push_back("q for quit");
	setLine();
	update();
}

void UI::to_friend(vector<string> fri){
	init();
	for(int i = 1; i <= fri.size(); i++)
		option.push_back(to_string(i) + " for your friend: " + fri[i-1]);
	option.push_back("a for add a new friend");
	option.push_back("d for delete a friend");
	option.push_back("c for check new friend requests");
	option.push_back("q for quit");
	setLine();
	update();
}

void UI::to_history(string me, string you, vector<string> msg){
	init();
	string start = "send from " + you;
	for(int i = 0; i < WIDTH-you.length()-me.length()-20; i++)
		start += " ";
	start += "send from "+me;
	setLine(start + "\n", 2);
	for(int i = 0; i < msg.size(); i++){
		if(msg[i].find("from: "+me) != string::npos)
			setLine(setRight(msg[i].substr(msg[i].find("msg: ")+5)) + "\n", 4+i);
		else
			setLine(msg[i].substr(msg[i].find("msg: ")+5) + "\n", 4+i);
	}
	update();
}

void UI::to_checking(vector<string> fri){
	init();
	for(int i = 1; i <= fri.size(); i++)
		option.push_back(to_string(i) + " for your friend: " + fri[i-1]);
	option.push_back("q for quit");
	setLine();
	update();
}

bool UI::have_server_msg(){
	lock_guard<mutex> Lock(UI_mtx);
	return _have_server_msg;
}

void UI::have_server_msg(bool a){
	lock_guard<mutex> Lock(UI_mtx);
	_have_server_msg = a;
}

string UI::get_ErrMsg(int n){
	if(n == UI_NO_OPTION)
		return "No such option, please type 0 to retype.";
	if(n == UI_NAME_LEN_LIMIT)
		return "The length of name can be up to " + to_string(NAME_LEN_MAX-1) + ".";
	if(n == UI_PASSWD_LEN_LIMIT)
		return "The length of password can be up to " + to_string(PASSWD_LEN_MAX-1) + ".";
	if(n == UI_REGISTER_REPASSWD_WRONG)
		return "The passwords you typed do not match, please retype them.";
	if(n == UI_SEND_MSG_LEN_LIMIT)
		return "The length of message can be up to " + to_string(DATA_MAX-NAME_LEN_MAX) + ".";

}

string UI::getCmd(int n){
	if(n == UI_UNLOGIN)
		return "Select your option: ";
	if(n == UI_UNLOGIN_REGISTER_NAME)
		return "Name: ";
	if(n == UI_UNLOGIN_REGISTER_PASSWD)
		return "Password: ";
	if(n == UI_UNLOGIN_REGISTER_REPASSWD)
		return "Please type your password again: ";
	if(n == UI_TRYLOGIN_NAME)
		return "Name: ";
	if(n == UI_TRYLOGIN_PASSWD)
		return "Password: ";
	if(n == UI_LOGIN)
		return "Select your option: ";
	if(n == UI_LOGIN_SENDMSG_NAME)
		return "Name: ";
	if(n == UI_LOGIN_SENDMSG)
		return "Message: ";
	if(n == UI_LOGIN_FRIEND)
		return "Select your option: ";
	if(n == UI_LOGIN_FRIEND_ADD)
		return "Friend: ";
	if(n == UI_LOGIN_FRIEND_DELETE)
		return "Delete Friend: ";
	if(n == UI_LOGIN_FRIEND_HISTORY)
		return "Press q to return: ";
	if(n == UI_LOGIN_FRIEND_CHECK)
		return "Select your option: ";
	if(n == UI_LOGIN_SENDFILE_NAME)
		return "Name: ";
	if(n == UI_LOGIN_SENDFILE_FILE)
		return "File Name: ";
	if(n == UI_LOGIN_PERSONAL)
		return "Select your option: ";
	if(n == UI_LOGIN_PERSONAL_WATCH)
		return "Which one do you want to look: ";
	if(n == UI_LOGIN_PERSONAL_SET)
		return "Type your personal signature: ";

	if(n == UI_ANS_FRIEND_REQUEST)
		return "Type yes or no: ";

	if(n == UI_WAITING)
		return "Working..... please wait for a moment.\n";
	
}
