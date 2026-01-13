#include "Downloader.h"
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "winhttp.lib")

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

// Helper: Format bytes to human readable string
static std::wstring FormatSize(ULONGLONG bytes) {
  const wchar_t *units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
  int i = 0;
  double size = (double)bytes;
  while (size >= 1024 && i < 4) {
    size /= 1024;
    i++;
  }
  wchar_t buf[64];
  swprintf_s(buf, 64, L"%.2f %s", size, units[i]);
  return buf;
}

bool Downloader::Download(const std::wstring &url,
                          const std::wstring &destination, bool debugMode) {
  // Check if file already exists and has size > 0
  WIN32_FILE_ATTRIBUTE_DATA fileData;
  if (GetFileAttributesExW(destination.c_str(), GetFileExInfoStandard,
                           &fileData)) {
    ULONGLONG fileSize =
        ((ULONGLONG)fileData.nFileSizeHigh << 32) | fileData.nFileSizeLow;
    if (fileSize > 0) {
      if (debugMode) {
        std::wcout << L"[Cache Hit] " << destination << L" ("
                   << FormatSize(fileSize) << L")" << std::endl;
      }
      return true;
    }
  }

  // Try curl first (shows progress), fallback to WinHTTP
  if (DownloadWithCurl(url, destination, debugMode)) {
    return true;
  }

  if (debugMode) {
    std::cout
        << "[DEBUG] curl failed or not available, falling back to WinHTTP..."
        << std::endl;
  }
  return DownloadWithWinHTTP(url, destination, debugMode);
}

bool Downloader::DownloadWithCurl(const std::wstring &url,
                                  const std::wstring &destination,
                                  bool debugMode) {
  std::string urlStr = WToStr(url);
  std::string destStr = WToStr(destination);

  if (debugMode) {
    std::wcout << L"Checking file size: " << url << std::endl;
  }

  // Build curl download command
  std::string cmd = "curl -L -# --fail --create-dirs -o \"" + destStr +
                    "\" \"" + urlStr + "\"";

  if (debugMode) {
    std::cout << "[DEBUG] Executing: " << cmd << std::endl;
  } else {
    std::wcout << L"Downloading: " << url << std::endl;
  }

  int result = system(cmd.c_str());

  if (result == 0) {
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesExW(destination.c_str(), GetFileExInfoStandard,
                             &fileData)) {
      ULONGLONG fileSize =
          ((ULONGLONG)fileData.nFileSizeHigh << 32) | fileData.nFileSizeLow;
      if (fileSize > 0) {
        std::wcout << L"Download complete: " << destination << L" ("
                   << FormatSize(fileSize) << L")" << std::endl;
        return true;
      }
    }
    DeleteFileW(destination.c_str());
  }

  return false;
}

bool Downloader::DownloadWithWinHTTP(const std::wstring &url,
                                     const std::wstring &destination,
                                     bool debugMode) {
  HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
  URL_COMPONENTS urlComp = {0};
  urlComp.dwStructSize = sizeof(urlComp);
  urlComp.dwHostNameLength = (DWORD)-1;
  urlComp.dwUrlPathLength = (DWORD)-1;

  if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp)) {
    return false;
  }

  std::wstring hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
  std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

  hSession = WinHttpOpen(L"MameCloudRom/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession)
    return false;

  hConnect = WinHttpConnect(hSession, hostname.c_str(), urlComp.nPort, 0);
  if (!hConnect) {
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD dwFlags =
      (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
  hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL,
                                WINHTTP_NO_REFERER,
                                WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
  if (!hRequest) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                          WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  if (!WinHttpReceiveResponse(hRequest, NULL)) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD dwStatusCode = 0;
  DWORD dwSize = sizeof(dwStatusCode);
  WinHttpQueryHeaders(
      hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
      WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, NULL);

  if (dwStatusCode != 200) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  std::ofstream outFile(destination, std::ios::binary);
  if (!outFile.is_open()) {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD dwDownloaded = 0;
  char buf[16384];
  while (WinHttpReadData(hRequest, buf, sizeof(buf), &dwDownloaded) &&
         dwDownloaded > 0) {
    outFile.write(buf, dwDownloaded);
  }

  outFile.close();
  std::wcout << L"Download complete: " << destination << std::endl;

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);
  return true;
}
