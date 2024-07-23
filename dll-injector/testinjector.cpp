#include <tchar.h>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <vector>
#include <string>
#include <Psapi.h>
#include "tchar.h"
void SetConsoleColor(WORD color) {//コンソール出力の色を変えるためのもの
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
void Open_Website(std::string Link) noexcept
{
    system(("start " + Link).c_str());
}
// Function prototypes
void waitForEnter();
DWORD GetProcessIdFromWindowClassName(const TCHAR* className);
bool InjectDLL(DWORD processId, const wchar_t* dllPath);
bool CheckDLLsExist(const std::vector<std::wstring>& dlls);
std::wstring GetDllFullPath(const wchar_t* dllName);

void waitForEnter() {
    std::cout << "Enter to close" << std::endl;
    std::cin.get(); // Wait for Enter key to close
}

DWORD GetProcessIdFromWindowClassName(const TCHAR* className) {
    HWND hwnd = FindWindow(className, NULL);
    if (!hwnd) {
        std::cerr << "Failed: [FW-404]" << std::endl; // Failed to find window with class name
        return 0;
    }
    else {
        std::cerr << "Success [FW-SC]" << std::endl; // Success 
    }

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    return processId;
}

bool InjectDLL(DWORD processId, const wchar_t* dllPath) {

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Failed: [PR-404]" << std::endl; // Failed to open process with ID
        return false;
    }
    else {
        std::cerr << "Success [PR-SC]" << std::endl; // Success open process with ID
    }

    size_t pathLength = wcslen(dllPath) + 1;
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, pathLength * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMemory) {
        std::cerr << "Failed: [MEM-alm404]" << std::endl; // Failed to allocate memory in remote process
        CloseHandle(hProcess);
        return false;
    }
    else {
        std::cerr << "Success [MEM-almSC]" << std::endl; // Success allocate memory in remote process
    }

    if (!WriteProcessMemory(hProcess, pRemoteMemory, dllPath, pathLength * sizeof(wchar_t), NULL)) {
        std::cerr << "Failed: [DL-404]" << std::endl; // Failed to write DLL path to remote process memory
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    else {
        std::cerr << "Success [DL-SC]" << std::endl; // Success write DLL path to remote process memory
    }

    HMODULE hKernel32 = GetModuleHandle(TEXT("Kernel32"));
    if (!hKernel32) {
        std::cerr << "Failed: [K32-404]" << std::endl; // Failed to get handle to Kernel32 module
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    else {
        std::cerr << "Success [K32-SC]" << std::endl; // Success get handle to Kernel32 module
    }

    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibrary) {
        std::cerr << "Failed: [KN-404]" << std::endl; // Failed to get address of LoadLibraryW
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    else {
        std::cerr << "Success [KN-SC]" << std::endl; // Success get address of LoadLibraryW
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteMemory, 0, NULL);
    if (!hThread) {
        std::cerr << "Failed: [THR-501]" << std::endl; // Failed to create remote thread in target process
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    else {
        std::cerr << "Success [THR-SC]" << std::endl; // Success create remote thread in target process
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    std::cerr << "Success: [DL-INTER-SC]" << std::endl; // Success inject
    return true;
}

// Function to get the full path of the DLL
std::wstring GetDllFullPath(const wchar_t* dllName) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    std::wstring exeDir = exePath;
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos + 1);
    }
    else {
        exeDir += L"\\";
    }

    return exeDir + dllName;
}

// Function to check if DLLs exist
bool CheckDLLsExist(const std::vector<std::wstring>& dlls) {
    for (const auto& dll : dlls) {
        std::wstring fullPath = GetDllFullPath(dll.c_str()); // Convert to const wchar_t* using c_str()
        if (GetFileAttributes(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::wcerr << "Missing [DL-MD]:" << dll << std::endl; // DLL not found
            return false;
        }
        else {
            std::wcerr << "Found [DL-MD]" << dll << std::endl; // Found DLL
        }
    }
    return true;
}

int main() {
    //コメントアウトの線で囲われた場所は消しても問題ないゾーンです。宣伝なので気にしないでください。削除してもよいですがクレジットとしてgithubサイトの掲載をしてくれるとありがたいです！
    // 
    // クレジットコピー用->
    // this tool is include Rar's dll-injector.
    // hes github page:https://rarcns.github.io/Rar-tools-page/
    // thx.
    // 
    // クレジットは書かなくても問題ないです。。。が悲しいので書いてくれるとありがたいです
    //----------------------------------------------------------------------------------------------------------------------------
    //class name getterを知っているか尋ねる(ここは削除しても問題ないです)
    std::string know;
    std::cout << "\n\n     do you know ";
    SetConsoleColor(11);//色を変える
    std::cout << "Rar's window classname getter";
    // 色をリセットする
    SetConsoleColor(7);
    std::cout << " ? yes or no:";
    std::cin >> know;
    if (know == "yes") {
        std::cout << "\n\n      thx for you know!\n\n" << std::endl;
    }
    else if (know == "no") {
        std::string go;
        std::cout << "\n\n     do you want to use ";
        SetConsoleColor(11);//色を変える
        std::cout << "Rar's window classname getter";
        // 色をリセットする
        SetConsoleColor(7);
        std::cout << " ? yes or no:";
        std::cin >> go;
        if(go == "yes") {
            Open_Website("https://rarcns.github.io/Rar-tools-page/");
        }else if (go == "no") {
            std::cout << "\n\n      you skipped get tool.\n\n" << std::endl;
        }
        else {
            std::cout << "\n\n     undefined option...";//optionが見つからなかったためSleep(5000);をした後にexit(1);する
            Sleep(5000);
            exit(1);
        }
    }
    else {
        std::cout << "\n\n     undefined option...";//optionが見つからなかったためSleep(5000);をした後にexit(1);する
        Sleep(5000);
        exit(1);
    }
    //----------------------------------------------------------------------------------------------------------------------------
    //class nameを事前に尋ねる
    std::wstring windowClassName;
    std::cout << "\n\n     please enter ";
    SetConsoleColor(11);//色を変える
    std::cout << "window classname ";
    // 色をリセットする
    SetConsoleColor(7);
    std::cout << "to get process kernel, pid, etc... :";//新式。class nameを事前に尋ねるようになっている
    std::wcin >> windowClassName;
    //windowClassNameをTCHARに変換
    const TCHAR* tcharWindowClassName = windowClassName.c_str();

    //dllnameを尋ねる
    std::wstring dllName;
    std::cout << "\n\n     please enter ";
    SetConsoleColor(11);//色を変える
    std::cout << "dll name ";
    // 色をリセットする
    SetConsoleColor(7);
    std::cout << "to inject dll : ";//新式。dll nameを事前に尋ねるようになっている
    std::wcin >> dllName;
    //dllNameをwchar_t*に変換
    const wchar_t* wchardllname = dllName.c_str();

    //const TCHAR* windowClassName = TEXT("");//旧式ではここにdllをインジェクトするウィンドウのクラス名を記入。クラス名に関してはすでに公開済みの window classname getterを使用してください。
    //新型では、ウィンドウが作成された後、cinでclass nameをユーザーに尋ねるようになっている。


    //const wchar_t* dllName = L"module.dll";//旧式ではここにdllの名前を記入していた。
    std::wstring dllPath = GetDllFullPath(wchardllname);
    std::vector<std::wstring> requiredDLLs = {//ここでdllの存在を確認している。旧式ではL"module.dll",のようにしていた。
        wchardllname,
    };

    if (!CheckDLLsExist(requiredDLLs)) {
        std::cerr << "\n\nFailed: [DL-RQ-404]" << std::endl; // Missing DLL
        waitForEnter();
        return 1;
    }

    DWORD processId = GetProcessIdFromWindowClassName(tcharWindowClassName);
    if (processId == 0) {
        std::cerr << "\n\nFailed: [PID-404]" << std::endl; // Failed to find process ID
        waitForEnter();
        return 1;
    }

    if (!InjectDLL(processId, dllPath.c_str())) {
        std::cerr << "\n\nFailed: [DLI-501]" << std::endl; // DLL injection failed
        waitForEnter();
        return 1;
    }

    std::cout << "\n\nSuccess: [LDN-SC]" << std::endl; // DLL injected successfully
    waitForEnter();
    return 0;
}
/*
エラーログに関して

Found [XX-XX]
のような場合、XX属のDDに関してのものが見つかったよっていうやつ

Missing [XX-DD]
のような場合、XX属のDDに関してのものが見つからなかったよっていうやつ

Failed: [XX-404]
のような404タイプが見つからなかった系(failed to handle等も)

Failed: [XX-501]
のような501タイプができなかった系

Success: [XX-SC]
のようなSCタイプが成功したとき

Success: [DL-INTER-SC]
インジェクト完了でしか使われない
*/