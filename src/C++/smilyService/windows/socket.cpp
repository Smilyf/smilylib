/*****************************************************************************************************************************
 *	1、加载套接字库，创建套接字（WSAStartup()/socket()）;
 *	2、绑定套接字到一个IP地址和一个端口上（bind()）;
 *	3、将套接字设置为监听模式等待连接请求；
 *	4、请求到来之后，接受连接请求，返回一个新的对应于此次连接的套接字(accept());
 *	5、用返回的套接字和客户端进行通信（send()/recv()）;
 *	6、返回，等待另一个连接请求
 *	7、关闭套接字，关闭加载的套接字库(closesocket()/WSACleanup());
 *****************************************************************************************************************************/
#include <iostream>
#include <WinSock2.h>
#include "smilyIoRing.cpp"
using namespace std;
#pragma comment(lib, "ws2_32.lib")
smilyIoRing iooo;
struct parmes
{
	HANDLE eventHandle;
	IORING_BUFFER_INFO *buffer_info;
	HIORING ioRing;
};
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
	parmes *cc = reinterpret_cast<parmes *>(lpParam);
	int index = 0;
	while (1)
	{
		WaitForSingleObject(cc->eventHandle, INFINITE);
		// Event signaled, execute fun1
		std::cout << "Executing fun1" << std::endl;

		std::cout << cc->buffer_info << std::endl;
		std::cout << "Information: " << std::endl;
		std::cout << "----------------------------------" << std::endl;
		IORING_CQE cqe;

		std::string result;

		// 将 void* 指针转换为 char* 指针
		char *charPtr = reinterpret_cast<char *>(cc->buffer_info->Address);

		std::cout << charPtr << std::endl;
		std::cout << std::endl;
		std::cout << "----------------------------------" << std::endl;
		if (PopIoRingCompletion(cc->ioRing, &cqe) == S_OK)
		{
			std::cout << "PopIoRingCompletion::S_OK" << std::endl;
			std::cout << "Information: " << cqe.Information << std::endl;
			std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
			std::cout << "UserData: " << cqe.UserData << std::endl;
		}

		break;
		ResetEvent(cc->eventHandle);
	}
	return 0;
}

int main()
{
	HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (eventHandle == nullptr)
	{
		DWORD errorCode = GetLastError();
		std::cout << "Failed to create event. Error code: " << errorCode << std::endl;
		return 1;
	}
	parmes ssss;

	// 初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData; // WSADATA结构体变量的地址值

	// int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
	// 成功时会返回0，失败时返回非零的错误代码值
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		cout << "WSAStartup() error!" << endl;
		return 0;
	}

	// 创建套接字
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET)
	{
		cout << "socket error !" << endl;
		return 0;
	}

	// 绑定IP和端口
	sockaddr_in sin; // ipv4的指定方法是使用struct sockaddr_in类型的变量
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8888);			   // 设置端口。htons将主机的unsigned short int转换为网络字节顺序
	sin.sin_addr.S_un.S_addr = INADDR_ANY; // IP地址设置成INADDR_ANY，让系统自动获取本机的IP地址
	// bind函数把一个地址族中的特定地址赋给scket。
	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("bind error !");
	}

	// 开始监听
	if (listen(slisten, 5) == SOCKET_ERROR)
	{
		cout << "listen error !" << endl;
		return -1;
	}

	// 循环接收数据
	SOCKET sclient;
	sockaddr_in remoteAddr; // sockaddr_in常用于socket定义和赋值,sockaddr用于函数参数
	int nAddrlen = sizeof(remoteAddr);
	char revData[255];
	iooo.smilyBuildIoRingRegisterBuffers(500);
	while (true)
	{

		sclient = accept(slisten, (sockaddr *)&remoteAddr, &nAddrlen);
		if (sclient == INVALID_SOCKET)
		{
			cout << "accept error !" << endl;
			continue;
		}
		cout << "**********************************" << inet_ntoa(remoteAddr.sin_addr) << endl;
		// 接收数据
		
		iooo.smilyBuildIoRingReadFile((HANDLE)sclient,eventHandle);

		ssss.eventHandle = eventHandle;
		ssss.buffer_info = iooo.buffer_info.data();
		ssss.ioRing = iooo.ioRing;
		auto threadHandle = CreateThread(nullptr, 0, ThreadFunction, &ssss, 0, nullptr);
		if (threadHandle == nullptr)
		{
			DWORD errorCode = GetLastError();
			std::cout << "Failed to create thread. Error code: " << errorCode << std::endl;
			CloseHandle(eventHandle);
			return 1;
		}

		// Set the event, signaling the thread to execute fun1
		if (SetEvent(eventHandle) == 0)
		{
			DWORD errorCode = GetLastError();
			std::cout << "Failed to set event. Error code: " << errorCode << std::endl;
			CloseHandle(eventHandle);
			CloseHandle(threadHandle);
			return 1;
		}

		iooo.smilySubmitIoRing();
		// iooo.fun2((HANDLE)sclient);
		//  int ret=recv(sclient,revData,255,0);
		//  if(ret>0)
		//  {
		//  	revData[ret]=0x00;
		//  	cout<<revData<<endl;
		//  }
		// 发送数据
		// const char *sendData = "hello**********************";
		// send(sclient, sendData, strlen(sendData), 0);
		closesocket(sclient);
		cout<<"++++++++++++++++++++++++++++"<<endl;
	}
	closesocket(slisten);
	WSACleanup();
	system("pause");
	return 0;
}
