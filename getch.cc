// hacky getch
//================================================
#include <termios.h>
#include <stdio.h>
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
