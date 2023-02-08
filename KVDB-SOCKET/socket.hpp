#pragma once
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include<WinSock2.h>
#include <string>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#pragma comment(lib, "ws2_32.lib")
#define putMsg_error(error_) std::cout << "[" << __LINE__ << "]" << " : " << "(" << error_ << ")" << "failcode is " << WSAGetLastError() << std::endl;

bool init_Socket();

bool close_socket();

SOCKET creatsocket_server();

DWORD WINAPI serverthread(LPVOID lp);

void color_cout(int count, int color);

extern int count_cli;

extern std::unordered_map<std::string, int>taskwatch;

extern std::mutex tex;

class header
{
public:
	char magicnumber[4] = {};
	char size[4] = {};
	char type[4] = {};
	char padding[4] = {};
	int type_ = 0;

	bool socket_recv_header(SOCKET* client);

	bool reset_respond_header(int type);

	bool send_header(SOCKET* client);

	bool reset_respond_header_get(int size);

};

class recv_data_Put
{
public:
	char Key_Size[4];
	std::string Key;
	char Value_Size[4];
	std::string Value;

	bool socket_recv_data_Put(SOCKET* client);
};

class recv_data_Get
{
public:
	char Key_Size[4];
	std::string Key;

	bool socket_recv_data_Get(SOCKET* client);
};

class recv_data_Delete
{
public:
	char Key_Size[4];
	std::string Key;

	bool socket_recv_data_Delete(SOCKET* client);
};

class send_put_delete_respond
{
public:
	bool request_status;

	bool send_status(SOCKET* client, int type);

	void send_fail(SOCKET* client);
};

class KVdatabase_data
{
public:
	int keylen;
	std::string key_KV;
	int valuelen;
	std::string value;

	KVdatabase_data();

	void setdata_put(recv_data_Put* Put);

	void setdata_delete(recv_data_Delete* Delete);

	void setdata_get(recv_data_Get* Get);

};

class KVdatabase
{
public:
	std::unordered_map<std::string, int>index;			//哈希表索引

	KVdatabase();										//构造函数 读文件 设置索引

	bool updateKVdatabasefile();						//更新索引

	void KVputdata(KVdatabase_data* data_input);

	bool KVgetdata(KVdatabase_data* data_get);

	bool KVdelete(KVdatabase_data* data_delete);


};

class send_get_respond
{
public:
	char value_size[4];
	std::string value;

	bool value_respond_send(SOCKET* client, int type);

	void get_value_key(KVdatabase_data* data);

	void send_fail(SOCKET* client);
};

bool change_uint32(char* cstr, int* result);

void updatataskwatch(std::unordered_map<std::string, int>& taskwatch);

void seek_to_line(std::ifstream* in, int line);

std::string CharToStr(char* contentChar);

void DelLineData(const char* fileName, int lineNum);

int c_strtoint(char* c_str);

#endif // _SOCKET_H_
