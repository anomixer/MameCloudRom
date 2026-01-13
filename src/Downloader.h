#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>

class Downloader {
public:
  // Download using curl (shows progress) or fallback to WinHTTP
  static bool Download(const std::wstring &url, const std::wstring &destination,
                       bool debugMode = false);

private:
  // curl-based download with progress
  static bool DownloadWithCurl(const std::wstring &url,
                               const std::wstring &destination, bool debugMode);

  // WinHTTP fallback
  static bool DownloadWithWinHTTP(const std::wstring &url,
                                  const std::wstring &destination,
                                  bool debugMode);

  static std::wstring GetHostname(const std::wstring &url);
  static std::wstring GetPath(const std::wstring &url);
};
