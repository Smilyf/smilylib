#include <windows.h>
#include <ioringapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <atomic>

struct smilyIoRing
{
public:
    enum userDataEnum : UINT_PTR
    {
        userDataRegisterBuffers,
        sscanf
    };
    enum ioRingType
    {
        readFile,
        writeFile
    };
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
            smilyIoRing::ioRingType type;
            readFileInformation() = default;
            readFileInformation(HANDLE handle_,
                                HANDLE completionEvent_,
                                UINT32 size_,
                                UINT64 fileOffset_,
                                HANDLE eventHandle_,
                                void *Address_, smilyIoRing::ioRingType type_)
            {
                handle = handle_;
                completionEvent = completionEvent_;
                size = size_;
                fileOffset = fileOffset_;
                eventHandle = eventHandle_;
                Address = Address_;
                type = type_;
            }
            readFileInformation(const readFileInformation &t) = default;
        };
        std::mutex Mutex;
        std::map<UINT_PTR, readFileInformation> handleMap;
        informationMap() = default;
        void push(UINT_PTR handle, readFileInformation value)
        {
            std::lock_guard<std::mutex> guard(Mutex);
            if (handleMap.find(handle) != handleMap.end())
            {
                handleMap.at(handle) = value;
            }
            else
            {
                handleMap.emplace(handle, value);
            }
        }
        readFileInformation get(UINT_PTR handle)
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
        void erase(UINT_PTR userData)
        {
            std::lock_guard<std::mutex> guard(Mutex);
            handleMap.erase(userData);
        }
    };

    IORING_CAPABILITIES t;

    HIORING ioRing;
    UINT32 allocSize = 512;
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

    std::atomic<bool> threadAtomic{true};

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
        HRESULT flag = CreateIoRing(t.MaxVersion, Flags, t.MaxSubmissionQueueSize, t.MaxCompletionQueueSize, &ioRing);
        if (flag == S_OK)
        {
            // std::cout << "CreateIoRing::S_OK" << std::endl;
        }
        else
        {
            std::cout << "CreateIoRing:S_FALSE:" << std::endl;
            // throw std::runtime_error("CreateIoRing:S_FALSE:");
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
        // for (auto pMemory : bufferInfo)
        // {
        //     VirtualFree(pMemory.Address, 0, MEM_RELEASE);
        // }
        threadAtomic.store(false);
        WaitForSingleObject(threadHandle, INFINITE);
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
        IORING_CQE cqe;
        while (true)
        {
            if (!smilyIoRingThread->threadAtomic)
            {
                break;
            }
            bool PopIoRingCompletion = smilyIoRingThread->smilyPopIoRingCompletion(cqe);
            if (!PopIoRingCompletion)
            {
                continue;
            }
            if (cqe.UserData == smilyIoRingThread->userDataEnum::userDataRegisterBuffers)
            {
                continue;
            }

            if (cqe.ResultCode == 0)
            {
                auto information = smilyIoRingThread->getInformationMap(cqe.UserData);

                if (smilyIoRing::ioRingType::readFile == information.type)
                { // std::cout << "strlen():   " << information.size << "    " << std::endl;
                    auto Address = information.Address;
                    char *charPtr = reinterpret_cast<char *>(Address);
                    smilyIoRingThread->setregisterBuffers(Address);
                    for (size_t i = 0; i < cqe.Information; i++)
                    {
                        std::cout << charPtr[i];
                    }
                    std::cout << "====================================================" << std::endl;
                    information.fileOffset += information.size;
                    smilyIoRingThread->setInformationMap(cqe.UserData, information);
                    smilyIoRingThread->smilyBuildIoRingReadFile(information.handle, information.eventHandle, information.fileOffset, true, cqe.UserData);
                }
                if (smilyIoRing::ioRingType::writeFile == information.type)
                {
                    smilyIoRingThread->smilyBuildIoRingWriteFile(information.handle, information.eventHandle, information.fileOffset, true, cqe.UserData);
                }
            }
            else
            {

                // CloseHandle(information.handle);
                // const char *sendData = "helloword";
                // send((SOCKET)information.handle, sendData, strlen(sendData), 0);
                // closesocket((SOCKET)information.handle);
                smilyIoRingThread->setUserDataDeque(cqe.UserData);
                smilyIoRingThread->eraseInformationMap(cqe.UserData);
                // std::cout << "\nstrlen():   " << 0 << std::endl;
            }
        }
        return 0;
    }
    void createThread()
    {
        threadHandle = CreateThread(nullptr, 0, &smilyIoRing::ThreadFunction, this, 0, nullptr);
        if (threadHandle == nullptr)
        {
            // throw std::runtime_error("CreateThread:S_FALSE:");
            std::cout << "CreateThread:S_FALSE:" << std::endl;
        }
    }
    void setInformationMap(UINT_PTR handle, informationMap::readFileInformation value)
    {
        std::lock_guard<std::mutex> guard(informationMapoMutex);
        informations.push(handle, value);
    }
    void eraseInformationMap(UINT_PTR handle)
    {
        std::lock_guard<std::mutex> guard(informationMapoMutex);
        informations.erase(handle);
    }

    informationMap::readFileInformation getInformationMap(UINT_PTR handle)
    {
        std::lock_guard<std::mutex> guard(informationMapoMutex);
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
    void setUserDataDeque(UINT_PTR userData)
    {
        std::lock_guard<std::mutex> guard(userDataMutex);
        userDataDeque.push_back(userData);
    }

    void smilyBuildIoRingRegisterBuffers(UINT32 size)
    {
        UINT32 count = (size % allocSize == 0) ? (size / allocSize) : (size / allocSize + 1);
        for (UINT32 i = 0; i < count; i++)
        {
            // bufferInfo.emplace_back(VirtualAlloc(NULL, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), allocSize);
            bufferInfo.emplace_back(new char[allocSize]);
        }
        HRESULT flag = BuildIoRingRegisterBuffers(ioRing, count, bufferInfo.data(), userDataRegisterBuffers);
        if (flag == S_OK)
        {
            // std::cout << "BuildIoRingRegisterBuffers::S_OK" << std::endl;
        }
        else
        {
            // throw std::runtime_error("BuildIoRingRegisterBuffers:S_FALSE:");
            std::cout << "BuildIoRingRegisterBuffers:S_FALSE:" << std::endl;
        }

        for (UINT32 i = 1; i < count; i++)
        {
            registerBuffersDeque.push_back(bufferInfo.data()[i].Address);
        }
    }
    void smilySetIoRingCompletionEvent(HANDLE hEvent)
    {
        HRESULT flag = SetIoRingCompletionEvent(ioRing, hEvent);
        if (flag == S_OK)
        {
            // std::cout << "SetIoRingCompletionEvent::S_OK" << std::endl;
        }
        else
        {
            // throw std::runtime_error("SetIoRingCompletionEvent:S_FALSE:");
            std::cout << "SetIoRingCompletionEvent:S_FALSE:" << std::endl;
        }
    }
    void smilySubmitIoRing()
    {
        UINT32 submittedEntries;
        HRESULT flag = SubmitIoRing(ioRing, 0, 1, &submittedEntries);
        // if (flag == S_OK)
        // {
        //     std::cout << "SubmitIoRing::S_OK" << std::endl;
        // }
        // else
        // {
        //     // throw std::runtime_error("SubmitIoRing:S_FALSE:");
        //     std::cout << "SubmitIoRing:S_FALSE:" << x<<std::endl;
        // }
    }
    void smilyBuildIoRingReadFile(HANDLE handle, HANDLE completionEvent, UINT64 fileOffset = 0, bool userDataFlag = false, UINT_PTR userData_ = 0)
    {
        UINT32 size = allocSize;
        auto Address = getregisterBuffers();
        UINT_PTR userData;
        if (!userDataFlag)
        {
            userData = getUserDataDeque();
        }
        else
        {
            userData = userData_;
        }
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            // throw std::runtime_error("CreateEvent:S_FALSE:");
            std::cout << "CreateEvent:S_FALSE:" << std::endl;
        }
        // setEventHandleDeque(eventHandle);
        //  smilySetIoRingCompletionEvent(eventHandle);
        setInformationMap(userData, informationMap::readFileInformation{handle, completionEvent, size, fileOffset, eventHandle, Address, smilyIoRing::ioRingType::readFile});
        HRESULT flag = BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(Address), size, fileOffset, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
        if (flag == S_OK)
        {
            // std::cout << "fileOffset::" << fileOffset << std::endl;
            // std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
        }
        else if (IORING_E_SUBMISSION_QUEUE_FULL == flag)
        {
            std::cout << "BuildIoRingReadFile:IORING_E_SUBMISSION_QUEUE_FULL:" << std::endl;
            // throw std::runtime_error("BuildIoRingReadFile:S_FALSE:");
            // std::cout << "BuildIoRingReadFile:S_FALSE:" << std::endl;
        }
        smilySubmitIoRing();
    }

    void smilyBuildIoRingWriteFile(HANDLE handle, HANDLE completionEvent, UINT64 fileOffset = 0, bool userDataFlag = false, UINT_PTR userData_ = 0)
    {
        UINT32 size = 512;
        auto t = getregisterBuffers();
        auto Address = (char *)t;
        for (int i = 0; i < 512; i++)
        {
            *Address = '8';
            Address++;
        }
        UINT_PTR userData;
        if (!userDataFlag)
        {
            userData = getUserDataDeque();
        }
        else
        {
            userData = userData_;
        }
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            // throw std::runtime_error("CreateEvent:S_FALSE:");
            std::cout << "CreateEvent:S_FALSE:" << std::endl;
        }
        // setEventHandleDeque(eventHandle);
        //  smilySetIoRingCompletionEvent(eventHandle);
        setInformationMap(userData, informationMap::readFileInformation{handle, completionEvent, size, fileOffset, eventHandle, t, smilyIoRing::ioRingType::writeFile});
        HRESULT flag = BuildIoRingWriteFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(t), size, fileOffset, FILE_WRITE_FLAGS_WRITE_THROUGH, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
        if (flag == S_OK)
        {
            // std::cout << "fileOffset::" << fileOffset << std::endl;
            std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
        }
        else if (IORING_E_SUBMISSION_QUEUE_FULL == flag)
        {
            std::cout << "BuildIoRingReadFile:IORING_E_SUBMISSION_QUEUE_FULL:" << std::endl;
            // throw std::runtime_error("BuildIoRingReadFile:S_FALSE:");
            // std::cout << "BuildIoRingReadFile:S_FALSE:" << std::endl;
        }
        smilySubmitIoRing();
    }

    bool smilyPopIoRingCompletion(IORING_CQE &cqe)
    {
        HRESULT flag = PopIoRingCompletion(ioRing, &cqe);
        if (flag == S_OK)
        {
            // std::cout << "PopIoRingCompletion::S_OK" << std::endl;
            // std::cout << "Information: " << cqe.Information << std::endl;
            // std::cout << "ResultCode: " << cqe.ResultCode << std::endl;
            // std::cout << "UserData: " << cqe.UserData << std::endl;
            // getUserDataDeque(cqe.UserData);
            return true;
        }
        else
        {
            return false;
        }
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