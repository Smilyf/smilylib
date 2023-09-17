
/*****************************************************************************************************************************
 *	1、加载套接字库，创建套接字（WSAStartup()/socket()）;
 *	2、绑定套接字到一个IP地址和一个端口上（bind()）;
 *	3、将套接字设置为监听模式等待连接请求；
 *	4、请求到来之后，接受连接请求，返回一个新的对应于此次连接的套接字(accept());
 *	5、用返回的套接字和客户端进行通信（send()/recv()）;
 *	6、返回，等待另一个连接请求
 *	7、关闭套接字，关闭加载的套接字库(closesocket()/WSACleanup());
 *****************************************************************************************************************************/

#include <thread>
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "smilyService/windows/smilyIoRing.cpp"
smilyIoRing iooo;
int main(int argc, char *argv[])
{
    // 初始化WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        return 0;
    }

    // 创建套接字
    SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slisten == INVALID_SOCKET)
    {
        printf("socket error !");
        return 0;
    }

    // 绑定IP和端口
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        printf("bind error !");
    }

    // 开始监听
    if (listen(slisten, 5) == SOCKET_ERROR)
    {
        printf("listen error !");
        return 0;
    }

    // 循环接收数据

    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    char revData[1024];

    HANDLE hFile = CreateFile(
        "C:\\Users\\yam_l\\Desktop\\text.txt",
        GENERIC_WRITE, // 以读取方式打开文件
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (eventHandle == nullptr)
    {
        DWORD errorCode = GetLastError();
        std::cout << "Failed to create event. Error code: " << errorCode << std::endl;
        return 1;
    }
    iooo.smilyBuildIoRingRegisterBuffers(1024 *1024*1024);

    int xxx = 0;

    while (true)
    {
        //printf("+++++++++++++++++++...\n");
        SOCKET sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
        if (sClient == INVALID_SOCKET)
        {
            printf("accept error !");
            continue;
        }
        // printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));

        iooo.smilyBuildIoRingReadFile((HANDLE)sClient, eventHandle);
        // printf("-----------------...\n");
        //  //接收数据
        //  int ret = recv(sClient, revData, 255, 0);
        //  if (ret > 0)
        //  {
        //      revData[ret] = 0x00;
        //      printf(revData);
        //  }
        
        // //发送数据
        // const char *sendData = "hello world\n";
        // send(sClient, sendData, strlen(sendData), 0);
        // closesocket(sClient);
    }
    closesocket(slisten);
    WSACleanup();
    return 0;
}
