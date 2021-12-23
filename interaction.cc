#include "interaction.h"

#include <string>

// =============================Linux and OSX===================================
#ifndef CLICKITONGUE_WINDOWS
#include <cstdio>
void promptInfo(const char* prompt)
{
  printf("\e[1;1H\e[2J");
  printf("\n-------------------------------------------------------------------\n"
         "%s\n-------------------------------------------------------------------\n\n\n",
         prompt);
}

bool promptYesNo(const char* prompt)
{
  int input = 'x';
  while (input != 'y' && input != 'Y' && input != 'n' && input != 'N')
  {
    printf("\e[1;1H\e[2J");
    printf("%s (y/n)  ", prompt);
    fflush(stdout);
    input = getchar();
  }

  // TODO needed for OSX too?
  printf("press any key if this message isn't automatically advanced beyond...");
  fflush(stdout);
  make_getchar_like_getch(); getchar(); resetTermios();
  printf("(ok, now advanced beyond that message)\n");

  return input == 'y' || input == 'Y';
}

// TODO will this work on OSX? or need something different?
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

#endif // not windows
// ==============================End Linux====================================


// ===============================Windows=====================================
#ifdef CLICKITONGUE_WINDOWS
#include <windows.h>
void promptInfo(const char* prompt)
{
  // The manual line breaks I use for Linux console don't look good in Windows
  // message boxes. Here is a hacky little cleanup.
  std::string s(prompt);
  for (int i=1; i<s.size()-1; i++)
    if (s[i] == '\n' && s[i-1] != '\n' && s[i+1] != '\n')
      s[i] = ' ';
  MessageBox(NULL, s.c_str(), "Clickitongue", MB_OK);
}
bool promptYesNo(const char* prompt)
{
  // The manual line breaks I use for Linux console don't look good in Windows
  // message boxes. Here is a hacky little cleanup.
  std::string s(prompt);
  for (int i=1; i<s.size()-1; i++)
    if (s[i] == '\n' && s[i-1] != '\n' && s[i+1] != '\n')
      s[i] = ' ';
  return MessageBox(NULL, s.c_str(), "Clickitongue", MB_YESNO) == IDYES;
}
#endif // CLICKITONGUE_WINDOWS
// =============================End Windows===================================
