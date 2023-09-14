#include <windows.h>
#include <ioringapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
class readFileInformation
{
public:
    HANDLE handle;
    HANDLE completionEvent;
    UINT32 size;
    UINT64 fileOffset;
    HANDLE eventHandle;
    UINT32 index;
    readFileInformation() = default;
    readFileInformation(HANDLE handle_,
                        HANDLE completionEvent_,
                        UINT32 size_,
                        UINT64 fileOffset_,
                        HANDLE eventHandle_,
                        UINT32 index_)
    {
        handle = handle_;
        completionEvent = completionEvent_;
        size = size_;
        fileOffset = fileOffset_;
        eventHandle = eventHandle_;
        index = index_;
    }
    readFileInformation(const readFileInformation &t) = default;
};
struct informationMap
{
public:
    std::mutex Mutex;
    std::map<HANDLE, readFileInformation> handleMap;
    informationMap() = default;
    void push(HANDLE handle, readFileInformation value)
    {
        std::lock_guard<std::mutex> guard(Mutex);
        if (handleMap.find(handle) != handleMap.end())
        {

            handleMap.at(handle) = readFileInformation{};
        }
        else
        {
            handleMap.emplace(handle, value);
        }
        // std::cout << "dasasdasd" << handle << std::endl;
        // std::cout << "dasasdasd" << handleMap[handle].index << std::endl;
    }
    readFileInformation get(HANDLE handle)
    {
        std::lock_guard<std::mutex> guard(Mutex);

        try
        {
            // std::cout << "dasasdasd" << handle << std::endl;
            //  std::cout << "dasasdasd" << handleMap[handle].index << std::endl;
            return handleMap.at(handle);
        }
        catch (const std::out_of_range &e)
        {
            std::cout << "Key not found!" << std::endl;
        }
        // return handleMap[userData];
    }
    void erase(HANDLE userData)
    {
        std::lock_guard<std::mutex> guard(Mutex);
        handleMap.erase(userData);
    }
};

struct handlesMaP
{
public:
    std::map<HANDLE, std::vector<int>> handleMap;
    handlesMaP() = default;
    void push(HANDLE handle, int value)
    {
        if (handleMap.find(handle) != handleMap.end())
        {
            handleMap.at(handle).push_back(value);
        }
        else
        {
            std::vector<int> values;
            values.push_back(value);
            handleMap.emplace(handle, values);
        }
    }
    std::vector<int> get(HANDLE handle)
    {
        return handleMap.at(handle);
    }
    void erase(HANDLE handle)
    {
        handleMap.erase(handle);
    }
};
// struct Parameters
// {
//     std::deque<UINT_PTR> userDataDeque;
//     std::deque<HANDLE> eventHandleDeque;
//     HIORING ioRing;
// };

struct smilyIoRing
{
public:
    IORING_CAPABILITIES t;
    std::vector<IORING_BUFFER_INFO> buffer_info;
    HIORING ioRing;
    UINT32 allocSize = 4096;
    handlesMaP handlesMaPs;

    HANDLE threadHandle;

    std::deque<UINT32> registerBuffersDeque{};
    std::deque<UINT_PTR> userDataDeque{};
    std::deque<HANDLE> eventHandleDeque{};

    informationMap informations;
    UINT_PTR userDataDequesize;
    std::mutex userDataMutex;
    std::mutex registerBufferMutex;
    std::mutex eventHandleMutex;

    enum userDataEnum : UINT_PTR
    {
        userDataRegisterBuffers,
        sscanf
    };

    smilyIoRing()
    {
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
            throw std::runtime_error("CreateIoRing:S_FALSE:");
        }
        userDataDequesize = t.MaxSubmissionQueueSize / 2;
        for (UINT_PTR i = 10; i < userDataDequesize; i++)
        {
            userDataDeque.push_back(i);
        }
        // Parameters param{userDataDeque, eventHandleDeque, ioRing};
        createThread();
    }
    ~smilyIoRing()
    {
        CloseHandle(threadHandle);
        CloseIoRing(ioRing);
    }

    static DWORD WINAPI ThreadFunction(LPVOID lpParam)
    {
        auto *ccsdasdasdasdasdas = reinterpret_cast<smilyIoRing *>(lpParam);
        std::deque<HANDLE> &eventHandleDeque = ccsdasdasdasdasdas->eventHandleDeque;
        informationMap &informations = ccsdasdasdasdasdas->informations;
        // std::cout<<"informations  "<<&informations.handleMap<< std::endl;
        int iundesdas = 0;
        while (1)
        {
            if (!eventHandleDeque.empty())
            {
                iundesdas++;

                auto t = ccsdasdasdasdasdas->getEventHandleDeque(true);
                std::cout << "---------------- " << t << std::endl;
                WaitForSingleObject(t, INFINITE);
                
                auto xxxx = informations.get(t);
                auto yy = xxxx.index;
                // std::cout << "---------------- " << t << std::endl;
                char *charPtr = reinterpret_cast<char *>(ccsdasdasdasdasdas->buffer_info[yy].Address);
                std::cout << "WaitForSingleObject " << iundesdas << std::endl;
                std::cout << "strlen():   " << strlen(charPtr) << "    " << charPtr << std::endl;
                if (strlen(charPtr) == 100)
                {
                    ccsdasdasdasdasdas->smilyPopIoRingCompletion();
                    ccsdasdasdasdasdas->smilyBuildIoRingReadFile(xxxx.handle, xxxx.eventHandle, xxxx.fileOffset + 100);
                    xxxx.fileOffset += 100;
                }
                else
                {
                    ccsdasdasdasdasdas->smilyPopIoRingCompletion();
                }
            }
            //
            // if (!eventHandleDeque.empty())
            // {
            //     auto t = eventHandleDeque.front();
            //     eventHandleDeque.pop_front();
            //
            //     auto yy = informations.get(t);

            //

            //     std::cout << charPtr << std::endl;
            //     ccsdasdasdasdasdas->smilyPopIoRingCompletion();
            //     // ResetEvent(cc->eventHandle);
            // }
            // else
            // {
            //     continue;
            // }

            // // Event signaled, execute fun1
            // std::cout << "Executing fun1" << std::endl;

            // std::cout << "Information: " << std::endl;
            // std::cout << "----------------------------------" << std::endl;
            // IORING_CQE cqe;

            // std::string result;

            // // 将 void* 指针转换为 char* 指针
            // char *charPtr = reinterpret_cast<char *>(cc->buffer_info->Address);

            // std::cout << charPtr << std::endl;
            // std::cout << std::endl;
            // std::cout << "----------------------------------" << std::endl;
            // if (PopIoRingCompletion(cc->ioRing, &cqe) == S_OK)
            // {
            //     std::cout << "PopIoRingCompletion::S_OK" << std::endl;
            //     std::cout << "Information: " << cqe.Information << std::endl;
            //     std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
            //     std::cout << "UserData: " << cqe.UserData << std::endl;
            // }
        }
        return 0;
    }
    void createThread()
    {
        threadHandle = CreateThread(nullptr, 0, &smilyIoRing::ThreadFunction, this, 0, nullptr);
        if (threadHandle == nullptr)
        {
            throw std::runtime_error("CreateThread:S_FALSE:");
        }
    }

    HANDLE getEventHandleDeque(bool flag = false)
    {
        std::lock_guard<std::mutex> guard(eventHandleMutex);
        HANDLE eventHandle = eventHandleDeque.front();
        if (flag)
        {
            eventHandleDeque.pop_front();
        }
        return eventHandle;
    }
    void setEventHandleDeque(HANDLE eventHandle)
    {
        std::lock_guard<std::mutex> guard(eventHandleMutex);
        eventHandleDeque.push_back(eventHandle);
    }
    UINT32 getregisterBuffers()
    {
        std::lock_guard<std::mutex> guard(registerBufferMutex);
        UINT32 index = registerBuffersDeque.front();
        registerBuffersDeque.pop_front();
        return index;
    }
    void setregisterBuffers(UINT32 index)
    {
        std::lock_guard<std::mutex> guard(registerBufferMutex);
        registerBuffersDeque.push_back(index);
    }

    UINT_PTR getUserData()
    {
        std::lock_guard<std::mutex> guard(userDataMutex);
        if (userDataDeque.empty())
        {
            for (UINT_PTR i = userDataDequesize; i < userDataDequesize * 2; i++)
            {
                userDataDeque.push_back(i);
            }
            userDataDequesize *= 2;
        }
        UINT_PTR userData = userDataDeque.front();
        userDataDeque.pop_front();
        return userData;
    }
    void setUserData(UINT_PTR userData)
    {
        std::lock_guard<std::mutex> guard(userDataMutex);
        if (userDataDeque.empty())
        {
            for (UINT_PTR i = userDataDequesize; i < userDataDequesize * 2; i++)
            {
                userDataDeque.push_back(i);
            }
            userDataDequesize *= 2;
        }
        userDataDeque.push_back(userData);
    }

    void smilyBuildIoRingRegisterBuffers(UINT32 size)
    {
        UINT32 count = (size % allocSize == 0) ? (size / allocSize) : (size / allocSize + 1);
        for (UINT32 i = 0; i < count; i++)
        {
            buffer_info.emplace_back(VirtualAlloc(NULL, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), allocSize);
        }
        if (BuildIoRingRegisterBuffers(ioRing, count, buffer_info.data(), userDataRegisterBuffers) == S_OK)
        {
            std::cout << "BuildIoRingRegisterBuffers::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("BuildIoRingRegisterBuffers:S_FALSE:");
        }
        userDataDequesize = buffer_info.size();
        for (UINT32 i = 0; i < userDataDequesize; i++)
        {
            registerBuffersDeque.push_back(i);
        }
    }
    void smilySetIoRingCompletionEvent(HANDLE hEvent)
    {
        if (SetIoRingCompletionEvent(ioRing, hEvent) == S_OK)
        {
            std::cout << "SetIoRingCompletionEvent::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("SetIoRingCompletionEvent:S_FALSE:");
        }
    }
    void smilySubmitIoRing()
    {
        UINT32 submittedEntries;
        if (SubmitIoRing(ioRing, 0, 1, &submittedEntries) == S_OK)
        {
            std::cout << "SubmitIoRing::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("SubmitIoRing:S_FALSE:");
        }
    }
    void smilyBuildIoRingReadFile(HANDLE handle, HANDLE completionEvent, UINT64 fileOffset = 0, UINT32 size = 100)
    {
        auto registerBufferIndex = getregisterBuffers();

        auto userData = getUserData();
        std::cout<< "-------dsadasdasdas--------- " <<userData<<std::endl;
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            throw std::runtime_error("CreateEvent:S_FALSE:");
        }

        setEventHandleDeque(eventHandle);
        smilySetIoRingCompletionEvent(eventHandle);
        std::cout << "---------------- " << eventHandle << std::endl;
        informations.push(eventHandle, readFileInformation{handle, completionEvent, size, 0, eventHandle, registerBufferIndex});

        if (BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(buffer_info[registerBufferIndex].Address), size, fileOffset, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS) == S_OK)
        {
            std::cout << "fileOffset::" << fileOffset << std::endl;
            std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("BuildIoRingReadFile:S_FALSE:");
        }
        smilySubmitIoRing();
    }
    IORING_CQE smilyPopIoRingCompletion()
    {
        IORING_CQE cqe;
        if (PopIoRingCompletion(ioRing, &cqe) == S_OK)
        {
            std::cout << "PopIoRingCompletion::S_OK" << std::endl;
            std::cout << "Information: " << cqe.Information << std::endl;
            std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
            std::cout << "UserData: " << cqe.UserData << std::endl;
        }
        else
        {
            throw std::runtime_error("PopIoRingCompletion:S_FALSE:");
        }
        return cqe;
    }

    // HRESULT createFile(HANDLE sclient)
    // {

    //     UINT_PTR userData = 99;
    //     SIZE_T count = 10;

    //     std::cout << "----" << buffer_info[0].Address << std::endl;

    //     HANDLE hFile = CreateFile(
    //         "C:\\Users\\yam_l\\Desktop\\text.txt",
    //         GENERIC_READ, // 以读取方式打开文件
    //         0,
    //         NULL,
    //         OPEN_EXISTING,
    //         FILE_ATTRIBUTE_NORMAL,
    //         NULL);
    //     DWORD dwFileSize;
    //     if (hFile == INVALID_HANDLE_VALUE)
    //     {
    //         // 打开文件失败
    //         // 进行异常处理
    //         std::cout << "无法打开文件" << std::endl;
    //         return 1;
    //     }
    //     else
    //     {
    //         // 获取文件大小
    //         dwFileSize = GetFileSize(hFile, NULL);
    //         std::cout << "FileSize: " << dwFileSize << std::endl;
    //     }

    // std::cout << "----" << buffer_info[0].Address << std::endl;
    //  if (BuildIoRingRegisterFileHandles(ioRing, 1, handles, BuildIoRin) == S_OK)
    //  {
    //      std::cout << "BuildIoRingRegisterFileHandles::S_OK" << std::endl;
    //  }
    //  else
    //  {
    //      std::cout << "BuildIoRingRegisterFileHandles:S_FALSE:" << std::endl;
    //      return 0;
    //  }

    // 从文件打开

    // std::cout << "----" << buffer_info[0].Address << std::endl;
    // auto hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // if (hEvent == NULL)
    // {
    //     std::cout << "Failed to create event." << std::endl;
    //     return 0;
    // }

    // eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    // if (eventHandle == nullptr)
    // {
    //     DWORD errorCode = GetLastError();
    //     std::cout << "Failed to create event. Error code: " << errorCode << std::endl;
    //     return 1;
    // }

    // Create a thread that waits for the event and invokes fun1

    // WaitForSingleObject(threadHandle, INFINITE);
};