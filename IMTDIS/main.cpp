// #include <Afxwin.h>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <numeric>
#include <thread>
#include <iostream>
#include <fstream>
#include <deque>

class CMainWindow : public CFrameWnd
{
    int pixel_per_frame;
    CClientDC dc;
    /* IMT */;
    Node *pnode = NULL;
    DWORD dwPreviousTime;
    HINSTANCE hinstLib;
    HHOOK hhook;
    UINT uPeriod;

public:
    CMainWindow(/* address to IMT data */)
        : dc(this)
    {
        dc.SeletObject(BLACK_PEN);
        dc.SeletObject(BLACK_BRUSH);
        if (timeBeginPeriod(uPeriod) == TIMERR_NOCANDO)
        {
            std::cerr << "unable to load dll\n";
            PostMessage(WM_CLOSE, 0, 0);
        }
        /* load IMT data into IMT */;
        Create(NULL, _T ("Recorder"));

        hinstLib = LoadLibrary(TEXT("path to dll"));
        if (!hinstLib)
        {
            printf("unable to load dll\n");
            PostMessage(WM_CLOSE, 0, 0);
        }
        MYPROC ProcAdd = (MYPROC)GetProcAddress(hinstLib, "function name");
        if (!ProcAdd)
        {
            printf("unable to get function pointer\n");
            PostMessage(WM_CLOSE, 0, 0);
        }

        CString process_name(_T("process name"));
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
        {
            std::cerr << "fail to CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)\n";
            PostMessage(WM_CLOSE, 0, 0);
        }
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hProcessSnap, &pe32))
        {
            std::cerr << "Process32First\n"; // show cause of failure
            CloseHandle(hProcessSnap);          // clean the snapshot object
            PostMessage(WM_CLOSE, 0, 0);
        }
        DWORD dwProcessID = NULL;
        do
        {
            if (process_name.CompareNoCase(CString(pe32.szExeFile)))
            {
                dwProcessID = pe32.th32ProcessID;
                break;
            }

        } while (Process32Next(hProcessSnap, &pe32));
        CloseHandle(hProcessSnap);
        if (!dwProcessID)
        {
            std::cerr << "process not found\n";
            PostMessage(WM_CLOSE, 0, 0);
        }
        std::deque<DWORD> dwThreadIDs = ListProcessThreads(dwOwnerPID);
        hhook = ::SetWindowsHookEx(WH_KEYBOARD, ProcAdd, hinstLib, dwThreadIDs.front());        
    }

    ~CMainWindow()
    {
        timeEndPeriod(uPeriod);
        std::deque<DWORD> dwThreadIDs = ListProcessThreads(::GetCurrentProcessId());
        for (DWORD dwThreadID : dwThreadIDs)
        {
            HANDLE hThread = ::OpenThread(READ_CONTROL, FALSE, dwThreadID);
            ::WaitForSingleObject(hThread, INFINITE);
            ::CloseHandle(hThread);
        }
        UnhookWindowsHookEx(hhook);
        if (!FreeLibrary(hinstLib))
            std::cerr << "failed to free dll\n";
    }

protected:

    struct FAREC
    {
        int start;
        int end;
        DWORD dwPreviousTime;
    };

    void fall_rectangle(FAREC *pfarec)
    {
        CRect rect;
        GetClientRect(rect);
        CRect rect_draw(rect.left + rect.Width() * 2 / 5, rect.bottom - pfarec->end * pixel_per_frame, rect.left + rect.Width() * 3 / 5, rect.buttom - pfarec->start * pixel_per_frame);
        DWORD dwCurrentTime = timeGetTime();
        rect_draw.MoveToY((dwCurrentTime - pfarec->dwPreviousTime) / 1000 * 60 * pixel_per_frame);
        while (rect_draw.top < rect.Height())
        {
            dc.Rectangle(rect);
            pfarec->dwPreviousTime = dwCurrentTime;
            dwCurrentTime = timeGetTime();
            rect_draw.MoveToY((dwCurrentTime - pfarec->dwPreviousTime) / 1000 * 60 * pixel_per_frame);
            ::Sleep(25);
        }
        delete pfarec;
    }

    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        if (pnode && 0 /* *pnode does not have child */)
            pnode = NULL;
        if (!pnode)
            if (nChar == 0 /* the first input */)
            {
                pnode = /* pointer to the first node */;
                dwPreviousTime = timeGetTime();
                FAREC *pfarec = new FAREC;
                pfarec->dwPreviousTime = dwPreviousTime;
                /* initialize *pfarec by the range of *pnode */;
                CWinThread *pThread = AfxBeginThread(fall_rectangle, pfarec);
            }
            else
            {
                if (nChar != /* the next input */)
                    pnode = NULL;
                else
                {
                    DWORD dwCurrentTime = timeGetTime();
                    if (dwCurrentTime - dwPreviousTime /* does not in the range of *pnode */)
                        pnode = NULL;
                    else
                    {
                        pnode = /* pointer to the child node determined by */ dwCurrentTime - dwPreviousTime;
                        dwPreviousTime = dwCurrentTime;
                        FAREC *pfarec = new FAREC;
                        pfarec->dwPreviousTime = dwPreviousTime;
                        /* initialize *pfarec by the range of *pnode */;
                        CWinThread *pThread = AfxBeginThread(fall_rectangle, pfarec);
                    }
                }
            }
    }

    std::deque<DWORD> ListProcessThreads(DWORD dwOwnerPID)
    {
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap == INVALID_HANDLE_VALUE)
        {
            std::cerr << "fail to CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)\n";
            exit(EXIT_FAILURE);
        }
        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        if (!Thread32First(hThreadSnap, &te32))
        {
            std::cerr << "Thread32First\n";
            CloseHandle(hThreadSnap);
            exit(EXIT_FAILURE);
        }
        std::deque<DWORD> dwThreadIDs;
        do
        {
            if (te32.th32OwnerProcessID == dwOwnerPID)
                dwThreadIDs.push_back(te32.th32ThreadID);
        } while (Thread32Next(hThreadSnap, &te32));
        CloseHandle(hThreadSnap);
        return dwThreadIDs;
    }

    DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CMainWindow, CFrameWnd)
ON_WM_KEYDOWN()
END_MESSAGE_MAP()

class CMyApp : public CWinApp
{
public:
    virtual BOOL InitInstance()
    {
        m_pMainWnd = new CMainWindow;
        m_pMainWnd->ShowWindow(m_nCmdShow);
        m_pMainWnd->UpdateWindow(); // this send WM_PAINT to m_pMainWnd
        return TRUE;
    }
} myApp;

// The DLL entry-point function sets up shared memory using a named file-mapping object.

#ifdef __cplusplus // If used by C++ code,
extern "C"
{ // we need to export the C interface
#endif

    static LPVOID lpvMem = NULL; // pointer to shared memory

    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
    {
        HANDLE hMapObject; // handle to file mapping
        switch (fdwReason)
        {
        case DLL_PROCESS_ATTACH:
            hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HWMD), TEXT("dllmemfilemap"));
            if (!hMapObject)
                return FALSE;
            lpvMem = MapViewOfFile(hMapObject, FILE_MAP_WRITE, 0, 0, 0);
            if (!lpvMem)
                return FALSE;
            break;

        case DLL_PROCESS_DETACH:
            UnmapViewOfFile(lpvMem);
            CloseHandle(hMapObject);
            break;

        default:
            break;
        }
        return TRUE;
    }

    // SetSharedMem sets the contents of the shared memory

    __declspec(dllexport) VOID __cdecl SetSharedMem(HWMD hwmd)
    {
        *((HWMD *)lpvMem) = hwmd;
    }

    __declspec(dllexport) LRESULT CALLBACK __cdecl KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {

        if (nCode = WM_KEYDOWN)
            LRESULT lresult = PostMessage(*((HWMD *)lpvMem), nCode, wParam, lParam);
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

#ifdef __cplusplus
}
#endif