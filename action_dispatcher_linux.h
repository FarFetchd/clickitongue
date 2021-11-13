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
