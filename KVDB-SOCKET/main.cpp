#include "socket.hpp"
#include <iostream>
#include <string>

int main()
{
	if (!init_Socket())
		std::cout << "server close!" << std::endl;
	SOCKET main_server;
	main_server = creatsocket_server();      //socket初始化完成 端口绑定25565
	std::cout << "The server is online!" << std::endl;
	std::cout << "port : 25565" << std::endl;
	std::cout << "waiting for connect......." << std::endl;
	while (1)
	{
		SOCKET* client = new SOCKET;
		*client = accept(main_server, NULL, NULL);
		count_cli++;
		if (INVALID_SOCKET == *client)
		{
			putMsg_error("accept");
		}
		CreateThread(NULL, 0, &serverthread, client, 0, NULL);
	}

	close_socket();
	std::cout << "server close!" << std::endl;
	system("pause");
	return 0;
}


//文件逻辑——finnal