@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
:: --- Initialize Defaults ---
set "MAME_CACHE=C:\mamecache"
set "DRIVE_LETTER=Z:"
set "MAME_DIR=C:\mame"
set "MAME_EXE=mame.exe"
set "BASE_URL="
set "ZIP_PREFIX=split/"
set "SEVEN_ZIP_PREFIX=standalone/"
set "ENABLE_7Z=n"
set "DEBUG_MODE=n"

:: --- Load Existing Settings ---
if exist mcr.ini (
    for /f "tokens=1,2 delims==" %%a in (mcr.ini) do (
        if "%%a"=="CACHE_DIR" set "MAME_CACHE=%%b"
        if "%%a"=="MOUNT_POINT" set "DRIVE_LETTER=%%b"
        if "%%a"=="MAME_DIR" set "MAME_DIR=%%b"
        if "%%a"=="MAME_EXE" set "MAME_EXE=%%b"
        if "%%a"=="DEBUG_MODE" set "DEBUG_MODE=%%b"
        if "%%a"=="ENABLE_7Z" set "ENABLE_7Z=%%b"
        if "%%a"=="BASE_URL" set "BASE_URL=%%b"
        if "%%a"=="ZIP_PREFIX" set "ZIP_PREFIX=%%b"
        if "%%a"=="SEVEN_ZIP_PREFIX" set "SEVEN_ZIP_PREFIX=%%b"
    )
)

:: --- Language Detection ---
set "UI_LANG=EN"
for /f "tokens=3" %%a in ('reg query "HKEY_CURRENT_USER\Control Panel\International" /v "LocaleName" 2^>nul') do set "LOCALE_NAME=%%a"
if /i "!LOCALE_NAME:~0,5!"=="zh-TW" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-HK" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-MO" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-CN" set "UI_LANG=CN"
if /i "!LOCALE_NAME:~0,5!"=="zh-SG" set "UI_LANG=CN"

:: Set UI strings and CodePage
if "!UI_LANG!"=="TW" (
    set "STR_INFO_WINFSP_OK=[OK] WinFsp 已安裝。"
    set "STR_ERR_WINFSP_MISSING=[錯誤] 找不到 WinFsp！請從 https://winfsp.dev/ 安裝。"
    set "STR_INFO_LOADED=[資訊] 已從 mcr.ini 載入現有設定"
    set "STR_PROMPT_CACHE=請輸入 MameCache 目錄 (預設: !MAME_CACHE!): "
    set "STR_PROMPT_DRIVE=請輸入虛擬磁碟代號 (預設: !DRIVE_LETTER!): "
    set "STR_PROMPT_MAME_DIR=請輸入您的 MAME 目錄 (預設: !MAME_DIR!): "
    set "STR_PROMPT_MAME_EXE=請輸入 MAME 執行檔名稱 (預設: !MAME_EXE!): "
    set "STR_PROMPT_7Z=啟用 .7z 檔案支援? (y/n) (預設: !ENABLE_7Z!): "
    set "STR_PROMPT_DEBUG=啟用除錯模式? (y/n) (預設: !DEBUG_MODE!): "
    set "STR_URL_HINT=請輸入第三方 MAME 倉庫的 BASE_URL。"
    set "STR_URL_FORMAT=(格式範例: https://domain.com/path/)"
    set "STR_PROMPT_URL=網址 (目前: !BASE_URL!): "
    set "STR_ERR_URL_EMPTY=[錯誤] 網址不能為空！"
    set "STR_PROMPT_ZIP_PFX=輸入 .zip 資料夾路徑前綴 (預設: !ZIP_PREFIX!): "
    set "STR_PROMPT_7Z_PFX=輸入 .7z 資料夾路徑前綴 (預設: !SEVEN_ZIP_PREFIX!): "
    set "STR_INFO_VALIDATING=[資訊] 正在驗證網址: !BASE_URL!"
    set "STR_INFO_TESTING=[資訊] 嘗試獲取測試檔案 (使用前綴 !ZIP_PREFIX! + robby.zip)..."
    set "STR_ERR_VALIDATE=[錯誤] 網址驗證失敗！無法存取 !BASE_URL!!ZIP_PREFIX!robby.zip"
    set "STR_INFO_CHECK_CON=請檢查網址、前綴以及您的網路連線。"
    set "STR_OK_VALIDATE=[OK] 網址連線驗證成功。"
    set "STR_OK_SAVED=[OK] 設定已儲存至 mcr.ini"
    set "STR_INFO_EXE_MISSING=[資訊] 在根目錄找不到 mcr.exe。正在呼叫 build.bat..."
    set "STR_ERR_BUILD=[錯誤] 編譯失敗。請檢查上方錯誤訊息。"
    set "STR_LOG_MCR_RUN=[自動化] 正在新視窗啟動 MAME..."
    set "STR_SUCCESS_TITLE=[成功] 設定完成！"
    set "STR_SUCCESS_RUN=您現在可以使用 'mcr.exe' 執行 MCR，"
    set "STR_SUCCESS_RUN_BAT=或使用 'mcr-run.bat' 同時啟動 MCR + Mame"
) else if "!UI_LANG!"=="CN" (
    set "STR_INFO_WINFSP_OK=[OK] WinFsp 已安装。"
    set "STR_ERR_WINFSP_MISSING=[错误] 找不到 WinFsp！请从 https://winfsp.dev/ 安装。"
    set "STR_INFO_LOADED=[信息] 已從 mcr.ini 加载现有设置"
    set "STR_PROMPT_CACHE=请输入 MameCache 目录 (默认: !MAME_CACHE!): "
    set "STR_PROMPT_DRIVE=请输入虚拟盘符 (默认: !DRIVE_LETTER!): "
    set "STR_PROMPT_MAME_DIR=请输入您的 MAME 目录 (默认: !MAME_DIR!): "
    set "STR_PROMPT_MAME_EXE=请输入 MAME 可执行文件名 (默认: !MAME_EXE!): "
    set "STR_PROMPT_7Z=启用 .7z 文件支持? (y/n) (默认: !ENABLE_7Z!): "
    set "STR_PROMPT_DEBUG=启用调试模式? (y/n) (默认: !DEBUG_MODE!): "
    set "STR_URL_HINT=请输入第三方 MAME 仓库的 BASE_URL。"
    set "STR_URL_FORMAT=(格式示例: https://domain.com/path/)"
    set "STR_PROMPT_URL=网址 (当前: !BASE_URL!): "
    set "STR_ERR_URL_EMPTY=[错误] 网址不能为空！"
    set "STR_PROMPT_ZIP_PFX=输入 .zip 文件夹路径前缀 (默认: !ZIP_PREFIX!): "
    set "STR_PROMPT_7Z_PFX=输入 .7z 文件夹路径前缀 (默认: !SEVEN_ZIP_PREFIX!): "
    set "STR_INFO_VALIDATING=[信息] 正在验证网址: !BASE_URL!"
    set "STR_INFO_TESTING=[信息] 尝试获取测试文件 (使用前缀 !ZIP_PREFIX! + robby.zip)..."
    set "STR_ERR_VALIDATE=[错误] 网址验证失败！无法访问 !BASE_URL!!ZIP_PREFIX!robby.zip"
    set "STR_INFO_CHECK_CON=请检查网址、前缀以及您的网络连接。"
    set "STR_OK_VALIDATE=[OK] 网址连接验证成功。"
    set "STR_OK_SAVED=[OK] 设置已保存至 mcr.ini"
    set "STR_INFO_EXE_MISSING=[信息] 在根目录找不到 mcr.exe。正在调用 build.bat..."
    set "STR_ERR_BUILD=[错误] 编译失败。请检查上方错误信息。"
    set "STR_LOG_MCR_RUN=[自动化] 正在新窗口启动 MAME..."
    set "STR_SUCCESS_TITLE=[成功] 设置完成！"
    set "STR_SUCCESS_RUN=您现在可以使用 'mcr.exe' 运行 MCR，"
    set "STR_SUCCESS_RUN_BAT=或使用 'mcr-run.bat' 同时启动 MCR + Mame"
) else (
    :: Default English
    set "STR_INFO_WINFSP_OK=[OK] WinFsp is installed."
    set "STR_ERR_WINFSP_MISSING=[ERROR] WinFsp not found! Please install from https://winfsp.dev/"
    set "STR_INFO_LOADED=[INFO] Loaded existing settings from mcr.ini"
    set "STR_PROMPT_CACHE=Enter MameCache directory (Default: !MAME_CACHE!): "
    set "STR_PROMPT_DRIVE=Enter virtual drive letter (Default: !DRIVE_LETTER!): "
    set "STR_PROMPT_MAME_DIR=Enter your MAME directory (Default: !MAME_DIR!): "
    set "STR_PROMPT_MAME_EXE=Enter MAME executable name (Default: !MAME_EXE!): "
    set "STR_PROMPT_7Z=Enable .7z file support? (y/n) (Default: !ENABLE_7Z!): "
    set "STR_PROMPT_DEBUG=Enable Debug Mode? (y/n) (Default: !DEBUG_MODE!): "
    set "STR_URL_HINT=Please enter the 3rd party MAME Repo BASE_URL."
    set "STR_URL_FORMAT=(Format example: https://domain.com/path/)"
    set "STR_PROMPT_URL=URL (Current: !BASE_URL!): "
    set "STR_ERR_URL_EMPTY=[ERROR] URL cannot be empty!"
    set "STR_PROMPT_ZIP_PFX=Enter .zip folder path prefix (Default: !ZIP_PREFIX!): "
    set "STR_PROMPT_7Z_PFX=Enter .7z folder path prefix (Default: !SEVEN_ZIP_PREFIX!): "
    set "STR_INFO_VALIDATING=[INFO] Validating URL: !BASE_URL!"
    set "STR_INFO_TESTING=[INFO] Attempting to fetch a test file (using prefix !ZIP_PREFIX! + robby.zip)..."
    set "STR_ERR_VALIDATE=[ERROR] URL validation failed! Could not reach !BASE_URL!!ZIP_PREFIX!robby.zip"
    set "STR_INFO_CHECK_CON=Please check the URL, prefixes, and your internet connection."
    set "STR_OK_VALIDATE=[OK] URL connection verified successfully."
    set "STR_OK_SAVED=[OK] Settings saved to mcr.ini"
    set "STR_INFO_EXE_MISSING=[INFO] mcr.exe not found in root. Calling build.bat..."
    set "STR_ERR_BUILD=[ERROR] Build failed. Please check the errors above."
    set "STR_LOG_MCR_RUN=[AUTOMATION] Launching MAME in a new window..."
    set "STR_SUCCESS_TITLE=[SUCCESS] Configuration complete!"
    set "STR_SUCCESS_RUN=You can now run MCR using: 'mcr.exe',"
    set "STR_SUCCESS_RUN_BAT=Or run MCR + Mame simultaneously with 'mcr-run.bat'"
)

echo ==========================================
echo MameCloudRom (MCR) v1.0 Configuration
echo ==========================================

:: 1. Check for WinFsp
set "WINFSP_DLL=C:\Program Files (x86)\WinFsp\bin\winfsp-x64.dll"
if not exist "%WINFSP_DLL%" (
    echo !STR_ERR_WINFSP_MISSING!
    pause
    exit /b 1
)
echo !STR_INFO_WINFSP_OK!

echo.
set /p "MAME_CACHE=!STR_PROMPT_CACHE!"
set /p "DRIVE_LETTER=!STR_PROMPT_DRIVE!"
set /p "MAME_DIR=!STR_PROMPT_MAME_DIR!"
set /p "MAME_EXE=!STR_PROMPT_MAME_EXE!"
set /p "ENABLE_7Z=!STR_PROMPT_7Z!"
set /p "DEBUG_MODE=!STR_PROMPT_DEBUG!"

:ENTER_URL
echo.
echo !STR_URL_HINT!
echo !STR_URL_FORMAT!
set /p "USER_URL=!STR_PROMPT_URL!"
if "!USER_URL!"=="" (
    if "!BASE_URL!"=="" (
        echo !STR_ERR_URL_EMPTY!
        goto ENTER_URL
    )
) else (
    set "BASE_URL=!USER_URL!"
)

echo.
set /p "ZIP_PREFIX=!STR_PROMPT_ZIP_PFX!"
set /p "SEVEN_ZIP_PREFIX=!STR_PROMPT_7Z_PFX!"

echo !STR_INFO_VALIDATING!
echo !STR_INFO_TESTING!
:: Use curl -f (fail on error) and download to nul to verify
:: Concatenate BASE_URL + ZIP_PREFIX + robby.zip
curl -f -s -L -o nul "!BASE_URL!!ZIP_PREFIX!robby.zip"
if errorlevel 1 (
    echo !STR_ERR_VALIDATE!
    echo !STR_INFO_CHECK_CON!
    goto ENTER_URL
)
echo !STR_OK_VALIDATE!

:: Validate Drive Letter format
if not "!DRIVE_LETTER:~-1!"==":" set "DRIVE_LETTER=!DRIVE_LETTER!:"

:: Normalize Flags
if /i "!ENABLE_7Z!"=="y" (set "ENABLE_7Z=y") else (set "ENABLE_7Z=n")
if /i "!DEBUG_MODE!"=="y" (set "DEBUG_MODE=y") else (set "DEBUG_MODE=n")

:: 3. Write to mcr.ini
echo MOUNT_POINT=!DRIVE_LETTER!> mcr.ini
echo CACHE_DIR=!MAME_CACHE!>> mcr.ini
echo MAME_DIR=!MAME_DIR!>> mcr.ini
echo MAME_EXE=!MAME_EXE!>> mcr.ini
echo ENABLE_7Z=!ENABLE_7Z!>> mcr.ini
echo DEBUG_MODE=!DEBUG_MODE!>> mcr.ini
echo BASE_URL=!BASE_URL!>> mcr.ini
echo ZIP_PREFIX=!ZIP_PREFIX!>> mcr.ini
echo SEVEN_ZIP_PREFIX=!SEVEN_ZIP_PREFIX!>> mcr.ini

echo !STR_OK_SAVED!

:: 4. Check for mcr.exe in root
if not exist "mcr.exe" (
    echo !STR_INFO_EXE_MISSING!
    call build.bat
    if errorlevel 1 (
        echo !STR_ERR_BUILD!
        pause
        exit /b 1
    )
)

:: 5. Create mcr-run.bat directly with current values
set "MCR_EXE_RUN=mcr.exe"
set "FLAGS="
if /i "!ENABLE_7Z!"=="y" set "FLAGS=!FLAGS! -7z"
if /i "!DEBUG_MODE!"=="y" set "FLAGS=!FLAGS! -d"

(
echo @echo off
echo echo Executing: "!MCR_EXE_RUN!" -m !DRIVE_LETTER! -c "!MAME_CACHE!" -u !BASE_URL! -zp !ZIP_PREFIX! -7p !SEVEN_ZIP_PREFIX!!FLAGS!
echo echo !STR_LOG_MCR_RUN!
echo echo.
echo start "MAME-MCR" cmd /c "echo Waiting for MCR to mount !DRIVE_LETTER!... ^& timeout /t 3 ^>nul ^& cd /d "!MAME_DIR!" ^& !MAME_EXE! -rompath !DRIVE_LETTER!"
echo "!MCR_EXE_RUN!" -m !DRIVE_LETTER! -c "!MAME_CACHE!" -u !BASE_URL! -zp !ZIP_PREFIX! -7p !SEVEN_ZIP_PREFIX!!FLAGS!
) > mcr-run.bat

echo ====================================================
echo !STR_SUCCESS_TITLE!
echo !STR_SUCCESS_RUN!
echo !STR_SUCCESS_RUN_BAT!
echo ====================================================
pause
