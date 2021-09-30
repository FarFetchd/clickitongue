#ifndef CLICKITONGUE_GETCH_H_
#define CLICKITONGUE_GETCH_H_

// Blocking keyboard-hit (i.e. enter key not needed) input.

// first do:
void make_getchar_like_getch();
// can now do:
//            char ch = getchar();
// once you're done, restore normality with:
void resetTermios();

#endif // CLICKITONGUE_GETCH_H_
