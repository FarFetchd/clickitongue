#ifdef CLICKITONGUE_WINDOWS

#include "windows_gui.h"

#include "constants.h"
#include "interaction.h"

#include <windows.h>
#include <commctrl.h>
#include <unistd.h>
HWND g_printf_hwnd;
HWND g_banner_hwnd;
HWND g_main_hwnd;
HWND g_retrain_button;
HWND g_newmic_button;
HWND g_exit_button;
void makeSafeToExit();
LRESULT CALLBACK WindowProcedure(HWND theHwnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
  bool should_exit = false;
  if (message == WM_DESTROY || message == WM_CLOSE)
    should_exit = true;
  else if (message == WM_COMMAND)
  {
    if ((HWND)lParam == g_exit_button)
      should_exit = true;
    else if ((HWND)lParam == g_retrain_button)
    {
      if (promptYesNo("Do you want to completely redo Clickitongue's training?"))
      {
        unlink("default.clickitongue");
        promptInfo("Ok. Clickitongue will now close. Run it again, and you will "
                   "be prompted to train it anew.");
        should_exit = true;
      }
    }
    else if ((HWND)lParam == g_newmic_button)
    {
      if (promptYesNo("Do you want to choose a new audio input device?"))
      {
        unlink("audio_input_device.config");
        promptInfo("Ok. Clickitongue will now close. Run it again, and you will "
                   "be prompted to pick a new audio input device.");
        should_exit = true;
      }
    }
  }
  if (should_exit)
  {
    makeSafeToExit();
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(theHwnd, message, wParam, lParam);
}
void crash(const char* s);
void windowsGUI(HINSTANCE hInstance, int nCmdShow)
{
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  const char kMainClassname[] = "clickitongue_main_window";
  WNDCLASSEX win_class;
  win_class.hInstance     = hInstance;
  win_class.lpszClassName = kMainClassname;
  win_class.lpfnWndProc   = &WindowProcedure;
  win_class.cbSize        = sizeof(win_class);
  win_class.lpszMenuName = NULL;
  win_class.cbClsExtra = 0;
  win_class.cbWndExtra = 0;

  win_class.style = 0;
  win_class.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  win_class.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  win_class.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);

  // Register our window classes, or error.
  if (!RegisterClassEx(&win_class))
    crash("Failed to register window class, crashing");

  g_main_hwnd = CreateWindowEx(
      WS_EX_CLIENTEDGE,
      kMainClassname,
      "Clickitongue",      // title bar text
      WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX, // styles
      CW_USEDEFAULT,       // default x position
      CW_USEDEFAULT,       // default y position
      640,                 // width in pixels
      360,                 // height in pixels
      HWND_DESKTOP,        // parent window
      NULL,                // menu
      hInstance,           // instance handler? anyways, WinMain hInstance
      NULL); // some extra data thing we don't use
  if (!g_main_hwnd)
    crash("Failed to create main Clickitongue window, crashing");

  NONCLIENTMETRICS ncm;
  ncm.cbSize = sizeof(ncm);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  HFONT default_font = CreateFontIndirect(&ncm.lfMessageFont);
  HFONT mono_font = (HFONT)GetStockObject(ANSI_FIXED_FONT);

  g_printf_hwnd = CreateWindow(
      "STATIC",   // predefined class
      CLICKITONGUE_VERSION,   // text to start with
      WS_VISIBLE | WS_CHILD,  // styles
      2,         // starting x position
      2,         // starting y position
      634,        // width in pixels
      270,        // height in pixels
      g_main_hwnd,       // parent window
      NULL,       // menu
      hInstance,
      NULL);      // some extra data thing we don't use
  SendMessage(g_printf_hwnd, WM_SETFONT, (WPARAM)default_font, 0);

  g_banner_hwnd = CreateWindow(
      "STATIC",   // predefined class
      kRecordingBanner,   // text
      WS_CHILD,  // styles
      2,         // starting x position
      44,        // starting y position
      634,        // width in pixels
      230,        // height in pixels
      g_main_hwnd,       // parent window
      NULL,       // menu
      hInstance,
      NULL);      // some extra data thing we don't use
  SendMessage(g_banner_hwnd, WM_SETFONT, (WPARAM)mono_font, 0);
  ShowWindow(g_banner_hwnd, SW_HIDE);

  g_retrain_button = CreateWindow(
        "BUTTON",   // predefined class
        "Retrain Clickitongue",       // text
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // styles
        16,         // starting x position
        274,         // starting y position
        187,        // width
        50,        // height
        g_main_hwnd,       // parent window
        NULL,       // No menu
        hInstance,
        NULL);      // pointer not needed
  SendMessage(g_retrain_button, WM_SETFONT, (WPARAM)default_font, 0);

  g_newmic_button = CreateWindow(
        "BUTTON",   // predefined class
        "Choose new mic device",       // text
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // styles
        223,         // starting x position
        274,         // starting y position
        187,        // width
        50,        // height
        g_main_hwnd,       // parent window
        NULL,       // No menu
        hInstance,
        NULL);      // pointer not needed
  SendMessage(g_newmic_button, WM_SETFONT, (WPARAM)default_font, 0);

  g_exit_button = CreateWindow(
        "BUTTON",   // predefined class
        "Exit",       // text
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // styles
        430,         // starting x position
        274,         // starting y position
        187,        // width
        50,        // height
        g_main_hwnd,       // parent window
        NULL,       // No menu
        hInstance,
        NULL);      // pointer not needed
  SendMessage(g_exit_button, WM_SETFONT, (WPARAM)default_font, 0);

  ShowWindow(g_main_hwnd, nCmdShow);
  UpdateWindow(g_main_hwnd);

  MSG win_msg;
  while (GetMessage(&win_msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&win_msg);
    DispatchMessage(&win_msg);
  }
}

#endif // CLICKITONGUE_WINDOWS
