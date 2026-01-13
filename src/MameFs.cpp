#include "MameFs.h"
#include "Downloader.h"
#include <iostream>
#include <mutex>
#include <shlwapi.h>
#include <stddef.h>
#include <winternl.h>

#pragma comment(lib, "shlwapi.lib")

// Safe helper to convert wstring to string (ANSI) to avoid conversion warnings
static std::string WToStr(const std::wstring &wstr) {
  if (wstr.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  if (size_needed <= 0)
    return std::string();
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

// Global download mutex to prevent concurrent writing to the same file
static std::mutex g_McrDownloadMutex;

static UINT64 FileTimeToInt64(const FILETIME &ft) {
  return ((UINT64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

struct MameFileContext {
  HANDLE Handle;
  HANDLE FindHandle;
  WIN32_FIND_DATAW FindData;
  bool IsDirectory;
  std::wstring Path;

  MameFileContext()
      : Handle(INVALID_HANDLE_VALUE), FindHandle(INVALID_HANDLE_VALUE),
        IsDirectory(false) {}
};

std::wstring MameFs::m_CacheDir;
std::wstring MameFs::m_BaseUrl;
std::wstring MameFs::m_ZipPrefix;
std::wstring MameFs::m_SevenZipPrefix;
bool MameFs::m_Enable7z = false;
bool MameFs::m_DebugMode = false;

std::wstring MameFs::GetLocalPath(PCWSTR fileName) {
  if (fileName[0] == L'\\')
    return m_CacheDir + fileName;
  return m_CacheDir + L"\\" + fileName;
}

bool MameFs::HasValidBasename(const std::wstring &fileName) {
  size_t lastSlash = fileName.rfind(L'\\');
  std::wstring baseName = (lastSlash != std::wstring::npos)
                              ? fileName.substr(lastSlash + 1)
                              : fileName;
  if (baseName.empty() || baseName[0] == L'.')
    return false;
  size_t dotPos = baseName.rfind(L'.');
  return (dotPos != std::wstring::npos && dotPos > 0);
}

static void FillFileInfo(HANDLE hFile, FSP_FSCTL_FILE_INFO *FileInfo) {
  BY_HANDLE_FILE_INFORMATION info;
  if (GetFileInformationByHandle(hFile, &info)) {
    FileInfo->FileAttributes = info.dwFileAttributes;
    FileInfo->FileSize = ((UINT64)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    FileInfo->AllocationSize = (FileInfo->FileSize + 4095) & ~4095;
    FileInfo->CreationTime = FileTimeToInt64(info.ftCreationTime);
    FileInfo->LastAccessTime = FileTimeToInt64(info.ftLastAccessTime);
    FileInfo->LastWriteTime = FileTimeToInt64(info.ftLastWriteTime);
    FileInfo->ChangeTime = FileInfo->LastWriteTime;
    FileInfo->IndexNumber = 0;
    FileInfo->HardLinks = 1;
    FileInfo->ReparseTag = 0;
  }
}

int MameFs::Run(const std::wstring &mountPoint, const std::wstring &cacheDir,
                const std::wstring &baseUrl, const std::wstring &zipPrefix,
                const std::wstring &sevenZipPrefix, bool enable7z,
                bool debugMode) {
  m_CacheDir = cacheDir;
  m_BaseUrl = baseUrl;
  m_ZipPrefix = zipPrefix;
  m_SevenZipPrefix = sevenZipPrefix;
  m_Enable7z = enable7z;
  m_DebugMode = debugMode;

  CreateDirectoryW(m_CacheDir.c_str(), NULL);

  FSP_FILE_SYSTEM *FileSystem = NULL;
  FSP_FILE_SYSTEM_INTERFACE *Interface = new FSP_FILE_SYSTEM_INTERFACE();
  memset(Interface, 0, sizeof(*Interface));

  Interface->GetVolumeInfo = SGetVolumeInfo;
  Interface->GetSecurity = SGetSecurity;
  Interface->GetSecurityByName = SGetSecurityByName;
  Interface->Create = SCreate;
  Interface->Open = SOpen;
  Interface->Read = SRead;
  Interface->Close = SClose;
  Interface->Cleanup = SCleanup;
  Interface->ReadDirectory = SReadDirectory;
  Interface->GetFileInfo = SGetFileInfo;
  Interface->Overwrite = SOverwrite;

  FSP_FSCTL_VOLUME_PARAMS VolumeParams = {0};
  VolumeParams.SectorSize = 4096;
  VolumeParams.SectorsPerAllocationUnit = 1;
  VolumeParams.MaxComponentLength = 255;
  VolumeParams.FileInfoTimeout = 0;
  VolumeParams.CaseSensitiveSearch = 0;
  VolumeParams.CasePreservedNames = 1;
  VolumeParams.UnicodeOnDisk = 1;
  VolumeParams.PersistentAcls = 0;

  NTSTATUS Status = STATUS_UNSUCCESSFUL;

  for (int i = 0; i < 5; ++i) {
    std::wstring uniqueSuffix = std::to_wstring(GetTickCount() + i);
    std::wstring prefix = L"\\mame-mcr" + uniqueSuffix + L"\\roms";
    std::wstring name = L"MameCloudRom" + uniqueSuffix;
    wcscpy_s(VolumeParams.Prefix, 64, prefix.c_str());
    wcscpy_s(VolumeParams.FileSystemName, 64, name.c_str());
    Status =
        FspFileSystemCreate((PWSTR)0, &VolumeParams, Interface, &FileSystem);
    if (NT_SUCCESS(Status))
      break;
  }

  if (!NT_SUCCESS(Status)) {
    VolumeParams.Prefix[0] = L'\0';
    wcscpy_s(VolumeParams.FileSystemName, 32, L"MameCloudRomDisk");
    Status = FspFileSystemCreate((PWSTR)L"\\Device\\WinFsp.Disk", &VolumeParams,
                                 Interface, &FileSystem);
  }

  if (!NT_SUCCESS(Status)) {
    std::cerr << "[ERROR] Failed to start FileSystem. Status: 0x" << std::hex
              << Status << std::endl;
    return -1;
  }

  Status = FspFileSystemSetMountPoint(FileSystem, (PWSTR)mountPoint.c_str());
  if (!NT_SUCCESS(Status)) {
    std::cerr << "[ERROR] Mount failed at " << WToStr(mountPoint) << std::endl;
    FspFileSystemDelete(FileSystem);
    return -1;
  }

  if (m_DebugMode) {
    FspDebugLogSetHandle(GetStdHandle(STD_ERROR_HANDLE));
  }

  Status = FspFileSystemStartDispatcher(FileSystem, 0);
  if (!NT_SUCCESS(Status)) {
    FspFileSystemDelete(FileSystem);
    return -1;
  }

  std::cout << "==========================================" << std::endl;
  std::cout << "MCR Service is running." << std::endl;
  std::wcout << L"Mounted successfully at " << mountPoint << std::endl;
  std::cout << "[IMPORTANT] Keep this window OPEN while playing." << std::endl;
  std::cout << "Press Ctrl+C to stop the service." << std::endl;
  std::cout << "==========================================" << std::endl;

  try {
    while (true) {
      Sleep(1000);
      if (FileSystem->DispatcherThread == NULL)
        break;
    }
  } catch (...) {
  }

  FspFileSystemStopDispatcher(FileSystem);
  FspFileSystemDelete(FileSystem);

  return 0;
}

NTSTATUS MameFs::SGetVolumeInfo(FSP_FILE_SYSTEM *FileSystem,
                                FSP_FSCTL_VOLUME_INFO *VolumeInfo) {
  VolumeInfo->TotalSize = 100LL * 1024 * 1024 * 1024;
  VolumeInfo->FreeSize = 80LL * 1024 * 1024 * 1024;
  wcscpy_s(VolumeInfo->VolumeLabel, 32, L"MameCloudRom");
  VolumeInfo->VolumeLabelLength =
      (UINT16)(wcslen(VolumeInfo->VolumeLabel) * sizeof(WCHAR));
  return STATUS_SUCCESS;
}

NTSTATUS MameFs::SGetSecurity(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                              PSECURITY_DESCRIPTOR SecurityDescriptor,
                              SIZE_T *SecurityDescriptorSize) {
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MameFs::SGetSecurityByName(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName,
                                    PUINT32 PFileAttributes,
                                    PSECURITY_DESCRIPTOR SecurityDescriptor,
                                    SIZE_T *PSecurityDescriptorSize) {
  std::wstring localPath = GetLocalPath(FileName);
  std::wstring fileNameStr = FileName;

  if (fileNameStr == L"\\" || fileNameStr.empty()) {
    if (PFileAttributes)
      *PFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    if (PSecurityDescriptorSize)
      *PSecurityDescriptorSize = 0;
    return STATUS_SUCCESS;
  }

  // Use exactly v0.2 verified flags for security checks
  HANDLE hFile =
      CreateFileW(localPath.c_str(), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      std::wstring nameStr = fileNameStr;
      if (!nameStr.empty() && nameStr[0] == L'\\')
        nameStr = nameStr.substr(1);

      bool isZip = (nameStr.length() > 4 &&
                    nameStr.substr(nameStr.length() - 4) == L".zip");
      bool is7z = (nameStr.length() > 3 &&
                   nameStr.substr(nameStr.length() - 3) == L".7z");

      if (isZip || (is7z && m_Enable7z)) {
        if (m_DebugMode)
          std::wcout << L"[MCR] File missing locally: " << nameStr
                     << L". Attempting download..." << std::endl;

        std::wstring url = m_BaseUrl;
        if (!url.empty() && url.back() == L'/')
          url.pop_back();

        if (isZip) {
          if (!m_ZipPrefix.empty()) {
            url += L"/";
            url += m_ZipPrefix;
          }
        } else if (is7z) {
          if (!m_SevenZipPrefix.empty()) {
            url += L"/";
            url += m_SevenZipPrefix;
          }
        }

        if (!url.empty() && url.back() != L'/')
          url += L"/";
        url += nameStr;

        {
          std::lock_guard<std::mutex> lock(g_McrDownloadMutex);
          if (GetFileAttributesW(localPath.c_str()) ==
              INVALID_FILE_ATTRIBUTES) {
            if (Downloader::Download(url, localPath, m_DebugMode)) {
              hFile = CreateFileW(
                  localPath.c_str(), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            }
          } else {
            hFile = CreateFileW(
                localPath.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
          }
        }
      }
    }
  }

  if (hFile == INVALID_HANDLE_VALUE)
    return STATUS_OBJECT_NAME_NOT_FOUND;

  BY_HANDLE_FILE_INFORMATION info;
  BOOL result = GetFileInformationByHandle(hFile, &info);
  CloseHandle(hFile);

  if (!result)
    return STATUS_UNSUCCESSFUL;
  if (PFileAttributes)
    *PFileAttributes = info.dwFileAttributes;
  if (PSecurityDescriptorSize)
    *PSecurityDescriptorSize = 0;

  return STATUS_SUCCESS;
}

NTSTATUS MameFs::SCreate(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName,
                         UINT32 CreateOptions, UINT32 GrantedAccess,
                         UINT32 FileAttributes,
                         PSECURITY_DESCRIPTOR SecurityDescriptor,
                         UINT64 AllocationSize, PVOID *PFileContext,
                         FSP_FSCTL_FILE_INFO *FileInfo) {
  return SOpen(FileSystem, FileName, CreateOptions, GrantedAccess, PFileContext,
               FileInfo);
}

NTSTATUS MameFs::SOpen(FSP_FILE_SYSTEM *FileSystem, PWSTR FileName,
                       UINT32 CreateOptions, UINT32 GrantedAccess,
                       PVOID *PFileContext, FSP_FSCTL_FILE_INFO *FileInfo) {
  std::wstring localPath = GetLocalPath(FileName);
  std::wstring fileNameStr = FileName;

  if (wcscmp(FileName, L"\\") != 0 && !(CreateOptions & FILE_DIRECTORY_FILE)) {
    bool isZip = (fileNameStr.length() > 4 &&
                  fileNameStr.substr(fileNameStr.length() - 4) == L".zip");
    bool is7z = (fileNameStr.length() > 3 &&
                 fileNameStr.substr(fileNameStr.length() - 3) == L".7z");
    bool isDataRequest = (GrantedAccess & FILE_READ_DATA) != 0;

    if (isZip || (is7z && m_Enable7z)) {
      if (fileNameStr.find(L"$") != std::wstring::npos ||
          fileNameStr.find(L"desktop.ini") != std::wstring::npos) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
      }

      if (HasValidBasename(fileNameStr)) {
        if (isDataRequest) {
          std::lock_guard<std::mutex> lock(g_McrDownloadMutex);
          if (GetFileAttributesW(localPath.c_str()) ==
              INVALID_FILE_ATTRIBUTES) {
            std::wstring url = m_BaseUrl;
            if (!url.empty() && url.back() == L'/')
              url.pop_back();

            if (isZip) {
              if (!m_ZipPrefix.empty()) {
                url += L"/";
                url += m_ZipPrefix;
              }
            } else if (is7z) {
              if (!m_SevenZipPrefix.empty()) {
                url += L"/";
                url += m_SevenZipPrefix;
              }
            }

            if (!url.empty() && url.back() != L'/')
              url += L"/";

            std::wstring relPath = FileName;
            for (auto &c : relPath)
              if (c == L'\\')
                c = L'/';
            if (!relPath.empty() && relPath[0] == L'/')
              relPath = relPath.substr(1);

            url += relPath;
            if (m_DebugMode)
              std::wcout << L"[DEBUG] Proxy Download: " << fileNameStr
                         << L" (Data Read Requested)" << std::endl;
            if (!Downloader::Download(url, localPath, m_DebugMode))
              return STATUS_OBJECT_NAME_NOT_FOUND;
          }
        } else {
          if (GetFileAttributesW(localPath.c_str()) ==
              INVALID_FILE_ATTRIBUTES) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
          }
        }
      } else {
        return STATUS_OBJECT_NAME_NOT_FOUND;
      }
    }
  }

  // USE EXACTLY v0.2 VERIFIED GENERIC_READ FOR STABILITY
  HANDLE hFile =
      CreateFileW(localPath.c_str(), GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
      return STATUS_OBJECT_NAME_NOT_FOUND;
    if (err == ERROR_ACCESS_DENIED)
      return STATUS_ACCESS_DENIED;
    return STATUS_UNSUCCESSFUL;
  }

  MameFileContext *ctx = new MameFileContext();
  ctx->Handle = hFile;
  ctx->Path = localPath;
  ctx->IsDirectory =
      (GetFileAttributesW(localPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
  *PFileContext = ctx;

  FillFileInfo(hFile, FileInfo);
  return STATUS_SUCCESS;
}

NTSTATUS MameFs::SRead(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                       PVOID Buffer, UINT64 Offset, ULONG Length,
                       PULONG PBytesTransferred) {
  MameFileContext *ctx = (MameFileContext *)FileContext;
  if (!ctx || ctx->Handle == INVALID_HANDLE_VALUE)
    return STATUS_INVALID_HANDLE;

  OVERLAPPED ov = {0};
  ov.Offset = (DWORD)Offset;
  ov.OffsetHigh = (DWORD)(Offset >> 32);
  DWORD bytesRead = 0;

  if (!ReadFile(ctx->Handle, Buffer, Length, &bytesRead, &ov)) {
    DWORD err = GetLastError();
    if (err == ERROR_HANDLE_EOF) {
      *PBytesTransferred = 0;
      return STATUS_END_OF_FILE;
    }
    return STATUS_UNSUCCESSFUL;
  }

  *PBytesTransferred = bytesRead;
  return STATUS_SUCCESS;
}

void MameFs::SClose(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext) {
  MameFileContext *ctx = (MameFileContext *)FileContext;
  if (ctx) {
    if (m_DebugMode)
      std::wcout << L"[Close] Context kept for stability." << std::endl;
    // v0.2 Stability Hack: Do not close handles or delete context.
  }
}

void MameFs::SCleanup(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                      PWSTR FileName, ULONG Flags) {}

NTSTATUS MameFs::SOverwrite(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                            UINT32 FileAttributes,
                            BOOLEAN ReplaceFileAttributes,
                            UINT64 AllocationSize,
                            FSP_FSCTL_FILE_INFO *FileInfo) {
  return STATUS_MEDIA_WRITE_PROTECTED;
}

NTSTATUS MameFs::SGetFileInfo(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                              FSP_FSCTL_FILE_INFO *FileInfo) {
  MameFileContext *ctx = (MameFileContext *)FileContext;
  if (!ctx || ctx->Handle == INVALID_HANDLE_VALUE)
    return STATUS_INVALID_HANDLE;
  FillFileInfo(ctx->Handle, FileInfo);
  return STATUS_SUCCESS;
}

NTSTATUS MameFs::SReadDirectory(FSP_FILE_SYSTEM *FileSystem, PVOID FileContext,
                                PWSTR Pattern, PWSTR Marker, PVOID Buffer,
                                ULONG Length, PULONG PBytesTransferred) {
  MameFileContext *ctx = (MameFileContext *)FileContext;
  if (!ctx || !ctx->IsDirectory)
    return STATUS_INVALID_HANDLE;

  if (Marker == NULL) {
    if (ctx->FindHandle != INVALID_HANDLE_VALUE) {
      FindClose(ctx->FindHandle);
      ctx->FindHandle = INVALID_HANDLE_VALUE;
    }
  }

  if (ctx->FindHandle == INVALID_HANDLE_VALUE) {
    std::wstring searchPath = ctx->Path + L"\\*";
    ctx->FindHandle = FindFirstFileW(searchPath.c_str(), &ctx->FindData);
    if (ctx->FindHandle == INVALID_HANDLE_VALUE)
      return (GetLastError() == ERROR_FILE_NOT_FOUND) ? STATUS_NO_MORE_FILES
                                                      : STATUS_UNSUCCESSFUL;

    if (Marker != NULL) {
      while (wcscmp(ctx->FindData.cFileName, Marker) != 0) {
        if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
          return STATUS_NO_MORE_FILES;
      }
      if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
        return STATUS_NO_MORE_FILES;
    }
  } else if (Marker != NULL) {
    if (wcscmp(ctx->FindData.cFileName, Marker) == 0) {
      if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
        return STATUS_NO_MORE_FILES;
    } else {
      FindClose(ctx->FindHandle);
      std::wstring searchPath = ctx->Path + L"\\*";
      ctx->FindHandle = FindFirstFileW(searchPath.c_str(), &ctx->FindData);
      if (ctx->FindHandle == INVALID_HANDLE_VALUE)
        return STATUS_NO_MORE_FILES;
      while (wcscmp(ctx->FindData.cFileName, Marker) != 0) {
        if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
          return STATUS_NO_MORE_FILES;
      }
      if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
        return STATUS_NO_MORE_FILES;
    }
  }

  while (true) {
    size_t nameLen = wcslen(ctx->FindData.cFileName);
    UINT16 nameSize = (UINT16)(nameLen * sizeof(WCHAR));
    UINT16 entrySize =
        (UINT16)((offsetof(FSP_FSCTL_DIR_INFO, FileNameBuf) + nameSize + 7) &
                 ~7);

    if (*PBytesTransferred + entrySize > Length)
      break;

    BYTE entryBuf[1024] = {0};
    FSP_FSCTL_DIR_INFO *pEntry = (FSP_FSCTL_DIR_INFO *)entryBuf;
    pEntry->Size = entrySize;
    pEntry->FileInfo.FileAttributes = ctx->FindData.dwFileAttributes;
    pEntry->FileInfo.FileSize = ((UINT64)ctx->FindData.nFileSizeHigh << 32) |
                                ctx->FindData.nFileSizeLow;
    pEntry->FileInfo.AllocationSize =
        (pEntry->FileInfo.FileSize + 4095) & ~4095;
    pEntry->FileInfo.CreationTime =
        FileTimeToInt64(ctx->FindData.ftCreationTime);
    pEntry->FileInfo.LastAccessTime =
        FileTimeToInt64(ctx->FindData.ftLastAccessTime);
    pEntry->FileInfo.LastWriteTime =
        FileTimeToInt64(ctx->FindData.ftLastWriteTime);
    pEntry->FileInfo.ChangeTime = pEntry->FileInfo.LastWriteTime;
    pEntry->FileInfo.IndexNumber = 0;
    pEntry->FileInfo.HardLinks = 1;
    memcpy(pEntry->FileNameBuf, ctx->FindData.cFileName, nameSize);

    memcpy((BYTE *)Buffer + *PBytesTransferred, pEntry, entrySize);
    *PBytesTransferred += entrySize;

    if (!FindNextFileW(ctx->FindHandle, &ctx->FindData))
      return STATUS_SUCCESS;
  }
  return STATUS_SUCCESS;
}
