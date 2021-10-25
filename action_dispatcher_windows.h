#include <windows.h>

// 1 is left, 2 is middle, 3 is right, 4 is wheel up, 5 is wheel down.
void ActionDispatcher::clickLeft()
{
  MessageBox(NULL, "clickLeft", "clickLeft", MB_OK);
}
void ActionDispatcher::leftDown()
{
  MessageBox(NULL, "leftDown", "leftDown", MB_OK);
}
void ActionDispatcher::leftUp()
{
  MessageBox(NULL, "leftUp", "leftUp", MB_OK);
}
void ActionDispatcher::clickRight()
{
  MessageBox(NULL, "clickRight", "clickRight", MB_OK);
}
void ActionDispatcher::scrollUp()
{
  MessageBox(NULL, "scrollUp", "scrollUp", MB_OK);
}
void ActionDispatcher::scrollDown()
{
  MessageBox(NULL, "scrollDown", "scrollDown", MB_OK);
}
