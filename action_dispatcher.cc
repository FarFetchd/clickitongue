#include "action_dispatcher.h"

ActionDispatcher::ActionDispatcher(BlockingQueue<Action>* action_queue)
  : action_queue_(action_queue) {}

bool ActionDispatcher::dispatchNextAction()
{
  std::optional<Action> action = action_queue_->deque();
  if (!action.has_value())
    return false;
  switch (action.value())
  {
  case Action::LeftDown:
    leftDown();
    break;
  case Action::LeftUp:
    leftUp();
    break;
  case Action::RightDown:
    rightDown();
    break;
  case Action::RightUp:
    rightUp();
    break;
  case Action::ScrollUp:
    scrollUp();
    break;
  case Action::ScrollDown:
    scrollDown();
    break;
  case Action::NoAction:
    break;
  default:
    printf("action not implemented\n");
  }
  return true;
}

void ActionDispatcher::shutdown()
{
  action_queue_->shutdown();
}

void actionDispatch(ActionDispatcher* me)
{
  while (me->dispatchNextAction()) {}
}




// ================================Linux======================================
#ifdef CLICKITONGUE_LINUX

#include <cstring>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>

int g_linux_uinput_fd = -1;
void initLinuxUinput()
{
  g_linux_uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (g_linux_uinput_fd == -1)
  {
    fprintf(stderr, "couldn't open /dev/uinput to write mouse clicks\n");
    exit(1);
  }
  ioctl(g_linux_uinput_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);

  struct uinput_setup usetup;
  memset(&usetup, 0, sizeof(usetup));
  strcpy(usetup.name, "clickitongue");
  usetup.id.bustype = BUS_USB;
  usetup.id.version = 1;
  // can't find any info about if there are particular reserved values, or
  // even whether anything goes wrong if there's a conflict. like the
  // uapi/linux/input.h header doesn't even have a comment about meaning.
  // so, here are some random values; we'll change them if there's a problem.
  usetup.id.vendor = 0x8675;
  usetup.id.product = 0x1337;

  ioctl(g_linux_uinput_fd, UI_DEV_SETUP, &usetup);
  ioctl(g_linux_uinput_fd, UI_DEV_CREATE);
}

void uinputWrite(int code, int val)
{
  assert(g_linux_uinput_fd != -1);

  struct input_event ie;
  ie.type = EV_KEY;
  ie.code = code;
  ie.value = val;
  ie.time.tv_sec = 0;
  ie.time.tv_usec = 0;
  if (write(g_linux_uinput_fd, &ie, sizeof(ie)) == -1)
    fprintf(stderr, "uinput write %d %d failed\n", code, val);

  struct input_event ie2;
  ie2.type = EV_SYN;
  ie2.code = SYN_REPORT;
  ie2.value = 0;
  ie2.time.tv_sec = 0;
  ie2.time.tv_usec = 0;
  if (write(g_linux_uinput_fd, &ie2, sizeof(ie2)) == -1)
    fprintf(stderr, "uinput EV_SYN SYN_REPORT write failed\n");
}

void ActionDispatcher::leftDown()
{
  uinputWrite(BTN_LEFT, 1);
}
void ActionDispatcher::leftUp()
{
  uinputWrite(BTN_LEFT, 0);
}
void ActionDispatcher::rightDown()
{
  uinputWrite(BTN_RIGHT, 1);
}
void ActionDispatcher::rightUp()
{
  uinputWrite(BTN_RIGHT, 0);
}
void ActionDispatcher::scrollUp()
{
  fprintf(stderr, "scrollUp not implemented. uinput REL_WHEEL?\n");
}
void ActionDispatcher::scrollDown()
{
  fprintf(stderr, "scrollDown not implemented. uinput REL_WHEEL?\n");
}

#endif // CLICKITONGUE_LINUX
// ==============================End Linux====================================


// ===============================Windows=====================================
#ifdef CLICKITONGUE_WINDOWS

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

#endif // CLICKITONGUE_WINDOWS
// =============================End Windows==================================


// ================================OSX========================================
#ifdef CLICKITONGUE_OSX
#error "OSX not supported yet"
#endif // CLICKITONGUE_OSX
// ==============================End OSX======================================
