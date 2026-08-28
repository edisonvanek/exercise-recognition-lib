#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>

// Global variables for window management
static HWND g_hwnd = nullptr;
static cv::Mat g_frame;
static std::atomic<bool> g_running(true);
static std::mutex g_frame_mutex;
static int g_screen_width = 0;
static int g_screen_height = 0;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void CameraThread();
void RenderFrame();

// Get fullscreen resolution
void GetScreenResolution(int& width, int& height)
{
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}

// Window procedure for handling events
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            g_running = false;
            PostMessage(hwnd, WM_QUIT, 0, 0);
        }
        return 0;

    case WM_PAINT:
        RenderFrame();
        return 0;

    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

// Render frame from camera
void RenderFrame()
{
    if (g_hwnd == nullptr)
        return;

    HDC hdc = GetDC(g_hwnd);
    if (hdc == nullptr)
        return;

    {
        std::lock_guard<std::mutex> lock(g_frame_mutex);
        if (g_frame.empty())
        {
            ReleaseDC(g_hwnd, hdc);
            return;
        }

        // Convert OpenCV Mat to BGR format (OpenCV uses BGR by default)
        cv::Mat bgr_frame;
        if (g_frame.channels() == 4)
        {
            cv::cvtColor(g_frame, bgr_frame, cv::COLOR_BGRA2BGR);
        }
        else if (g_frame.channels() == 3)
        {
            bgr_frame = g_frame.clone();
        }
        else
        {
            cv::cvtColor(g_frame, bgr_frame, cv::COLOR_GRAY2BGR);
        }

        // Resize frame to fullscreen
        cv::Mat resized_frame;
        cv::resize(bgr_frame, resized_frame, cv::Size(g_screen_width, g_screen_height));

        // Convert BGR to RGB and then to DIB format for Windows
        cv::Mat rgb_frame;
        cv::cvtColor(resized_frame, rgb_frame, cv::COLOR_BGR2RGB);

        // Create and display DIB
        int num_channels = rgb_frame.channels();
        int bytes_per_pixel = num_channels;
        int width_bytes = rgb_frame.cols * bytes_per_pixel;

        // Create compatible DC and bitmap
        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP hbitmap = CreateCompatibleBitmap(hdc, rgb_frame.cols, rgb_frame.rows);
        HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, hbitmap);

        // Copy frame data to bitmap
        for (int y = 0; y < rgb_frame.rows; ++y)
        {
            SetDIBits(
                mem_dc,
                hbitmap,
                y,
                1,
                rgb_frame.ptr<uchar>(y),
                nullptr,
                DIB_RGB_COLORS
            );
        }

        // Blit to screen
        BitBlt(hdc, 0, 0, g_screen_width, g_screen_height, mem_dc, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(mem_dc, old_bitmap);
        DeleteObject(hbitmap);
        DeleteDC(mem_dc);
    }

    ReleaseDC(g_hwnd, hdc);
}

// Camera capture thread
void CameraThread()
{
    cv::VideoCapture camera(0);

    if (!camera.isOpened())
    {
        std::cerr << "Error: Cannot open camera!" << std::endl;
        g_running = false;
        return;
    }

    // Set camera properties
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    camera.set(cv::CAP_PROP_FPS, 30);

    std::cout << "Camera opened successfully. Press ESC to exit." << std::endl;

    cv::Mat frame;
    while (g_running)
    {
        if (camera.read(frame))
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            g_frame = frame.clone();
            
            // Trigger window update
            if (g_hwnd != nullptr)
            {
                InvalidateRect(g_hwnd, nullptr, FALSE);
            }
        }
        else
        {
            std::cerr << "Error: Cannot read frame from camera!" << std::endl;
            break;
        }

        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    camera.release();
    std::cout << "Camera closed." << std::endl;
}

// Main application
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
    // Get screen resolution
    GetScreenResolution(g_screen_width, g_screen_height);
    std::cout << "Screen resolution: " << g_screen_width << "x" << g_screen_height << std::endl;

    // Register window class
    const wchar_t CLASS_NAME[] = L"ExerciseRecognitionWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Create fullscreen window
    g_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Exercise Recognition - Camera Feed",
        WS_POPUP,  // Fullscreen borderless window
        0, 0,
        g_screen_width, g_screen_height,
        nullptr,
        nullptr,
        hinstance,
        nullptr
    );

    if (g_hwnd == nullptr)
    {
        std::cerr << "Error: Failed to create window!" << std::endl;
        return 1;
    }

    // Show window
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    // Start camera thread
    std::thread camera_thread(CameraThread);

    // Message loop
    MSG msg = {};
    while (g_running && GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Wait for camera thread
    camera_thread.join();

    // Cleanup
    DestroyWindow(g_hwnd);
    UnregisterClass(CLASS_NAME, hinstance);

    std::cout << "Application closed." << std::endl;
    return 0;
}
