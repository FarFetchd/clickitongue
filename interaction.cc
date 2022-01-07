#include "interaction.h"

#include <string>

// =============================Linux and OSX===================================
#ifndef CLICKITONGUE_WINDOWS
#include <cstdio>
extern bool g_show_debug_info;
void promptInfo(const char* prompt)
{
  if (!g_show_debug_info)
    PRINTF("\e[1;1H\e[2J");
  PRINTF("\n-------------------------------------------------------------------\n"
         "%s\n-------------------------------------------------------------------\n\n\n",
         prompt);
}

bool promptYesNo(const char* prompt)
{
  int input = 'x';
  while (input != 'y' && input != 'Y' && input != 'n' && input != 'N')
  {
    if (!g_show_debug_info)
      PRINTF("\e[1;1H\e[2J");
    PRINTF("%s (y/n)  ", prompt);
    fflush(stdout);
    input = getchar();
  }

  PRINTF("press any key if this message isn't automatically advanced beyond...");
  fflush(stdout);
  make_getchar_like_getch(); getchar(); resetTermios();
  PRINTF("(ok, now advanced beyond that message)\n");

  return input == 'y' || input == 'Y';
}

#include <termios.h>
#include <cstdio>
static struct termios old_termios, current_termios;

void make_getchar_like_getch()
{
  tcgetattr(0, &old_termios); // grab old terminal i/o settings
  current_termios = old_termios; // make new settings same as old settings
  current_termios.c_lflag &= ~ICANON; // disable buffered i/o
  current_termios.c_lflag &= ~ECHO;
  tcsetattr(0, TCSANOW, &current_termios); // use these new terminal i/o settings now
}
// can now do:    char ch = getchar();
// once you're done, restore normality with:
void resetTermios()
{
  tcsetattr(0, TCSANOW, &old_termios);
}

void showRecordingBanner()
{
  make_getchar_like_getch(); getchar(); resetTermios();
  PRINTF(kRecordingBanner);
}

void hideRecordingBanner()
{
  promptInfo("");
}

#endif // not windows
// ==========================End Linux and OSX================================



// ===============================Windows=====================================
#ifdef CLICKITONGUE_WINDOWS
#include <windows.h>
#include <cassert>
extern HWND g_printf_hwnd;
extern HWND g_banner_hwnd;
extern HWND g_main_hwnd;

#include <atomic>
extern std::atomic<bool> g_windows_msgbox_active;

void promptInfo(const char* prompt)
{
  // The manual line breaks I use for Linux console don't look good in Windows
  // message boxes. Here is a hacky little cleanup.
  std::string s(prompt);
  for (int i=1; i<s.size()-1; i++)
    if (s[i] == '\n' && s[i-1] != '\n' && s[i+1] != '\n')
      s[i] = ' ';
  g_windows_msgbox_active = true;
  MessageBox(NULL, s.c_str(), "Clickitongue", MB_OK);
  g_windows_msgbox_active = false;
}
bool promptYesNo(const char* prompt)
{
  // The manual line breaks I use for Linux console don't look good in Windows
  // message boxes. Here is a hacky little cleanup.
  std::string s(prompt);
  for (int i=1; i<s.size()-1; i++)
    if (s[i] == '\n' && s[i-1] != '\n' && s[i+1] != '\n')
      s[i] = ' ';
  g_windows_msgbox_active = true;
  bool answer = (MessageBox(NULL, s.c_str(), "Clickitongue", MB_YESNO) == IDYES);
  g_windows_msgbox_active = false;
  return answer;
}

std::string stdstring_sprintf(const char* fmt, va_list args)
{
  std::string ret;
  while (*fmt)
  {
    if (*fmt != '%')
      ret += *(fmt++);
    else
    {
      assert(*(fmt+1) == 'd' || *(fmt+1) == 'g' || *(fmt+1) == 's');
      if (*(fmt+1) == 'd')
      {
        int the_int = va_arg(args, int);
        ret += std::to_string(the_int);
      }
      else if (*(fmt+1) == 'g')
      {
        double the_double = va_arg(args, double);
        ret += std::to_string(the_double);
      }
      else // s
      {
        char* the_str = va_arg(args, char*);
        ret += the_str;
      }
      fmt += 2;
    }
  }
  return ret;
}

#include <chrono>
uint64_t curTimeMSSE()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}
extern HWND g_printf_hwnd;
void PRINTF(const char* fmt...)
{
  va_list args;
  va_start(args, fmt);
  std::string to_print = stdstring_sprintf(fmt, args);
  va_end(args);

  static int lines_filled = 0;
  static uint64_t last_printf_msse = 0;
  static std::string printf_text;
  uint64_t now = curTimeMSSE();
  if (to_print.find("\n") != std::string::npos)
  {
    if (now - last_printf_msse > 5000 || ++lines_filled > 16)
    {
      printf_text = "";
      lines_filled = 0;
    }
  }
  printf_text += to_print;
  last_printf_msse = now;
  SetWindowTextA(g_printf_hwnd, printf_text.c_str());
}

void PRINTERR(FILE* junk, const char* fmt...)
{
  va_list args;
  va_start(args, fmt);
  std::string to_print = "ERROR!!! ";
  to_print += stdstring_sprintf(fmt, args);
  va_end(args);
  promptInfo(to_print.c_str());
}

void showRecordingBanner()
{
  ShowWindow(g_printf_hwnd, SW_HIDE);
  ShowWindow(g_banner_hwnd, SW_SHOW);
  UpdateWindow(g_printf_hwnd);
  UpdateWindow(g_banner_hwnd);
  UpdateWindow(g_main_hwnd);
}

void hideRecordingBanner()
{
  ShowWindow(g_banner_hwnd, SW_HIDE);
  ShowWindow(g_printf_hwnd, SW_SHOW);
  UpdateWindow(g_banner_hwnd);
  UpdateWindow(g_printf_hwnd);
  UpdateWindow(g_main_hwnd);
}

#endif // CLICKITONGUE_WINDOWS
// =============================End Windows===================================
