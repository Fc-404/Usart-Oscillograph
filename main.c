#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <pthread.h>

HINSTANCE hInst;
HWND hMain;
HANDLE hCom;

#define WIN_W 1600
#define WIN_H 900
#define LEFT_BAR_W 100
#define BOTM_BAR_H 100
#define BOTM_BAR_Y (WIN_H - BOTM_BAR_H)
#define WAVE_W (WIN_W - LEFT_BAR_W)
#define WAVE_H (WIN_H - BOTM_BAR_H)

#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 255, 0)
#define BLUE RGB(0, 0, 255)
#define GREY RGB(0x55, 0x55, 0x55)
#define BLACK RGB(0, 0, 0)
#define WHITE RGB(255, 255, 255)
void drawPot(int x, int y, COLORREF color);
void drawLine(int x, int y, int xx, int yy, COLORREF color);
void drawText(int x, int y, LPCSTR text, COLORREF color);

static PAINTSTRUCT ps;
static HDC hdc = NULL;
static HBITMAP hbm = NULL;

static int dot_per_bit = 1;
static char text_buf[64];

#define COMx "COM3"
#define serialBufL 8
static char serialC;
static long serialTop = 0;
static long serialLen = WAVE_W;
static char serialBuf[WIN_W * 10][serialBufL];

enum WAVE_MODE
{
    WAVE_Pot,
    WAVE_Line
} wave_mode;

void drawMain()
{
    // layout

    // 网格
    int step_h = WAVE_H / 10;
    for (int i = -5, n = 0; i <= 5; ++i)
    {
        if (i != -5 && i != 5)
            drawLine(LEFT_BAR_W - 20, WAVE_H - n, WIN_W, WAVE_H - n, GREY);
        sprintf(text_buf, "%dv", i);
        drawText(30, WAVE_H - n, text_buf, BLUE);
        n += step_h;
    }
    for (int i = LEFT_BAR_W + 100;; i += 100)
    {
        if (i > WIN_W)
            break;
        drawLine(i, 0, i, WAVE_H, GREY);
    }
    // 分割线
    // drawLine(0, BOTM_BAR_Y + 50, WIN_W, BOTM_BAR_Y + 50, RED);
    drawLine(LEFT_BAR_W, WIN_H - BOTM_BAR_H, WIN_W, WIN_H - BOTM_BAR_H, RED);
    drawLine(LEFT_BAR_W, 0, LEFT_BAR_W, WIN_H - BOTM_BAR_H, RED);

    drawText(120, WIN_H - 46, serialBuf[serialTop], GREEN);
    sprintf(text_buf, "%d", dot_per_bit);
    drawText(WIN_W - 59, WIN_H - 46, text_buf, GREEN);
    drawText(20, WIN_H - 46, "current data: ", GREY);
    drawText(WIN_W - 109, WIN_H - 46, "dot/bit: ", GREY);
    drawText(400, WIN_H - 46, "time: ", GREY);
    sprintf(text_buf, "period * %d / block", 100 * dot_per_bit);
    drawText(450, WIN_H - 46, text_buf, BLUE);

    // printf("%d\n", serialTop);
    HPEN pen = CreatePen(PS_SOLID, 1, GREEN);
    HPEN oPen = SelectObject(hdc, pen);
    MoveToEx(hdc, LEFT_BAR_W, WAVE_H - (strtof(serialBuf[0], NULL) + 5) * step_h, NULL);
    for (int i = 1, n = 0; i <= WAVE_W; ++i)
    {
        for (int j = 0; j < dot_per_bit; ++j)
        {
            if (wave_mode == WAVE_Pot)
                drawPot(LEFT_BAR_W + i, WAVE_H - (strtof(serialBuf[n++], NULL) + 5) * step_h, GREEN);
            else if (wave_mode == WAVE_Line)
                LineTo(hdc, LEFT_BAR_W + i, WAVE_H - (strtof(serialBuf[n++], NULL) + 5) * step_h);
        }
    }
    SelectObject(hdc, oPen);
    DeleteObject(pen);
    drawLine(LEFT_BAR_W + (serialTop) / dot_per_bit, 0, LEFT_BAR_W + (serialTop) / dot_per_bit, WAVE_H, BLUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        HDC t = GetDC(hwnd);
        hdc = CreateCompatibleDC(t);
        hbm = CreateCompatibleBitmap(t, WIN_W, WIN_H);
        ReleaseDC(hwnd, t);
        break;
    }
    case WM_PAINT:
    {
        HDC t = BeginPaint(hwnd, &ps);
        hdc = CreateCompatibleDC(t);
        HBITMAP o_hbm = (HBITMAP)SelectObject(hdc, hbm);
        HBRUSH brush = CreateSolidBrush(BLACK);
        FillRect(hdc, &ps.rcPaint, brush);
        DeleteObject(brush);

        drawMain();

        BitBlt(t, 0, 0, WIN_W, WIN_H, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, o_hbm);
        DeleteDC(hdc);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_LEFT && dot_per_bit > 1)
        {
            --dot_per_bit;
            serialLen = dot_per_bit * WAVE_W;
        }
        else if (wParam == VK_RIGHT && dot_per_bit < 10)
        {
            ++dot_per_bit;
            serialLen = dot_per_bit * WAVE_W;
        }

        if (wParam == VK_SPACE)
        {
            if (wave_mode == WAVE_Pot)
                wave_mode = WAVE_Line;
            else
                wave_mode = WAVE_Pot;
        }
        break;
    }

    case WM_DESTROY:
        DeleteDC(hdc);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void drawPot(int x, int y, COLORREF color)
{
    SetPixel(hdc, x, y, color);
}
void drawLine(int x, int y, int xx, int yy, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oPen = SelectObject(hdc, pen);
    MoveToEx(hdc, x, y, NULL);
    LineTo(hdc, xx, yy);
    SelectObject(hdc, oPen);
    DeleteObject(pen);
}
void drawText(int x, int y, LPCSTR text, COLORREF color)
{
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, x, y, text, lstrlenA(text));
}

BOOL initSerialPort(LPCSTR portName)
{
    hCom = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, _T("Error opening serial port"), _T("Error"), MB_OK);
        return FALSE;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hCom, &dcbSerialParams))
    {
        MessageBox(NULL, _T("Error getting serial port state"), _T("Error"), MB_OK);
        return FALSE;
    }

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hCom, &dcbSerialParams))
    {
        MessageBox(NULL, _T("Error setting serial port state"), _T("Error"), MB_OK);
        return FALSE;
    }

    printf("Success to open serial port...\n");
    MessageBox(NULL, _T("Success to open serial port"), _T("Success"), MB_OK);
    return TRUE;
}

void *readSerialPort(void *args)
{
    static int bufLen = 0;
    while (1)
    {
        if (ReadFile(hCom, &serialC, 1, NULL, NULL))
        {
            if (serialC == '\n')
            {
                serialBuf[serialTop++][bufLen] = '\0';
                serialTop %= serialLen;
                bufLen = 0;
                // printf("%s\n", serialBuf);
                InvalidateRect(hMain, NULL, TRUE);
                continue;
            }
            if (bufLen < 0 || bufLen >= serialBufL)
            {
                bufLen = -1;
                continue;
            }
            serialBuf[serialTop][bufLen++] = serialC;
        }
    }
    return NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    printf("start...\n");

    const char CLASS_NAME[] = "Usart Oscillograph";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hMain = CreateWindowEx(
        0, CLASS_NAME, CLASS_NAME,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        100, 100, WIN_W, WIN_H,
        NULL, NULL, hInstance, NULL);

    if (hMain == NULL)
    {
        perror("Failed to create window!\n");
        return -1;
    }

    ShowWindow(hMain, nCmdShow);
    UpdateWindow(hMain);

    if (!initSerialPort(COMx))
    {
        perror("Failed to init serial port!\n");
        return -1;
    }
    pthread_t t_readSPort;
    if (pthread_create(&t_readSPort, NULL, readSerialPort, NULL) != 0)
    {
        perror("Failed to create thread!\n");
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hCom);

    return 0;
}