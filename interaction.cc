#include "interaction.h"

#include <cstdio>

bool promptYesNo(const char* prompt)
{
  // TODO support multiple OSes
  int input = 'x';
  while (input != 'y' && input != 'Y' && input != 'n' && input != 'N')
  {
    printf("%s (y/n)  ", prompt);
    fflush(stdout);
    input = getchar();
  }
  return input == 'y' || input == 'Y';
}

void promptInfo(const char* prompt)
{
  // TODO support multiple OSes
  if (!DOING_DEVELOPMENT_TESTING)
    printf("\e[1;1H\e[2J");
  printf("\n-------------------------------------------------------------------\n"
         "%s\n-------------------------------------------------------------------\n\n\n",
         prompt);
}
