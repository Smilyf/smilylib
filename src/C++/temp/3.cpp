#include <windows.h>
#include <ioringapi.h>
#include <iostream>
HANDLE eventHandle;
IORING_BUFFER_INFO buffer_info[10];
HIORING ioRing;
struct parmes
{
    HANDLE eventHandle;
    IORING_BUFFER_INFO buffer_info[10];
    HIORING ioRing;

} ssss;
struct buffer
{
    int buffer_array[256];
};
bool fun1()
{
    IORING_CQE cqe;

    std::string result;

    // 将 void* 指针转换为 char* 指针
    char *charPtr = reinterpret_cast<char *>(buffer_info[0].Address);

    std::cout << "----" << buffer_info[0].Address << std::endl;
    // 使用 char* 指针读取数据，直到遇到空字符 '\0' 为止
    int index = 0;
    while (index < 30)
    {
        result += *charPtr;
        charPtr++;
        index++;
    }
    result += 'c';
    std::cout << "Information: " << result << std::endl;
    std::cout << "----------------------------------" << std::endl;
    if (PopIoRingCompletion(ioRing, &cqe) == S_OK)
    {
        std::cout << "PopIoRingCompletion::S_OK" << std::endl;
        std::cout << "Information: " << cqe.Information << std::endl;
        std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
        std::cout << "UserData: " << cqe.UserData << std::endl;
    }
    return false;
}
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{

    int index = 0;
    while (1)
    {
        WaitForSingleObject(eventHandle, INFINITE);
        // Event signaled, execute fun1
        std::cout << "Executing fun1" << std::endl;
        std::cout << "----" << buffer_info[0].Address << std::endl;
        fun1();
        if (index > 3)
        {
            break;
        }
        index++;
        ResetEvent(eventHandle);
    }

    return 0;
}

int main()
{

    IORING_CAPABILITIES t;
    QueryIoRingCapabilities(&t);
    std::cout << "MaxVersion: " << t.MaxVersion << std::endl;
    std::cout << "FeatureFlags: " << t.FeatureFlags << std::endl;
    std::cout << "MaxCompletionQueueSize: " << t.MaxCompletionQueueSize << std::endl;
    std::cout << "MaxSubmissionQueueSize: " << t.MaxSubmissionQueueSize << std::endl;
    IORING_CREATE_FLAGS Flags;
    Flags.Required = IORING_CREATE_REQUIRED_FLAGS_NONE;
    Flags.Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE;
    if (CreateIoRing(t.MaxVersion, Flags, t.MaxSubmissionQueueSize, t.MaxCompletionQueueSize, &ioRing) == S_OK)
    {
        std::cout << "CreateIoRing::S_OK" << std::endl;
    }
    else
    {
        std::cout << "CreateIoRing:S_FALSE:" << std::endl;
        return 0;
    }

    UINT_PTR userData = 99;
    UINT32 count = 10;

    for (int i = 0; i < 10; i++)
    {
        buffer_info[i].Address = calloc(1, sizeof(struct buffer));
        // buffer_info[i].Address = new buffer;
        buffer_info[i].Length = 1024;
    }
    std::cout << "----" << buffer_info[0].Address << std::endl;

    HANDLE hFile = CreateFile(
        "C:\\Users\\yam_l\\Desktop\\text.txt",
        GENERIC_READ, // 以读取方式打开文件
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    DWORD dwFileSize;
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // 打开文件失败
        // 进行异常处理
        std::cout << "无法打开文件" << std::endl;
        return 1;
    }
    else
    {
        // 获取文件大小
        dwFileSize = GetFileSize(hFile, NULL);
        std::cout << "FileSize: " << dwFileSize << std::endl;
    }

    UINT_PTR userDataRead = 20;
    UINT_PTR BuildIoRin = 30;
    HANDLE handles[10];
    handles[0] = hFile;

    if (BuildIoRingRegisterBuffers(ioRing, count, buffer_info, userData) == S_OK)
    {
        std::cout << "BuildIoRingRegisterBuffers::S_OK" << std::endl;
    }
    else
    {
        std::cout << "BuildIoRingRegisterBuffers:S_FALSE:" << std::endl;
        return 0;
    }
    std::cout << "----" << buffer_info[0].Address << std::endl;
    // if (BuildIoRingRegisterFileHandles(ioRing, 1, handles, BuildIoRin) == S_OK)
    // {
    //     std::cout << "BuildIoRingRegisterFileHandles::S_OK" << std::endl;
    // }
    // else
    // {
    //     std::cout << "BuildIoRingRegisterFileHandles:S_FALSE:" << std::endl;
    //     return 0;
    // }

    // 从文件打开
    if (BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(hFile), IoRingBufferRefFromPointer(buffer_info[0].Address), dwFileSize, 0, userDataRead, IOSQE_FLAGS_DRAIN_PRECEDING_OPS) == S_OK)
    {
        std::cout << "RingReadFile::S_OK" << std::endl;
    }
    else
    {
        std::cout << "RingReadFile:S_FALSE:" << std::endl;
        return 0;
    }
    std::cout << "----" << buffer_info[0].Address << std::endl;
    auto hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        std::cout << "Failed to create event." << std::endl;
        return 0;
    }
    if (SetIoRingCompletionEvent(ioRing, hEvent) == S_OK)
    {
        std::cout << "SetIoRingCompletion::S_OK" << std::endl;
    }
    else
    {
        std::cout << "SetIoRingCompletion:S_FALSE:" << std::endl;
        return 0;
    }
    UINT32 x;

    eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (eventHandle == nullptr)
    {
        DWORD errorCode = GetLastError();
        std::cout << "Failed to create event. Error code: " << errorCode << std::endl;
        return 1;
    }

    // Create a thread that waits for the event and invokes fun1

    HANDLE threadHandle = CreateThread(nullptr, 0, ThreadFunction, &ssss, 0, nullptr);
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
    SetIoRingCompletionEvent(ioRing, eventHandle);
    if (SubmitIoRing(ioRing, 0, 1, &x) == S_OK)
    {
        std::cout << "Submit::S_OK" << x << std::endl;
    }
    else
    {
        std::cout << "Submit:S_FALSE:" << std::endl;
        return 0;
    }
    WaitForSingleObject(threadHandle, INFINITE);
    CloseIoRing(ioRing);
    return 0;
}