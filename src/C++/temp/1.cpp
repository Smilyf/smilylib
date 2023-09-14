// #include <ntstatus.h>
// #define WIN32_NO_STATUS
// #include <Windows.h>
// #include <cstdio>
// #include <ioringapi.h>


// typedef struct _IO_RING_STRUCTV1
// {
//     ULONG IoRingVersion;
//     ULONG SubmissionQueueSize;
//     ULONG CompletionQueueSize;
//     ULONG RequiredFlags;
//     ULONG AdvisoryFlags;
// } IO_RING_STRUCTV1, *PIO_RING_STRUCTV1;

// typedef struct _IORING_QUEUE_HEAD
// {
//     ULONG Head;
//     ULONG Tail;
//     ULONG64 Flags;
// } IORING_QUEUE_HEAD, *PIORING_QUEUE_HEAD;

// typedef struct _NT_IORING_INFO
// {
//     ULONG Version;
//     IORING_CREATE_FLAGS Flags;
//     ULONG SubmissionQueueSize;
//     ULONG SubQueueSizeMask;
//     ULONG CompletionQueueSize;
//     ULONG CompQueueSizeMask;
//     PIORING_QUEUE_HEAD SubQueueBase;
//     PVOID CompQueueBase;
// } NT_IORING_INFO, *PNT_IORING_INFO;

// typedef struct _NT_IORING_SQE
// {
//     ULONG Opcode;
//     ULONG Flags;
//     HANDLE FileRef;
//     LARGE_INTEGER FileOffset;
//     PVOID Buffer;
//     ULONG BufferSize;
//     ULONG BufferOffset;
//     ULONG Key;
//     PVOID Unknown;
//     PVOID UserData;
//     PVOID stuff1;
//     PVOID stuff2;
//     PVOID stuff3;
//     PVOID stuff4;
// } NT_IORING_SQE, *PNT_IORING_SQE;

// EXTERN_C_START
// NTSTATUS
// NtSubmitIoRing (
//     _In_ HANDLE Handle,
//     _In_ IORING_CREATE_REQUIRED_FLAGS Flags,
//     _In_ ULONG EntryCount,
//     _In_ PLARGE_INTEGER Timeout
//     );

// NTSTATUS
// NtCreateIoRing (
//     _Out_ PHANDLE pIoRingHandle,
//     _In_ ULONG CreateParametersSize,
//     _In_ PIO_RING_STRUCTV1 CreateParameters,
//     _In_ ULONG OutputParametersSize,
//     _Out_ PNT_IORING_INFO pRingInfo
//     );

// NTSTATUS
// NtClose (
//     _In_ HANDLE Handle
//     );

// EXTERN_C_END



// void IoRingKernelBase ()
// {
//     HRESULT result;
//     HIORING handle;
//     IORING_CREATE_FLAGS flags;
//     IORING_HANDLE_REF requestDataFile;
//     IORING_BUFFER_REF requestDataBuffer;
//     UINT32 submittedEntries;
//     HANDLE hFile = NULL;
//     ULONG sizeToRead = 0x200;
//     PVOID *buffer = NULL;
//     ULONG64 endOfBuffer;

//     flags.Required = IORING_CREATE_REQUIRED_FLAGS_NONE;
//     flags.Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE;
//     result = CreateIoRing(IORING_VERSION_1, flags, 1, 1, &handle);
//     if (!SUCCEEDED(result))
//     {
//         printf("Failed creating IO ring handle: 0x%x\n", result);
//         goto Exit;
//     }

//     hFile = CreateFile(L"C:\\Windows\\System32\\notepad.exe",
//                        GENERIC_READ,
//                        0,
//                        NULL,
//                        OPEN_EXISTING,
//                        FILE_ATTRIBUTE_NORMAL,
//                        NULL);
//     if (hFile == INVALID_HANDLE_VALUE)
//     {
//         printf("Failed opening file handle: 0x%x\n", GetLastError());
//         goto Exit;
//     }
//     requestDataFile.Kind = IORING_REF_RAW;
//     requestDataFile.Handle = hFile;
//     requestDataBuffer.Kind = IORING_REF_RAW;
//     buffer = (PVOID*)VirtualAlloc(NULL,
//                                   sizeToRead,
//                                   MEM_COMMIT,
//                                   PAGE_READWRITE);
//     if (buffer == NULL)
//     {
//         printf("Failed to allocate memory\n");
//         goto Exit;
//     }
//     requestDataBuffer.Buffer = buffer;
//     result = BuildIoRingReadFile(handle,
//                                  requestDataFile,
//                                  requestDataBuffer,
//                                  sizeToRead,
//                                  0,
//                                  NULL,
//                                  IOSQE_FLAGS_NONE);
//     if (!SUCCEEDED(result))
//     {
//         printf("Failed building IO ring read file structure: 0x%x\n", result);
//         goto Exit;
//     }

//     result = SubmitIoRing(handle, 1, 10000, &submittedEntries);
//     if (!SUCCEEDED(result))
//     {
//         printf("Failed submitting IO ring: 0x%x\n", result);
//         goto Exit;
//     }
//     printf("Data from file:\n");
//     endOfBuffer = (ULONG64)buffer + sizeToRead;
//     for (; (ULONG64)buffer < endOfBuffer; buffer++)
//     {
//         printf("%p ", *buffer);
//     }
//     printf("\n");

// Exit:
//     if (handle != 0)
//     {
//         CloseIoRing(handle);
//     }
//     if (hFile)
//     {
//         NtClose(hFile);
//     }
//     if (buffer)
//     {
//         VirtualFree(buffer, NULL, MEM_RELEASE);
//     }
// }

// int main ()
// {
//     IoRingKernelBase();
//     ExitProcess(0);
// }