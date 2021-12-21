#ifndef CLICKITONGUE_INTERACTION_H_
#define CLICKITONGUE_INTERACTION_H_

bool promptYesNo(const char* prompt);

void promptInfo(const char* prompt);

#ifdef CLICKITONGUE_LINUX
void make_getchar_like_getch();
// can now do:    char ch = getchar();
// once you're done, restore normality with:
void resetTermios();
#endif // CLICKITONGUE_LINUX

#endif // CLICKITONGUE_INTERACTION_H_
