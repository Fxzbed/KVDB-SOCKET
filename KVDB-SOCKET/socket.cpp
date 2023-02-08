#include "socket.hpp"
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <stdlib.h>
#include <mutex>
/*
* 协议传输类型：
* 0 put请求
* 1 delete请求
* 2 Get请求
* 3 put响应
* 4 delete响应
* 5 Get响应
*/

int count_cli;             //计数连接的cli

std::mutex tex;

std::unordered_map<std::string, int>taskwatch;

bool init_Socket()
{
	WSAData wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		putMsg_error("WSAStartup");
		return false;
	}
	else
	{
		std::cout << "WSAStartup sussess" << std::endl;
		return true;
	}
}

bool close_socket()
{
	if (0 != WSACleanup())
	{
		putMsg_error("WSASCleanup");
		return false;
	}
	else
	{
		std::cout << "WSACleanup success" << std::endl;
		return true;
	}
}

SOCKET creatsocket_server()
{
	SOCKET server;        //空的socket
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == INVALID_SOCKET)
	{
		putMsg_error("socket");
		return INVALID_SOCKET;
	}
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(25565);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(server, (sockaddr*)&addr, sizeof(addr)))
	{
		putMsg_error("bind");
		return false;
	}
	listen(server, 10);
	return server;
}

DWORD WINAPI serverthread(LPVOID lp)                //线程中执行的函数
{
	int i = 5;
	int total_size = 0;
	int id_thread = count_cli;
	std::cout << "thread" << "[" << id_thread << "]" << "created!" << std::endl;
	SOCKET* client = (SOCKET*)lp;
	header thread_recv_header;
	while (i --)
	{
		if (thread_recv_header.socket_recv_header(client))				//在这里判断recv到的类型
		{
			change_uint32(thread_recv_header.type, &thread_recv_header.type_);
			switch (thread_recv_header.type_)
			{
			case 0:
			{
				send_put_delete_respond respond;
				recv_data_Put Put;
				respond.request_status = Put.socket_recv_data_Put(client);
				KVdatabase_data KVDB_data;
				tex.lock();
				taskwatch[KVDB_data.key_KV] = 0;
				KVDB_data.setdata_put(&Put);
				KVdatabase KVDB;
				KVDB.KVputdata(&KVDB_data);
				taskwatch[KVDB_data.key_KV] = 1;
				tex.unlock();
				thread_recv_header.reset_respond_header(3);
				thread_recv_header.send_header(client);
				respond.send_status(client, 3);
				break;
			}
			case 1:
			{
				send_put_delete_respond respond;
				recv_data_Delete Delete;
				respond.request_status = Delete.socket_recv_data_Delete(client);
				KVdatabase_data KVDB_data;
				KVDB_data.setdata_delete(&Delete);
			/*	tex.lock();
				while (!taskwatch[KVDB_data.key_KV])
				{	
					updatataskwatch(taskwatch);
					tex.unlock();
					Sleep(1);
				}*/
				tex.lock();
				KVdatabase KVDB;
				if (!KVDB.KVdelete(&KVDB_data))
				{
					tex.unlock();
					thread_recv_header.reset_respond_header(4);
					respond.send_fail(client);
					break;
				}
				Sleep(1);
				tex.unlock();
				thread_recv_header.reset_respond_header(4);
				thread_recv_header.send_header(client);
				respond.send_status(client, 4);
				break;
			}
			case 2:
			{
				char* mid;        //临时value
				send_get_respond respond;
				recv_data_Get Get;
				Get.socket_recv_data_Get(client);
				KVdatabase_data KVDB_get;
				KVDB_get.setdata_get(&Get);
				while (1)
				{
					tex.lock();
					if (taskwatch[KVDB_get.key_KV])
					{
						tex.unlock();
						break;
					}
					updatataskwatch(taskwatch);
					tex.unlock();
					Sleep(100);
				}
				
				tex.lock();
				KVdatabase KVDB;
				KVDB.KVgetdata(&KVDB_get);
				tex.unlock();
				total_size = KVDB_get.valuelen + 4;
				respond.get_value_key(&KVDB_get);
				thread_recv_header.reset_respond_header_get(total_size);
				thread_recv_header.send_header(client);
				respond.value_respond_send(client, 5);
			}
			}
		}
		else
		{
			std::cout << "break" << std::endl;
			return NULL;
		}
	}
	std::cout << "break" << std::endl;
	return NULL;
}

void color_cout(int count, int color)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | color);
	std::cout << count;
	SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | 7);
}


bool header::socket_recv_header(SOCKET* client)
{
	recv(*client, this->magicnumber, 4, 0);
	recv(*client, this->size, 4, 0);
	recv(*client, this->type, 4, 0);
	recv(*client, this->padding, 4, 0);
	return true;
}

bool header::reset_respond_header(int type)
{
	this->magicnumber[0] = -46;
	this->magicnumber[1] = 4;
	this->magicnumber[2] = 0;
	this->magicnumber[3] = 0;
	if (type == 3)
		this->type[0] = 3;
	if (type == 4)
		this->type[0] = 4;
	if (type == 5)
		this->type[0] = 5;
	if (type == 3 || type == 4)
	{
		for (int i = 1; i < 4; i++)
		{
			this->type[i] = 0;
		}
		this->size[0] = 1;
		for (int i = 1; i < 4; i++)
		{
			this->size[i] = 0;
		}
		for (int i = 0; i < 4; i++)
		{
			this->padding[i] = 0;
		}
	}
	return true;
}

bool header::send_header(SOCKET* client)
{
	send(*client, this->magicnumber, 4, 0);
	send(*client, this->size, 4, 0);
	send(*client, this->type, 4, 0);
	send(*client, this->padding, 4, 0);
	return true;
}

bool header::reset_respond_header_get(int size)
{
	this->magicnumber[0] = -46;
	this->magicnumber[1] = 4;
	this->magicnumber[2] = 0;
	this->magicnumber[3] = 0;
	this->type[0] = 5;
	for (int i = 1; i < 4; i++)
	{
		this->type[i] = 0;
	}
	this->size[0] = size;
	for (int i = 1; i < 4; i++)
	{
		this->size[i] = 0;
	}
	for (int i = 0; i < 4; i++)
	{
		this->padding[i] = 0;
	}
	return true;
}

bool recv_data_Put::socket_recv_data_Put(SOCKET* client)
{
	char* temp_cstr;
	int length;
	if (recv(*client, this->Key_Size, 4, 0) == 4)
	{
		if (!change_uint32(this->Key_Size, &length))
			goto back_false;
		temp_cstr = (char*)malloc(sizeof(char) * length);
	}
	else
		goto back_false;
	if (recv(*client, temp_cstr, length, 0) == length)
	{
		for (int i = 0; i < length; i++)
		{
			this->Key.push_back(temp_cstr[i]);
		}
		free(temp_cstr);
	}
	else
		goto back_false;
	if (recv(*client, this->Value_Size, 4, 0) == 4)
	{
		if (!change_uint32(this->Value_Size, &length))
			goto back_false;
		temp_cstr = (char*)malloc(sizeof(char) * length);
	}
	else
		goto back_false;
	if (recv(*client, temp_cstr, length, 0) == length)
	{
		for (int i = 0; i < length; i++)
		{
			this->Value.push_back(temp_cstr[i]);
		}
		free(temp_cstr);
	}
	else
		goto back_false;

back_false:
	return true;
}

bool recv_data_Get::socket_recv_data_Get(SOCKET* client)
{
	char* temp_cstr;
	int length;
	if (recv(*client, this->Key_Size, 4, 0) == 4)
	{
		if (!change_uint32(this->Key_Size, &length))
			goto back_false;
		temp_cstr = (char*)malloc(sizeof(char) * length);
	}
	else
		goto back_false;
	if (recv(*client, temp_cstr, length, 0) == length)
	{
		for (int i = 0; i < length; i++)
		{
			this->Key.push_back(temp_cstr[i]);
		}
		free(temp_cstr);
	}
	else
		goto back_false;
back_false:
	return false;
}

bool recv_data_Delete::socket_recv_data_Delete(SOCKET* client)
{
	char* temp_cstr;
	int length;
	if (recv(*client, this->Key_Size, 4, 0) == 4)
	{
		if (!change_uint32(this->Key_Size, &length))
			goto back_false;
		temp_cstr = (char*)malloc(sizeof(char) * length);
	}
	else
		goto back_false;
	if (recv(*client, temp_cstr, length, 0) == length)
	{
		for (int i = 0; i < length; i++)
		{
			this->Key.push_back(temp_cstr[i]);
		}
		free(temp_cstr);
	}
	else
		goto back_false;
back_false:
	return false;
}

bool change_uint32(char* cstr, int* result)
{
	*result = 0;
	for (int i = 0; i < 4; i++)
	{
		*result += cstr[i];
	}
	return true;
}

void updatataskwatch(std::unordered_map<std::string, int>& taskwatch)
{
	if ((fopen("KVdatabasefile.txt", "rb")) != NULL)
	{
		char push_mid;
		std::ifstream file_read;
		if (file_read.eof())
		{
			taskwatch.clear();
			return;
		}
		std::string mid;
		int mid_ = 1;
		file_read.open("KVdatabasefile.txt", std::ios::in);
		while (1)
		{
			push_mid = file_read.get();
		string_key:
			if (push_mid != ' ')
				mid.push_back(push_mid);
			else
				goto string_offset;
		}
	string_offset:
		while (1)
		{
			push_mid = file_read.get();
			if (push_mid == '\n')
				goto data_save_index;
		}
	data_save_index:
		taskwatch[mid] = mid_;
		mid.clear();
		push_mid = file_read.get();
		if (push_mid == -1)    //文件读取完毕
		{
			file_read.close();
		}
		else
			goto string_key;
	}
}

bool send_put_delete_respond::send_status(SOCKET* client, int type)
{
	char buf[1];
	if (this->request_status)
		buf[0] = 1;
	send(*client, buf, 1, 0);
	if ((int)type != 3 && (int)type != 4)
		return false;
	return true;
}

void send_put_delete_respond::send_fail(SOCKET* client)
{
	send(*client, 0, 1, 0);
}

bool send_get_respond::value_respond_send(SOCKET* client, int type)
{
	char* mid;
	int length;
	switch (type)
	{
	case 5:
		send(*client, this->value_size, 4, 0);
		if (!change_uint32(this->value_size, &length))
			goto back_false;
		mid = (char*)malloc(sizeof(char) * length);
		strcpy(mid, this->value.c_str());
		send(*client, mid, strlen(mid), 0);
		return true;
	}
back_false:
	return false;
}

void send_get_respond::get_value_key(KVdatabase_data* data)//返回value的长度
{
	this->value = data->value;
	this->value_size[0] = data->valuelen;
	for (int i = 1; i < 4; i++)
	{
		this->value_size[i] = 0;
	}
	return;
}

void send_get_respond::send_fail(SOCKET* client)
{
	send(*client, 0, 1, 0);
}


KVdatabase_data::KVdatabase_data()
{
	keylen = -1;
	valuelen = -1;
}

void KVdatabase_data::setdata_put(recv_data_Put* Put)
{
	this->key_KV = Put->Key.c_str();
	change_uint32(Put->Key_Size, &this->keylen);
	this->value = Put->Value.c_str();
	change_uint32(Put->Value_Size, &this->valuelen);
}

void KVdatabase_data::setdata_delete(recv_data_Delete* Delete)
{
	this->key_KV = Delete->Key.c_str();
	change_uint32(Delete->Key_Size, &this->keylen);
}

void KVdatabase_data::setdata_get(recv_data_Get* Get)
{
	this->key_KV = Get->Key.c_str();
	change_uint32(Get->Key_Size, &this->keylen);
}

KVdatabase::KVdatabase()          //储存格式：key[空格]value
{
	index.clear();
	//判断文件是否创建
	if ((fopen("KVdatabasefile.txt", "rb")) == NULL)
	{
		std::cout << "The database file does not exist!" << std::endl;
		std::cout << "Database file creating..." << std::endl;
		std::ofstream file_create("KVdatabasefile.txt", std::ios::out);
		file_create.close();
		std::cout << "The file created!" << std::endl;
	}
	else
	{
		std::cout << "databasefile is loading..." << std::endl;
		char push_mid;
		std::ifstream file_read;
		if (file_read.eof())
		{
			index.clear();
			return;
		}
		std::string mid;
		int mid_ = 0;
		file_read.open("KVdatabasefile.txt", std::ios::in);
		while (1)
		{
			push_mid = file_read.get();
		string_key:
			if (push_mid != ' ')
				mid.push_back(push_mid);
			else
				goto string_offset;
		}
	string_offset:
		while (1)
		{
			push_mid = file_read.get();
			if (push_mid == '\n')
			{
				mid_++;
				goto data_save_index;
			}
		}
	data_save_index:
		index[mid] = mid_;
		mid.clear();
		push_mid = file_read.get();
		if (push_mid == -1 )    //文件读取完毕
		{
			file_read.close();
			std::cout << "database file loading completed !" << std::endl;
		}
		else
			goto string_key;
	}
}

bool KVdatabase::updateKVdatabasefile()           //已初始化后的更新
{
	if ((fopen("KVdatabasefile.txt", "rb")) == NULL)
	{
		std::cout << "The file does not exist!" << std::endl;
		putMsg_error("File updata");
		return false;
	}
	index.clear();
	char push_mid;
	std::ifstream file_read;
	std::string mid;
	int mid_ = 0;
	file_read.open("KVdatabasefile.txt", std::ios::in);
	while (1)
	{
		push_mid = file_read.get();
	string_key:
		if (push_mid != ' ')
			mid.push_back(push_mid);
		else
			goto string_offset;
	}
string_offset:
	while (1)
	{
		push_mid = file_read.get();
		if (push_mid == '\n')
		{
			mid_++;
			goto data_save_index;
		}
	}
data_save_index:
	index[mid] = mid_;;
	mid.clear();
	push_mid = file_read.get();
	if (push_mid == -1)    //文件读取完毕
		file_read.close();
	else
		goto string_key;
	return true;
}

void KVdatabase::KVputdata(KVdatabase_data* data_input)
{
	std::ofstream KVputdata_write;
	KVputdata_write.open("KVdatabasefile.txt", std::ios::ate | std::ios::in);
	KVputdata_write.write(data_input->key_KV.c_str(), data_input->keylen);
	KVputdata_write.put(' ');
	KVputdata_write.write(data_input->value.c_str(), data_input->valuelen);
	KVputdata_write.put('\n');
	KVputdata_write.close();
	return;
}

bool KVdatabase::KVgetdata(KVdatabase_data* data_get)
{
	char* key_cstr = (char*)malloc(sizeof(char) * data_get->keylen);
	std::ifstream KVgetdata;
	int value_length = 0;
	KVgetdata.open("KVdatabasefile.txt", std::ios::in);
	if (!index[data_get->key_KV])
	{
		std::cout << "The file didn't save the key send from cilent!" << std::endl;
		return false;
	}
	seek_to_line(&KVgetdata, this->index[data_get->key_KV] - 1);
	KVgetdata.read(key_cstr, data_get->keylen);
	KVgetdata.get();					//使得文件指针定位到对应索引value的位置
	while (1)
	{
		char value_mid;
		value_mid = KVgetdata.get();
		if (value_mid != '\n')
		{
			data_get->value.push_back(value_mid);
			value_length++;
		}
		else
			break;
	}
	data_get->valuelen = value_length;
	KVgetdata.close();
	return true;
}

bool KVdatabase::KVdelete(KVdatabase_data* data_delete)
{
	std::ifstream KVdeletedata;
	KVdeletedata.open("KVdatabasefile.txt", std::ios::out);
	if (!index[data_delete->key_KV])
	{
		std::cout << "The file didn't save the key send from cilent!" << std::endl;
		return false;
	} 
	DelLineData("KVdatabasefile.txt", this->index[data_delete->key_KV]);
	this->updateKVdatabasefile();
	return true;
}

void seek_to_line(std::ifstream* in, int line)
{
	//将打开的文件in，定位到line行。
	{
		int i;
		char buf[1024];
		in->seekg(0, std::ios::beg);  //定位到文件开始。
		for (i = 0; i < line; i++)
		{
			in->getline(buf, sizeof(buf));//读取行。
		}
		return;
	}
}


void DelLineData(const char* fileName, int lineNum)
{
	std::ifstream in;
	in.open(fileName);

	std::string strFileData = "";
	int line = 1;
	char lineData[1024] = { 0 };
	while (in.getline(lineData, sizeof(lineData)))
	{
		if (line == lineNum)
		{
			//strFileData+="\n";
		}
		else
		{
			strFileData += CharToStr(lineData);
			strFileData += "\n";
		}
		line++;

	}
	in.close();

	std::ofstream out;
	out.open(fileName);
	out.flush();
	out << strFileData;
	out.close();
}

int c_strtoint(char* c_str)
{
	int temp = 0;
	for (int i = 0; i < strlen(c_str); i++)
	{
		temp += c_str[i];
	}
	return temp;
}


std::string CharToStr(char* contentChar)
{
	std::string tempStr;
	for (int i = 0; contentChar[i] != '\0'; i++)
	{
		tempStr += contentChar[i];
	}
	return tempStr;
}
