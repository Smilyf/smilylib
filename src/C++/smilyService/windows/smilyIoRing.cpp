#include <windows.h>
#include <ioringapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <atomic>
namespace smily
{
    struct smilyIoRing
    {
    public:
        enum userDataEnum : UINT_PTR
        {
            userDataRegisterBuffers,
            userDataRegisterFileHandles
        };
        enum ioRingType
        {
            readFile,
            writeFile
        };
        class informationMap
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

                                    void *Address_, smilyIoRing::ioRingType type_, HANDLE eventHandle_ = nullptr)
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
            std::map<UINT_PTR, readFileInformation> handleMap;
            std::mutex informationMapoMutex;
            informationMap() = default;
            void setInformationMap(UINT_PTR handle, informationMap::readFileInformation value)
            {
                std::lock_guard<std::mutex> guard(informationMapoMutex);
                if (handleMap.find(handle) != handleMap.end())
                {
                    handleMap.at(handle) = value;
                }
                else
                {
                    handleMap.emplace(handle, value);
                }
            }
            void eraseInformationMap(UINT_PTR handle)
            {
                std::lock_guard<std::mutex> guard(informationMapoMutex);
                handleMap.erase(handle);
            }

            informationMap::readFileInformation getInformationMap(UINT_PTR handle)
            {
                std::lock_guard<std::mutex> guard(informationMapoMutex);
                return handleMap.at(handle);
            }
        };

        class RegisterBuffersDeque
        {
        public:
            std::mutex registerBufferMutex;
            std::deque<void *> registerBuffersDeque{};
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
        };
        class UserDataDeque
        {
        public:
            std::deque<UINT_PTR> userDataDeque{};
            std::mutex userDataMutex;
            UINT_PTR userDataDequesize;
            void setUserDataDequesize(UINT_PTR size)
            {
                userDataDequesize = size;
            }
            void init(UINT_PTR size)
            {
                userDataDequesize = size;
                for (UINT_PTR i = 10; i < userDataDequesize; i++)
                {
                    userDataDeque.push_back(i);
                }
            }
            UINT_PTR getUserDataDequesize()
            {
                return userDataDequesize;
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
        };

        class EventHandleDeque
        {
        public:
            std::mutex eventHandleMutex;
            std::deque<HANDLE> eventHandleDeque{};
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
        };

        IORING_CAPABILITIES t;
        HIORING ioRing;
        UINT32 allocSize = 1024 * 4;

        RegisterBuffersDeque buffersDeque;
        informationMap informations;
        UserDataDeque userDataDeques;
        EventHandleDeque eventHandleDeques;

        HANDLE threadHandle;

        bool messageFlag = true;

        std::vector<IORING_BUFFER_INFO> bufferInfo;

        std::atomic<bool> threadAtomic{true};

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
            userDataDeques.init(t.MaxSubmissionQueueSize);
            createThread();
        }
        ~smilyIoRing()
        {
            for (auto pMemory : bufferInfo)
            {
                delete pMemory.Address;
            }
            threadAtomic.store(false);
            WaitForSingleObject(threadHandle, INFINITE);
            CloseHandle(threadHandle);
            CloseIoRing(ioRing);
        }
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
                    auto information = smilyIoRingThread->informations.getInformationMap(cqe.UserData);

                    if (smilyIoRing::ioRingType::readFile == information.type)
                    { // std::cout << "strlen():   " << information.size << "    " << std::endl;
                        auto Address = information.Address;
                        char *charPtr = reinterpret_cast<char *>(Address);
                        smilyIoRingThread->buffersDeque.setregisterBuffers(Address);
                        // for (size_t i = 0; i < cqe.Information; i++)
                        // {
                        //     std::cout << charPtr[i];
                        // }
                        smilyIoRingThread->smilyBuildIoRingWriteFile(information.handle, information.eventHandle);
                        // std::cout << "====================================================" << std::endl;
                        information.fileOffset += cqe.Information;
                        information.size += cqe.Information;
                        smilyIoRingThread->informations.setInformationMap(cqe.UserData, information);
                        smilyIoRingThread->smilyBuildIoRingReadFile(information.handle, information.eventHandle, information.fileOffset, true, cqe.UserData);
                    }
                    if (smilyIoRing::ioRingType::writeFile == information.type)
                    {
                        smilyIoRingThread->smilyBuildIoRingWriteFile(information.handle, information.eventHandle, information.fileOffset, true, cqe.UserData);
                    }
                }
                else
                {
                    auto information = smilyIoRingThread->informations.getInformationMap(cqe.UserData);
                    CloseHandle(information.handle);
                    smilyIoRingThread->userDataDeques.setUserDataDeque(cqe.UserData);
                    smilyIoRingThread->informations.eraseInformationMap(cqe.UserData);
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
                // std::cout << "CreateThread:S_FALSE:" << std::endl;
            }
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
                // std::cout << "BuildIoRingRegisterBuffers:S_FALSE:" << std::endl;
            }
            for (UINT32 i = 0; i < count; i++)
            {
                buffersDeque.setregisterBuffers(bufferInfo.data()[i].Address);
            }
            smilySubmitIoRing();
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
                // std::cout << "SetIoRingCompletionEvent:S_FALSE:" << std::endl;
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
            auto Address = buffersDeque.getregisterBuffers();
            UINT_PTR userData;
            if (!userDataFlag)
            {
                userData = userDataDeques.getUserDataDeque();
                // HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                // if (eventHandle == nullptr)
                // {
                //     // throw std::runtime_error("CreateEvent:S_FALSE:");
                //     // std::cout << "CreateEvent:S_FALSE:" << std::endl;
                // }
                // eventHandleDeques.setEventHandleDeque(eventHandle);
                // smilySetIoRingCompletionEvent(eventHandle);
                informations.setInformationMap(userData, informationMap::readFileInformation{handle, completionEvent, 0, fileOffset, Address, smilyIoRing::ioRingType::readFile, nullptr});
                userData = userData_;
            }
            else
            {
                userData = userData_;
            }
            HRESULT flag = BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(Address), size, fileOffset, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
            if (flag == S_OK)
            {
                // std::cout << "fileOffset::" << fileOffset << std::endl;
                // std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
                smilySubmitIoRing();
            }
            else if (IORING_E_SUBMISSION_QUEUE_FULL == flag)
            {
                std::cout << "BuildIoRingReadFile:IORING_E_SUBMISSION_QUEUE_FULL:" << std::endl;
                while (true)
                {
                    flag = BuildIoRingReadFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(Address), size, fileOffset, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
                    if (flag == S_OK)
                    {
                        // std::cout << "fileOffset::" << fileOffset << std::endl;
                        // std::cout << "BuildIoRingReadFile::S_OK" << std::endl;
                        smilySubmitIoRing();
                        break;
                    }
                }

                // throw std::runtime_error("BuildIoRingReadFile:S_FALSE:");
                // std::cout << "BuildIoRingReadFile:S_FALSE:" << std::endl;
            }
        }

        void smilyBuildIoRingWriteFile(HANDLE handle, HANDLE completionEvent, UINT64 fileOffset = 0, bool userDataFlag = false, UINT_PTR userData_ = 0)
        {
            UINT32 size = 512;
            auto t = buffersDeque.getregisterBuffers();
            auto Address = (char *)t;
            for (int i = 0; i < 512; i++)
            {
                *Address = '8';
                Address++;
            }
            UINT_PTR userData;
            if (!userDataFlag)
            {
                userData = userDataDeques.getUserDataDeque();
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
            informations.setInformationMap(userData, informationMap::readFileInformation{handle, completionEvent, size, fileOffset, t, smilyIoRing::ioRingType::writeFile, nullptr});
            HRESULT flag = BuildIoRingWriteFile(ioRing, IoRingHandleRefFromHandle(handle), IoRingBufferRefFromPointer(t), size, fileOffset, FILE_WRITE_FLAGS_WRITE_THROUGH, userData, IOSQE_FLAGS_DRAIN_PRECEDING_OPS);
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

        void smilyBuildIoRingRegisterFileHandles(UINT32 count, const HANDLE *handles)
        {

            HRESULT flag = BuildIoRingRegisterFileHandles(ioRing, count, handles, userDataRegisterFileHandles);
            if (flag == S_OK)
            {
                // std::cout << "BuildIoRingRegisterFileHandles::S_OK" << std::endl;
            }
            else
            {
                // std::cout << "BuildIoRingRegisterFileHandles:S_FALSE:" << std::endl;
            }
            smilySubmitIoRing();
        }
    };
};
