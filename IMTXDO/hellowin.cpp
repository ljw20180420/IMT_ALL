#include <windows.h>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <numeric>
#include <thread>
#include "header/Extract.h"

LPCTSTR lpApplicationName = TEXT("\"path to app\""), lpCurrentDirectory = TEXT("\"running directory\"");
std::thread subthread;
std::mutex mtx;
static int HWND_DRAW = 0, HWND_SWITCH = 1;
HWND hwnds[2], hwnd_target;
static int SKILL = 0, DURATION = 1, DURATION_LAST = 2, DMAMGE = 3, HIT = 4;
HDC hdcs[5], hdc_target;
HBITMAP hbitmaps[5];
std::array<double, 4> positions[5];                    // x, y, cx, cy
std::vector<std::array<double, 2>> digit_positions[5]; // x, cx

template <class T>
struct Matrix
{
    std::vector<T> data;
    int cx;

    Matrix();

    Matrix(HBITMAP hBitmap)
    {
        BITMAP bitmap;
        GetObject(hBitmap, sizeof(BITMAP), &bitmap);
        char *bytes = new char[bitmap.bmHeight * bitmap.bmWidthBytes];
        GetBitmapBits(hBitmap, bitmap.bmHeight * bitmap.bmWidthBytes, bytes);
        for (int i = 0; i < bitmap.bmHeight; ++i)
            for (int j = 0; j < bitmap.bmWidth; ++j)
            {
                data.push_back(0);
                for (int k = 0; k < bitmap.bmBitsPixel; ++k)
                {
                    l = i * bitmap.bmWidthBytes * 8 + j * bitmap.bmBitsPixel + k;
                    if (bytes[l / 8] & 128 >> (l % 8))
                        data.back() += 1 << k;
                }
            }
        cx = bitmap.bmWidth;
        delete[] bytes;
    }

    void stretch(int lcm_cx, int lcm_cy)
    {
        int cy = data.size() / cx;
        if (lcm_cx == cx && lcm_cy == cy)
            return;
        int sx = lcm_cx / cx, sy = lcm_cy / cy;
        std::vector<T> data_stretch(lcm_cx * lcm_cy);
        for (int i = 0; i < cy; ++i)
            for (int j = 0; j < cx; ++j)
                for (int l = 0; l < sy; ++l)
                    for (int k = (i + l) * lcm_cx * sy + j * sx; k < (i + l) * lcm_cx * sy + (j + 1) * sx; ++k)
                        data_stretch[k] = data[i * cx + j];
        data.swap(data_stretch);
        cx = lcm_cx;
    }

    double compare(Matrix<T> &matrix, T thres)
    {
        if (cx != matrix.cx || data.size() != matrix.data.size())
        {
            std::cerr << "bitmap has inconsistent size\n";
            exit(-1);
        }
        uint64_t same_num;
        uint64_t total_num;
        for (int i = 0; i < data.size(); ++i)
            if (data[i] > 0)
            {
                ++total_num;
                if (data[i] <= matrix.data[i] && matrix.data[i] - data[i] <= thres || data[i] > matrix.data[i] && data[i] - matrix.data[i] <= thres)
                    ++same_num;
            }
        return double(same_num) / total_num;
    }

    Matrix<T> submatrix(int x_start, int x_end, int y_start, int y_end)
    {
        Matrix<T> sub_matrix;
        for (int i = y_start; i < y_end; ++i)
            for (int j = x_start; j < x_end; ++j)
                sub_matrix.data.push_back(data[i * cx + j]);
        sub_matrix.cx = x_end - x_start;
        return sub_matrix;
    }

    void swap(Matrix<T> &matrix)
    {
        data.swap(matrix.data);
        std::swap(cx, matrix.cx);
    }
};

int find(std::vector<MatUp> &matups, Matrix<T> factor_matrix, double percentage_thres, double thres)
{
    double max_same_percentage = -1;
    int max_idx;
    for (int i = 0; i < matups.size(); ++i)
    {
        int lcm_cx = std::lcm(matups[i].expand.cx, factor_matrix.cx);
        int lcm_cy = std::lcm(matups[i].expand.data.size() / matups[i].expand.cx, factor_matrix.data.size() / factor_matrix.cx);
        matups[i].expand.stretch(lcm_cx, lcm_cy);
        factor_matrix.stretch(lcm_cx, lcm_cy);
        double same_percentage = matups[i].expand.compare(factor_matrix, thres);
        if (same_percentage > max_same_percentage)
        {
            max_same_percentage = same_percentage;
            max_idx = i;
        }
    }
    if (max_same_percentage < percentage_thres)
        return -1;
    return max_idx;
}

std::vector<Matrix<T>> extract_factor_matrices(std::initializer_list<int> idxs);

template <class T>
struct CanonicalMatrices
{
    static int DURATION_DIGIT = 0;
    static int DIRECTION = 1;
    static int BUTTON_A = 2;
    static int BUTTON_B = 3;
    static int BUTTON_C = 4;
    static int BUTTON_D = 5;
    static int DAMAGE_DIGIT = 6;
    static int HIT_DIGIT = 7;
    struct MatUp
    {
        Matrix<T> matrix;
        Matrix<T> expand;
        bool updated = false;

        MatUp(Matrix<T> matrix_)
            : matrix(matrix_), expand(matrix_)
        {
        }
    };
    std::array<std::vector<MatUp>, 8> matupss;
    bool updated = false;

    CanonicalMatrix()
    {
        matupss[DURATION_DIGIT] = load_bitmaps("DU", 10, hInstance);
        matupss[DIRECTION] = load_bitmaps("DI", 8, hInstance);
        matupss[BUTTON_A] = load_bitmaps("BA", 2, hInstance);
        matupss[BUTTON_B] = load_bitmaps("BB", 2, hInstance);
        matupss[BUTTON_C] = load_bitmaps("BC", 2, hInstance);
        matupss[BUTTON_D] = load_bitmaps("BD", 2, hInstance);
        matupss[DAMAGE_DIGIT] = load_bitmaps("DA", 10, hInstance);
        matupss[HIT_DIGIT] = load_bitmaps("HI", 10, hInstance);
    }

    std::vector<MatUp> load_bitmaps(std::string name, int num, HINSTANCE hInstance)
    {
        std::vector<MatUp> matups;
        for (int i = 0; i < num; ++i)
        {
            HBITMAP hBitmap = LoadBitmap(hInstance, TEXT(("bitmaps/" + name + std::itos(i)).c_str()));
            matups.emplace_back(Matrix<T>(hBitmap));
            DeleteObject(hBitmap);
        }
        return matups;
    }

    void update(std::vector<MatUp> &matups, Matrix<T> &factor_matrix, double percentage_thres, double thres)
    {
        int max_idx = find(matups, factor_matrix, percentage_thres, thres);
        if (max_idx >= 0 && !matups[max_idx].updated)
        {
            matups[max_idx].matrix.swap(factor_matrix);
            matups[max_idx].updated = true;
        }
    }

    bool update(double percentage_thres, double thres)
    {
        std::vector<Matrix<T>> factor_matrices = extract_factor_matrices({SKILL});
        update(matupss[DIRECTION], factor_matrices[0], percentage_thres, thres);
        update(matupss[BUTTON_A], factor_matrices[1], percentage_thres, thres);
        update(matupss[BUTTON_B], factor_matrices[2], percentage_thres, thres);
        update(matupss[BUTTON_C], factor_matrices[3], percentage_thres, thres);
        update(matupss[BUTTON_D], factor_matrices[4], percentage_thres, thres);
        factor_matrices = extract_factor_matrices({DURATION});
        for (int i = 5; i < 8; ++i)
            update(matupss[DURATION_DIGIT], factor_matrices[i], percentage_thres, thres);
        factor_matrices = extract_factor_matrices({DAMAGE});
        for (int i = 8; i < 13; ++i)
            update(matupss[DAMAGE_DIGIT], factor_matrices[i], percentage_thres, thres);
        factor_matrices = extract_factor_matrices({HIT});
        for (int i = 13; i < 16; ++i)
            update(matupss[HIT_DIGIT], factor_matrices[i], percentage_thres, thres);
        for (auto &matups : matupss)
            for (auto &matup : matups)
                if (!matup.updated)
                    return false;

        for (auto &matups : matupss)
            for (auto &matup : matups)
            {
                matup.matrix.swap(matup.expand);
                matup.matrix.data.clear();
            }
        return true;
    }
};
CanonicalMatrices<uint64_t> canonicalmatrices;

bool CALLBACK SendWMCloseMsg(HWND hwnd, LPARAM lParam);
void clear_process(PROCESS_INFORMATION *lppiProcessInfo, int time_out)
bool startup_process(LPCTSTR lpApplicationName, LPCTSTR lpCurrentDirectory);

HWND find_main_window(DWORD dwProcessId);
bool CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);
bool is_main_window(HWND handle);

void timer_check(double percentage_thres, double thres);
Show bitmap_identify(std::vector<Matrix<T>> &factor_matrices, double percentage_thres, double thres));
int digits2val(std::intializer_list<bool> digits);
Button bools2button(std::intializer_list<bool> bools);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc_plate;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = TEXT("plate");

    if (!RegisterClass(wndclass))
    {
        MessageBox(NULL, TEXT("This program requires Windows NT!"), szAppName, MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindow(wndclass.lpszClassName, TEXT("Collector"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    PROCESS_INFORMATION piProcessInfo;
    startup_process(lpApplicationName, lpCurrentDirectory, &piProcessInfo);
    hwnd_target = find_main_window(piProcessInfo.dwProcessId);
    hdc_target = GetDC(hwnd_target);
    RECT rect;
    GetWindowRect(hwnd_target, rect);
    int x_resolution = desktop.right - desktop.left;
    int y_resoluton = desktop.bottom - desktop.top;

    HDC hdc_screen = GetDC(NULL);
    for (int i = 0; i < 5; ++i)
    {
        hdcs[i] = CreateCompatibleDC(NULL);
        hbitmaps[i] = CreateCompatibleBitmap(hdc_screen, x_resolution * positions[i][2], y_resolution * positions[i][3]);
        SelectObject(hdcs[i], hbitmaps[i]);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    void clear_process(&piProcessInfo, 5000);
    for (int i = 0; i < 5; ++i)
    {
        DeleteObject(hbitmaps[i]);
        DeleteDC(hdcs[i]);
    }
    ReleaseDc(NULL, hdc_screen);
    ReleaseDc(NULL, hdc_target);

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        static int cxChar = LOWORD(GetDialogBaseUnits());
        static int cyChar = HIWORD(GetDialogBaseUnits());
        hwnds[HWND_DRAW] = CreateWindow(TEXT("button"), TEXT("draw"), WS_CHILD | WS_VISIBLE | BS_CHECKBOX, cxChar, cyChar, 7 * cxChar, 7 * cyChar / 4, hwnd, (HMENU)0, ((LPCREATESTRUCT)lParam)−> hInstance, NULL);
        hwnds[HWND_SWITCH] = CreateWindow(TEXT("button"), TEXT("open"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, cxChar, 3 * cyChar, 7 * cxChar, 7 * cyChar / 4, hwnd, (HMENU)1, ((LPCREATESTRUCT)lParam)−> hInstance, NULL);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        {
            if (!SendMessage(hwnds[HWND_SWITCH], BM_GETSTATE, 0, 0))
            {
                SendMessage(hwnds[HWND_SWITCH], BM_SETSTATE, 1, 0);
                SetWindowText(hwnds[HWND_SWITCH], TEXT("close"));
                subthread = std::thread(std::bind(timer_check, hwnd, percentage_thres, thres));
            }
            else
            {
                mtx.lock();
                SendMessage(hwnds[HWND_SWITCH], BM_SETSTATE, 0, 0);
                mtx.unlock();
                subthread.join();
                SetWindowText(hwnds[HWND_SWITCH], TEXT("open"));
            }
        }
        return 0;
    case WM_DESTROY:
        if (SendMessage(hwnds[HWND_SWITCH], BM_GETSTATE, 0, 0))
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(GetWindowLong(hwnds[HWND_SWITCH], GWL_ID), BN_CLICKED), (LPARAM)hwnds[HWND_SWITCH]);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

std::vector<Matrix<T>> extract_factor_matrices(std::initializer_list<int> idxs)
{
    std::vector<Matrix<T>> factor_matrices;
    for (auto idx : idxs)
    {
        BitBlt(hdcs[idx], 0, 0, positions[idx][0], positions[idx][1], hdcs[TARGET], positions[idx][2], positions[idx][3], SRCCOPY);
        Matrix<uint64_t> matrix(hbitmaps[idx]);
        for (int j = 0; j < digit_positions[idx].size(); ++j)
        {
            double x = digit_positions[idx][j][0], cx = digit_positions[idx][j][1];
            factor_matrices.push_back(matrix.submatrix(round(matrix.cx * x), round(matrix.cx * (x + cx)), 0, matrix.data.size() / matrix.cx));
        }
    }
    return factor_matrices;
}

void timer_check(HWND hwnd, double percentage_thres, double thres)
{
    LASTINPUTINFO lii;
    bool file_exists = true;
    std::string data;
    for (int i = 0; file_exists; ++i)
    {
        data = "data" + std::itos(i);
        std::ifstream fin(data);
        file_exists = fin.is_open();
    }
    std::ofstream fout(data);
    Show show_last{0, 0, 0, 0, 0};
    LARGE_INTEGER Frequency, PerformanceCountTic, PerformanceCountToc;
    if (!GetLastInputInfo(&lii) || !QueryPerformanceFrequency(&Frequency))
    {
        DestroyWindow(hwnd);
        return;
    }
    QueryPerformanceCounter(&PerformanceCountTic);
    DWORD dwTime = lii.dwTime;
    while (true)
    {
        mtx.lock();
        if (!SendMessage(switchButton, BM_GETSTATE, 0, 0))
            break;
        mtx.unlock();
        do
        {
            GetLastInputInfo(&lii);
            QueryPerformanceCounter(&PerformanceCountToc);
        } while (dwTime == lii.dwTime || PerformanceCountToc - PerformanceCountTic < Frequency/100);
        dwTime = lii.dwTime;
        PerformanceCountTic = PerformanceCountToc;
        if (!SendMessage(drawButton, BM_GETCHECK, 0, 0))
        {
            canonicalmatrices.update(percentage_thres, thres);
            if (canonicalmatrices.updated)
                SendMessage(drawButton, BM_SETCHECK, 1, 0);
        }
        else
        {
            Show show = bitmap_identify({0, 3, 4}, percentage_thres, thres);
            if (show.direction != show_last.direction || show.button != show_last.button)
                fout << show_last.direction << '\t' << show_last.button << '\t' << bitmap_identify({2}, percentage_thres, thres).duration << '\t' << -1 << '\t' << -1 << '\n';
            if (show.Damage > 0 && show.Damage != show_last.Damage || show.Hit > 0 && show.Hit != show_last.Hit)
                fout << show.direction << '\t' << show.button << '\t' << bitmap_identify({1}, percentage_thres, thres).duration << '\t' << show.damage << '\t' << show.hit << '\n';
        }
    }
}

Show bitmap_identify(std::initializer_list<int> idxs, double percentage_thres, double thres)
{
    Show show;
    for (auto idx : idxs)
    {
        switch (idx)
        {
        case SKILL:
            std::vector<Matrix<T>> factor_matrices = extract_factor_matrices({SKILL});
            show.direction = find(canonicalmatrices.matupss[CanonicalMatrices::DIRECTION], factor_matrices[0], percentage_thres, thres) + 1;
            bool bool_A = find(canonicalmatrices.matupss[CanonicalMatrices::BUTTON_A], factor_matrices[1], percentage_thres, thres);
            bool bool_B = find(canonicalmatrices.matupss[CanonicalMatrices::BUTTON_B], factor_matrices[2], percentage_thres, thres);
            bool bool_C = find(canonicalmatrices.matupss[CanonicalMatrices::BUTTON_C], factor_matrices[3], percentage_thres, thres);
            bool bool_D = find(canonicalmatrices.matupss[CanonicalMatrices::BUTTON_D], factor_matrices[4], percentage_thres, thres);
            show.button = bools2button({bool_A, bool_B, bool_C, bool_D});
            continue;
        case DURATION:
        case DURATION_LAST:
            std::vector<Matrix<T>> factor_matrices = extract_factor_matrices({idx});
            std::vector<int> digits;
            for (auto &factor_maxtrix : factor_matrices)
                digits.push_back(find(canonicalmatrices.matupss[CanonicalMatrices::DURATION_DIGIT], factor_matrix, percentage_thres, thres));
            show.duration = digits2val(digits);
            continue;
        case DAMAGE
            std::vector<Matrix<T>> factor_matrices = extract_factor_matrices({DAMAGE});
            std::vector<int> digits;
            for (auto &factor_maxtrix : factor_matrices)
                digits.push_back(find(canonicalmatrices.matupss[CanonicalMatrices::DAMAGE_DIGIT], factor_matrix, percentage_thres, thres));
            show.damage = digits2val(digits);
            continue;
        case HIT
            std::vector<Matrix<T>> factor_matrices = extract_factor_matrices({HIT});
            std::vector<int> digits;
            for (auto &factor_maxtrix : factor_matrices)
                digits.push_back(find(canonicalmatrices.matupss[CanonicalMatrices::HIT_DIGIT], factor_matrix, percentage_thres, thres));
            show.hit = digits2val(digits);
        }
    }
    return show;
}

int digits2val(std::initializer_list<int> digits)
{
    int val = 0;
    for (int i = 0; i < digits.size(); i++)
        val = val * 10 + (digits[i] > 0 ? digits[i] : 0); // access all the arguments assigned to valist
    return val;
}

Button bools2button(std::initializer_list<bool> bools)
{
    int button = 0;
    for (auto bo : bools)
    {
        button *= 2;
        if (bo)
            ++button;
    }
    return button;
}

bool CALLBACK SendWMCloseMsg(HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwProcessId == lParam)
        SendMessageTimeout(hwnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 30000, NULL);
    return TRUE;
}

void clear_process(PROCESS_INFORMATION *lppiProcessInfo, int time_out)
{
    WaitForInputIdle(lppiProcessInfo->hProcess, INFINITE);

    EnumWindows(&SendWMCloseMsg, lppiProcessInfo->dwProcessId);
    if (WaitForSingleObject(lppiProcessInfo->hProcess, time_out) == WAIT_TIMEOUT)
        TerminateProcess(lppiProcessInfo->hProcess, 0);
    const DWORD result = WaitForSingleObject(lppiProcessInfo->hProcess, time_out);
    if (result != WAIT_OBJECT_0)
    {
        if (result == WAIT_TIMEOUT)
            // Timed out
        else
            //an error occurred
    }

    CloseHandle(lppiProcessInfo->hProcess);
    CloseHandle(lppiProcessInfo->hThread);
}

bool startup_process(LPCTSTR lpApplicationName, LPCTSTR lpCurrentDirectory, PROCESS_INFORMATION *lppiProcessInfo)
{
    STARTUPINFO siStartupInfo;
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
    memset(lppiProcessInfo, 0, sizeof(*lppiProcessInfo));

    bool started = CreateProcess(lpApplicationName, NULL, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, lpCurrentDirectorys, &siStartupInfo, lppiProcessInfo);

    if (started)
        WaitForInputIdle(piProcessInfo.hProcess, INFINITE);
    return started;
}

HWND find_main_window(DWORD dwProcessId)
{
    std::pair<DWORD, HWND> data{dwProcessId, 0};
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.second;
}

bool CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    std::pair<DWORD, HWND> *pdata = (std::pair<DWORD, HWND>*)lParam;
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(handle, &dwProcessId);
    if (pdata->first != dwProcessId || !is_main_window(handle))
        return TRUE;
    pdata->second = handle;
    return false;   
}

bool is_main_window(HWND handle)
{   
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}