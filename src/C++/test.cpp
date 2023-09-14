
/*****************************************************************************************************************************
 *	1、加载套接字库，创建套接字（WSAStartup()/socket()）;
 *	2、绑定套接字到一个IP地址和一个端口上（bind()）;
 *	3、将套接字设置为监听模式等待连接请求；
 *	4、请求到来之后，接受连接请求，返回一个新的对应于此次连接的套接字(accept());
 *	5、用返回的套接字和客户端进行通信（send()/recv()）;
 *	6、返回，等待另一个连接请求
 *	7、关闭套接字，关闭加载的套接字库(closesocket()/WSACleanup());
 *****************************************************************************************************************************/
// #include<iostream>
#include "smilyService/windows/smilyIoRing.cpp"
// #include <WinSock2.h>
// #pragma comment(lib, "ws2_32.lib")
#include <thread>
smilyIoRing iooo;
#include <thread>
int main()
{
    HANDLE hFile = CreateFile(
        "C:\\Users\\yam_l\\Desktop\\text.txt",
        GENERIC_READ, // 以读取方式打开文件
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    //         if (hFile == INVALID_HANDLE_VALUE)
    // {
    //     // 打开文件失败
    //     // 进行异常处理
    //     std::cout << "无法打开文件" << std::endl;
    //     return 1;
    // }
    // else
    // {
    //     // 获取文件大小
    //     auto dwFileSize = GetFileSize(hFile, NULL);
    //     std::cout << "FileSize: " << dwFileSize << std::endl;
    // }
    HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (eventHandle == nullptr)
    {
        DWORD errorCode = GetLastError();
        std::cout << "Failed to create event. Error code: " << errorCode << std::endl;
        return 1;
    }
    iooo.smilyBuildIoRingRegisterBuffers(1024 * 20);
    iooo.smilyBuildIoRingReadFile(hFile, eventHandle);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}

// 	// 初始化WSA
// 	WORD sockVersion = MAKEWORD(2, 2);
// 	WSADATA wsaData; // WSADATA结构体变量的地址值

// 	// int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
// 	// 成功时会返回0，失败时返回非零的错误代码值
// 	if (WSAStartup(sockVersion, &wsaData) != 0)
// 	{
// 		std::cout << "WSAStartup() error!" << std::endl;
// 		return 0;
// 	}

// 	// 创建套接字
// 	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
// 	if (slisten == INVALID_SOCKET)
// 	{
// 		std::cout << "socket error !" << std::endl;
// 		return 0;
// 	}

// 	// 绑定IP和端口
// 	sockaddr_in sin; // ipv4的指定方法是使用struct sockaddr_in类型的变量
// 	sin.sin_family = AF_INET;
// 	sin.sin_port = htons(8888);			   // 设置端口。htons将主机的unsigned short int转换为网络字节顺序
// 	sin.sin_addr.S_un.S_addr = INADDR_ANY; // IP地址设置成INADDR_ANY，让系统自动获取本机的IP地址
// 	// bind函数把一个地址族中的特定地址赋给scket。
// 	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
// 	{
// 		printf("bind error !");
// 	}

// 	// 开始监听
// 	if (listen(slisten, 5) == SOCKET_ERROR)
// 	{
// 		std::cout << "listen error !" << std::endl;
// 		return -1;
// 	}

// 	// 循环接收数据
// 	SOCKET sclient;
// 	sockaddr_in remoteAddr; // sockaddr_in常用于socket定义和赋值,sockaddr用于函数参数
// 	int nAddrlen = sizeof(remoteAddr);
// 	char revData[255];
// 	//iooo.smilyBuildIoRingRegisterBuffers(500);
// 	while (true)
// 	{

// 		sclient = accept(slisten, (sockaddr *)&remoteAddr, &nAddrlen);
// 		if (sclient == INVALID_SOCKET)
// 		{
// 			std::cout << "accept error !" << std::endl;
// 			continue;
// 		}
// 		std::cout << "**********************************" << inet_ntoa(remoteAddr.sin_addr) << std::endl;
// 		// 接收数据

// 		//iooo.smilyBuildIoRingReadFile((HANDLE)sclient,eventHandle);

// 		// iooo.fun2((HANDLE)sclient);
// 		//  int ret=recv(sclient,revData,255,0);
// 		//  if(ret>0)
// 		//  {
// 		//  	revData[ret]=0x00;
// 		//  	std::cout<<revData<<std::endl;
// 		//  }
// 		// 发送数据
// 		// const char *sendData = "hello**********************";
// 		// send(sclient, sendData, strlen(sendData), 0);
// 		closesocket(sclient);
// 		std::cout<<"++++++++++++++++++++++++++++"<<std::endl;
// 	}
// 	closesocket(slisten);
// 	WSACleanup();
// 	system("pause");
// 	return 0;
// }
