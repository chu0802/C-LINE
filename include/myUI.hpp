#ifndef _MYUI_HPP_
#define _MYUI_HPP_

#include "protocol.hpp"

typedef enum{
	OP_NONE = 0xff,
	UNLOGIN_REGISTER = 0x01,
	UNLOGIN_LOGIN = 0x02,
	UNLOGIN_EXIT = 0x03,
} unLogin_opt;

typedef enum{
	LOGIN_SEND_MSG = 0x01,
	LOGIN_FRIEND = 0x02,
	LOGIN_SEND_FILE = 0x03,
	LOGIN_PERSONAL_SIGN = 0x04,
	LOGIN_LOGOUT = 0x05,
} Login_opt;

typedef enum{
	FRIEND_REQUEST = 0x90,
	FRIEND_HISTORY = 0x91,
} Friend_opt;

typedef enum{
	PERSONAL_WATCH = 0xa0,
	PERSONAL_SET = 0xa1,
} Personal_opt;

typedef enum{
	UI_UNLOGIN = 0x00,
	UI_UNLOGIN_REGISTER_NAME = 0x01,
	UI_UNLOGIN_REGISTER_PASSWD = 0x02,
	UI_UNLOGIN_REGISTER_REPASSWD = 0x03,
	
	UI_TRYLOGIN_NAME = 0x11,
	UI_TRYLOGIN_PASSWD = 0x12,
	
	UI_LOGIN = 0x20,
	UI_LOGIN_SENDMSG_NAME = 0x21,
	UI_LOGIN_SENDMSG = 0x22,
	
	UI_LOGIN_FRIEND = 0x30,
	UI_LOGIN_FRIEND_ADD = 0x31,
	UI_LOGIN_FRIEND_DELETE = 0x32,
	UI_LOGIN_FRIEND_HISTORY = 0x33,
	UI_LOGIN_FRIEND_CHECK = 0x34,

	UI_LOGIN_SENDFILE_NAME = 0x40,
	UI_LOGIN_SENDFILE_FILE = 0x41,

	UI_LOGIN_PERSONAL = 0x50,
	UI_LOGIN_PERSONAL_WATCH = 0x51,
	UI_LOGIN_PERSONAL_SET = 0x52,

	UI_ANS_FRIEND_REQUEST = 0x60,

	UI_ONLINE = 0xaa,
	UI_OFFLINE = 0xbb,
	UI_WAITING = 0x99,
} UI_Status;

class UI{
	public:
		UI();
		void init();
		void logout_init();
		void update();
		static string setCenter(string, char);//排版，可以決定兩邊要插入甚麼字元
		static string setCenter(string);//排版，預設是空白
		static string setRight(string);
		void to_login();//登入後的介面
		void to_unlogin();//登入前的介面
		void to_register();
		void to_try_login();
		void to_friend(vector<string>);
		void to_history(string, string, vector<string>);
		void to_personal(string);
		void to_watch_personal(vector<string>, string);
		void to_checking(vector<string>);
		void setLine();
		void setLine(string, int);
		void status(uint8_t);
		int status();
		void to_Error(string);
		static string get_ErrMsg(int);
		static string getCmd(int);
		void line_dump();//儲存當前的介面
		
		bool have_server_msg();
		void have_server_msg(bool);
		bool is_sending;
		bool is_getting;
		vector<string> server_msg;
	private:
		int _status;
		vector<string> title;
		vector<string> option;
		vector<string> line;
		bool _have_server_msg;
};


extern UI *myUI;
extern mutex UI_mtx;

#endif
