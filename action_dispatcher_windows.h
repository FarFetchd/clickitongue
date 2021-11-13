#include <windows.h>

void mouseButtonEvent(DWORD mouse_event_flag)
{
  INPUT input;
  input.type = INPUT_MOUSE;
  input.mi.dx = input.mi.dy = 0;
  input.mi.mouseData = 0;
  input.mi.dwFlags = mouse_event_flag;
  input.mi.time = 0;
  input.mi.dwExtraInfo = 0;
  SendInput(1, &input, sizeof(INPUT));
}

void ActionDispatcher::leftDown()
{
  mouseButtonEvent(MOUSEEVENTF_LEFTDOWN);
}
void ActionDispatcher::leftUp()
{
  mouseButtonEvent(MOUSEEVENTF_LEFTUP);
}
void ActionDispatcher::rightDown()
{
  mouseButtonEvent(MOUSEEVENTF_RIGHTDOWN);
}
void ActionDispatcher::rightUp()
{
  mouseButtonEvent(MOUSEEVENTF_RIGHTUP);
}
void ActionDispatcher::scrollUp()
{
  MessageBox(NULL, "scrollUp", "scrollUp", MB_OK);
}
void ActionDispatcher::scrollDown()
{
  MessageBox(NULL, "scrollDown", "scrollDown", MB_OK);
}
