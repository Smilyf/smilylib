#include <windows.h>
#include <ioringapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <deque>

struct informationMap
{
public:
    class readFileInformation
    {
    public:
        HANDLE handle;
        HANDLE completionEvent;
        UINT32 size;
        UINT64 fileOffset;
        HANDLE eventHandle;
        void *Address;
        readFileInformation() = default;
        readFileInformation(HANDLE handle_,
                            HANDLE completionEvent_,
                            UINT32 size_,
                            UINT64 fileOffset_,
                            HANDLE eventHandle_,
                            void *Address_)
        {
            handle = handle_;
            completionEvent = completionEvent_;
            size = size_;
            fileOffset = fileOffset_;
            eventHandle = eventHandle_;
            Address = Address_;
        }
        readFileInformation(const readFileInformation &t) = default;
    };
    std::mutex Mutex;
    std::map<HANDLE, readFileInformation> handleMap;
    informationMap() = default;
    void push(HANDLE handle, readFileInformation value)
    {
        std::lock_guard<std::mutex> guard(Mutex);
        if (handleMap.find(handle) != handleMap.end())
        {

            handleMap.at(handle) = value;
            // std::cout << "+++++++++++++++++" << value.fileOffset << std::endl;
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
        return handleMap.at(handle);
        // try
        // {
        //     return handleMap.at(handle);
        // }
        // catch (const std::out_of_range &e)
        // {
        //     std::cout << "Key not found!" << std::endl;
        // }
    }
    void erase(HANDLE userData)
    {
        std::lock_guard<std::mutex> guard(Mutex);
        handleMap.erase(userData);
    }
};

struct smilyIoRing
{
public:
    IORING_CAPABILITIES t;

    HIORING ioRing;
    UINT32 allocSize = 128;
    informationMap informations;
    UINT_PTR userDataDequesize;
    HANDLE threadHandle;
    bool messageFlag = true;

    std::deque<void *> registerBuffersDeque{};
    std::deque<UINT_PTR> userDataDeque{};
    std::deque<HANDLE> eventHandleDeque{};
    std::vector<IORING_BUFFER_INFO> bufferInfo;

    std::mutex userDataMutex;
    std::mutex registerBufferMutex;
    std::mutex eventHandleMutex;
    std::mutex bufferInfoMutex;
    std::mutex informationMapoMutex;

    enum userDataEnum : UINT_PTR
    {
        userDataRegisterBuffers,
        sscanf
    };

    smilyIoRing()
    {
        QueryIoRingCapabilities(&t);
        // std::cout << "MaxVersion: " << t.MaxVersion << std::endl;
        // std::cout << "FeatureFlags: " << t.FeatureFlags << std::endl;
        // std::cout << "MaxCompletionQueueSize: " << t.MaxCompletionQueueSize << std::endl;
        // std::cout << "MaxSubmissionQueueSize: " << t.MaxSubmissionQueueSize << std::endl;
        IORING_CREATE_FLAGS Flags;
        Flags.Required = IORING_CREATE_REQUIRED_FLAGS_NONE;
        Flags.Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE;
        if (CreateIoRing(t.MaxVersion, Flags, t.MaxSubmissionQueueSize, t.MaxCompletionQueueSize, &ioRing) == S_OK)
        {
            // std::cout << "CreateIoRing::S_OK" << std::endl;
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
    // void Print(std::string message, bool flag = true)
    // {
    //     if (flag & messageFlag)
    //     {
    //         std::cout << message << std::endl;
    //     }
    // }
    static DWORD WINAPI ThreadFunction(LPVOID lpParam)
    {
        auto *smilyIoRingThread = reinterpret_cast<smilyIoRing *>(lpParam);
        while (true)
        {
            if (!smilyIoRingThread->emptyEventHandleDeque())
            {
                auto eventHandle = smilyIoRingThread->getEventHandleDeque(true);
                WaitForSingleObject(eventHandle, INFINITE);
                auto information = smilyIoRingThread->getInformationMap(eventHandle);
                auto Address = information.Address;
                char *charPtr = reinterpret_cast<char *>(Address);
                smilyIoRingThread->setregisterBuffers(Address);
                std::cout << "strlen():   " << strlen(charPtr) << "    " << charPtr << std::endl;
                auto cqe = smilyIoRingThread->smilyPopIoRingCompletion();
                if (cqe.UserData == smilyIoRingThread->userDataEnum::userDataRegisterBuffers)
                {
                    cqe = smilyIoRingThread->smilyPopIoRingCompletion();
                }
                if (cqe.ResultCode == 0)
                {
                    information.fileOffset += information.size;
                    smilyIoRingThread->setInformationMap(information.handle, information);
                    smilyIoRingThread->smilyBuildIoRingReadFile(information.handle, information.eventHandle, information.fileOffset);
                }
                else
                {
                    CloseHandle(information.handle);
                    smilyIoRingThread->eraseInformationMap(information.handle);
                }
                
            }
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
    void setInformationMap(HANDLE handle, informationMap::readFileInformation value)
    {
        std::lock_guard<std::mutex> guard(informationMapoMutex);
        informations.push(handle, value);
    }
    void eraseInformationMap(HANDLE handle)
    {
        std::lock_guard<std::mutex> guard(informationMapoMutex);
        informations.erase(handle);
    }

    informationMap::readFileInformation getInformationMap(HANDLE handle)
    {
        return informations.get(handle);
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
    bool emptyEventHandleDeque(bool flag = false)
    {
        std::lock_guard<std::mutex> guard(eventHandleMutex);
        return eventHandleDeque.empty();
    }

    void setEventHandleDeque(HANDLE eventHandle)
    {
        std::lock_guard<std::mutex> guard(eventHandleMutex);
        eventHandleDeque.push_back(eventHandle);
    }
    void *getregisterBuffers()
    {
        std::lock_guard<std::mutex> guard(registerBufferMutex);
        void *index = registerBuffersDeque.front();
        registerBuffersDeque.pop_front();
        return index;
    }
    void setregisterBuffers(void *index)
    {
        std::lock_guard<std::mutex> guard(registerBufferMutex);
        registerBuffersDeque.push_back(index);
    }

    UINT_PTR getUserDataDeque()
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
    void getUserDataDeque(UINT_PTR userData)
    {
        std::lock_guard<std::mutex> guard(userDataMutex);
        userDataDeque.push_back(userData);
    }

    void smilyBuildIoRingRegisterBuffers(UINT32 size)
    {
        UINT32 count = (size % allocSize == 0) ? (size / allocSize) : (size / allocSize + 1);
        for (UINT32 i = 0; i < count; i++)
        {
            bufferInfo.emplace_back(VirtualAlloc(NULL, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), allocSize);
        }
        if (BuildIoRingRegisterBuffers(ioRing, count, bufferInfo.data(), userDataRegisterBuffers) == S_OK)
        {
            // std::cout << "BuildIoRingRegisterBuffers::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("BuildIoRingRegisterBuffers:S_FALSE:");
        }

        for (UINT32 i = 0; i < count; i++)
        {
            registerBuffersDeque.push_back(bufferInfo.data()[i].Address);
        }
    }
    void smilySetIoRingCompletionEvent(HANDLE hEvent)
    {
        if (SetIoRingCompletionEvent(ioRing, hEvent) == S_OK)
        {
            // std::cout << "SetIoRingCompletionEvent::S_OK" << std::endl;
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
            // std::cout << "SubmitIoRing::S_OK" << std::endl;
        }
        else
        {
            throw std::runtime_error("SubmitIoRing:S_FALSE:");
        }
    }
    void smilyBuildIoRingReadFile(HANDLE handle, HANDLE completionEvent, UINT64 fileOffset = 0, UINT32 size = 100)
    {
        size = allocSize;
        auto Address = getregisterBuffers();
        auto userData = getUserDataDeque();
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            throw std::runtime_error("CreateEvent:S_FALSE:");
        }
        setEventHandleDeque(eventHandle);
        smilySetIoRingCompletionEvent(eventHandle);
        setInformationMap(eventHandle, informationMap::readFileInformation{handle, completionEvent, size, fileOffset, eventHandle, Address});
        if (BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(Address), size, fileOffset, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS) == S_OK)
        {
            // std::cout << "fileOffset::" << fileOffset << std::endl;
            // std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
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
            // std::cout << "PopIoRingCompletion::S_OK" << std::endl;
            // std::cout << "Information: " << cqe.Information << std::endl;
            // std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
            // std::cout << "UserData: " << cqe.UserData << std::endl;
            getUserDataDeque(cqe.UserData);
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

    //     std::cout << "----" << bufferInfo[0].Address << std::endl;

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

    // std::cout << "----" << bufferInfo[0].Address << std::endl;
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

    // std::cout << "----" << bufferInfo[0].Address << std::endl;
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