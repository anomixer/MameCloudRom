#include "MameFs.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

// Safe helper to convert string to wstring (ANSI)
static std::wstring StrToW(const std::string &str) {
  if (str.empty())
    return std::wstring();
  int size_needed =
      MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
  if (size_needed <= 0)
    return std::wstring();
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

// Helper to trim strings
std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (std::string::npos == first)
    return str;
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

void print_usage() {
  std::cout << "Usage: mcr -m <MountPoint> -c <CacheDir> -u <BaseUrl> [-zp "
               "<ZipPrefix>] [-7p <7zPrefix>] [-7z] [-d]"
            << std::endl;
  std::cout << "  -m   Mount point (e.g., Z:)" << std::endl;
  std::cout << "  -c   Cache directory (e.g., C:\\MameCache)" << std::endl;
  std::cout << "  -u   Base URL (e.g., <ROM_SOURCE_URL>)" << std::endl;
  std::cout << "  -zp  Zip folder path prefix (default: split/)" << std::endl;
  std::cout << "  -7p  7z folder path prefix (default: standalone/)"
            << std::endl;
  std::cout << "  -7z  Enable .7z file support (default: disabled)"
            << std::endl;
  std::cout << "  -d   Enable verbose debug logging (default: simple log)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Example Usage:" << std::endl;
  std::cout << "  mcr -m Z: -c C:\\MameCache -u <ROM_SOURCE_URL> -zp split/ "
               "-7p standalone/"
            << std::endl;
  std::cout << std::endl;
  std::cout << "[TIP] It is highly recommended to run 'config.bat' first to "
               "configure settings."
            << std::endl;
  std::cout << "      If no arguments are provided, MCR will load settings "
               "from mcr.ini"
            << std::endl;
}

int main(int argc, char *argv[]) {
  // ASCII Art Logo
  std::wcout
      << L"\n"
         L"   __  ___               _______             _____           \n"
         L"  /  |/  /__ ___ _  ___ / ___/ /__  __ _____/ / _ \\___  __ _ \n"
         L" / /|_/ / _ `/  ' \\/ -_) /__/ / _ \\/ // / _  / , _/ _ \\/  ' \\\n"
         L"/_/  "
         L"/_/\\_,_/_/_/_/\\__/\\___/_/\\___/\\_,_/\\_,_/_/|_|\\___/_/_/_/\n"
         L"                                                             \n"
         L"                                                  by anomixer\n"
         L"\n";

  std::cout << "MameCloudRom (MCR) v1.0" << std::endl;

  // Defines defaults
  std::wstring mountPoint = L"Z:";
  std::wstring cacheDir = L"C:\\MameCache";
  std::wstring baseUrl = L"";
  std::wstring zipPrefix = L"split/";
  std::wstring sevenZipPrefix = L"standalone/";
  bool enable7z = false;
  bool debugMode = false;

  if (argc == 1) {
    // Try to load from mcr.ini
    std::ifstream iniFile("mcr.ini");
    if (iniFile.is_open()) {
      std::cout << "[INFO] Loading settings from mcr.ini..." << std::endl;
      std::string line;
      while (std::getline(iniFile, line)) {
        std::stringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
          key = trim(key);
          value = trim(value);
          if (key == "MOUNT_POINT")
            mountPoint = StrToW(value);
          else if (key == "CACHE_DIR")
            cacheDir = StrToW(value);
          else if (key == "BASE_URL")
            baseUrl = StrToW(value);
          else if (key == "ZIP_PREFIX")
            zipPrefix = StrToW(value);
          else if (key == "SEVEN_ZIP_PREFIX")
            sevenZipPrefix = StrToW(value);
          else if (key == "ENABLE_7Z")
            enable7z = (value == "y" || value == "yes");
          else if (key == "DEBUG_MODE")
            debugMode = (value == "y" || value == "yes");
        }
      }
      iniFile.close();
    } else {
      print_usage();
      return 1;
    }
  } else {
    // Parse args
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-m" && i + 1 < argc) {
        mountPoint = StrToW(argv[++i]);
      } else if (arg == "-c" && i + 1 < argc) {
        cacheDir = StrToW(argv[++i]);
      } else if (arg == "-u" && i + 1 < argc) {
        baseUrl = StrToW(argv[++i]);
      } else if (arg == "-zp" && i + 1 < argc) {
        zipPrefix = StrToW(argv[++i]);
      } else if (arg == "-7p" && i + 1 < argc) {
        sevenZipPrefix = StrToW(argv[++i]);
      } else if (arg == "-7z") {
        enable7z = true;
      } else if (arg == "-d") {
        debugMode = true;
      } else if (arg == "/?" || arg == "-h" || arg == "--help") {
        print_usage();
        return 0;
      } else {
        print_usage();
        return 1;
      }
    }
  }

  std::wcout << L"Service configuration:" << std::endl;
  std::wcout << L"  Mount Point: " << mountPoint << std::endl;
  std::wcout << L"  Cache Dir:   " << cacheDir << std::endl;
  std::wcout << L"  Base URL:    " << baseUrl << std::endl;
  std::wcout << L"  Zip Prefix:  " << zipPrefix << std::endl;
  std::wcout << L"  7z Prefix:   " << sevenZipPrefix << std::endl;
  std::wcout << L"  7z Support:  " << (enable7z ? L"Enabled" : L"Disabled")
             << std::endl;
  std::wcout << L"  Debug Mode:  " << (debugMode ? L"Enabled" : L"Disabled")
             << std::endl;
  std::wcout << std::endl;

  return MameFs::Run(mountPoint, cacheDir, baseUrl, zipPrefix, sevenZipPrefix,
                     enable7z, debugMode);
}
