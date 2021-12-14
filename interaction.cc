#include "interaction.h"

// ================================Linux======================================
#ifdef CLICKITONGUE_LINUX
#include <cstdio>
void promptInfo(const char* prompt)
{
  if (!DOING_DEVELOPMENT_TESTING)
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
    printf("%s (y/n)  ", prompt);
    fflush(stdout);
    input = getchar();
  }
  return input == 'y' || input == 'Y';
}
#endif // CLICKITONGUE_LINUX
// ==============================End Linux====================================


// ===============================Windows=====================================
#ifdef CLICKITONGUE_WINDOWS
#include <windows.h>
void promptInfo(const char* prompt)
{
  MessageBox(NULL, prompt, "Clickitongue", MB_OK);
}
bool promptYesNo(const char* prompt)
{
  return MessageBox(NULL, prompt, "Clickitongue", MB_YESNO) == IDYES;
}
#endif // CLICKITONGUE_WINDOWS
// =============================End Windows===================================


// ================================OSX========================================
#ifdef CLICKITONGUE_OSX
#error "OSX not supported yet"
void promptInfo(const char* prompt)
{
}
bool promptYesNo(const char* prompt)
{
}
#endif // CLICKITONGUE_OSX
// ==============================End OSX======================================
