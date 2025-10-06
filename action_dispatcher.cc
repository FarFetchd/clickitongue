#include "action_dispatcher.h"

#include <thread>

#include "config_io.h"
#include "interaction.h"

void crash(const char* s);
std::vector<Detector*> g_HACK_all_detectors;

ActionDispatcher::ActionDispatcher(BlockingQueue<Action>* action_queue)
  : action_queue_(action_queue) {}

extern bool g_show_debug_info;
bool ActionDispatcher::dispatchNextAction()
{
  std::optional<Action> action = action_queue_->deque();
  if (!action.has_value())
    return false;
  switch (action.value())
  {
  case Action::LeftDown:
    if (g_show_debug_info)
      PRINTF("LeftDown\n");
    leftDown();
    break;
  case Action::LeftUp:
    if (g_show_debug_info)
      PRINTF("LeftUp\n");
    leftUp();
    break;
  case Action::RightDown:
    if (g_show_debug_info)
      PRINTF("RightDown\n");
    rightDown();
    break;
  case Action::RightUp:
    if (g_show_debug_info)
      PRINTF("RightUp\n");
    rightUp();
    break;
  case Action::ScrollUp:
    PRINTF("ScrollUp not implemented\n");//scrollUp();
    break;
  case Action::ScrollDown:
    PRINTF("ScrollDown not implemented\n");//scrollDown();
    break;
  case Action::CopyPaste:
    if (g_show_debug_info)
      PRINTF("CopyPaste\n"); // (write CopyPaste and NoAction into default.clickitongue to use. must have xdotool installed.)
    copyPaste();
    break;
  case Action::JustCopy:
    if (g_show_debug_info)
      PRINTF("JustCopy\n");
    justCopy();
    break;
  case Action::JustPaste:
    if (g_show_debug_info)
      PRINTF("JustPaste\n");
    justPaste();
    break;
  case Action::NoAction:
    break;
  default:
    PRINTF("[unknown action] not implemented\n");
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

// =============== voice to LLM stuff ==================


void copyPrevLines(int lines);
void ctrlC();
void ctrlV();
PokeQueue* lazy_wav_ready() { static PokeQueue* ret = new PokeQueue(); return ret; }
PokeQueue* lazy_voice_rec_finisher() { static PokeQueue* ret = new PokeQueue(); return ret; }
std::unique_ptr<AudioRecording> g_recorder;

void endRecDescribeCode()
{
  lazy_voice_rec_finisher()->poke();
  // sleep for enough time for the user's global hotkey to be released
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  ctrlC();
  lazy_wav_ready()->consumePoke();
  std::string cmd = "python3 ";
  cmd += getConfigDir() + "clickitongue_voice_to_llm.py describeToWhisperAndBack ";
  cmd += g_whisper_url;
  cmd += " ";
  cmd += g_athene_url;
  PRINTF("ended recording, now running describe mode\n");
  system(cmd.c_str());
  ctrlV();
  for (auto d : g_HACK_all_detectors)
    d->enabled_ = true;
}

void endRecDictate()
{
  lazy_voice_rec_finisher()->poke();
  // sleep for enough time for the user's global hotkey to be released
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  copyPrevLines(9);
  lazy_wav_ready()->consumePoke();
  std::string cmd = "python3 ";
  cmd += getConfigDir() + "clickitongue_voice_to_llm.py dictateAndClipboardToWhisperAndBack ";
  cmd += g_whisper_url;
  cmd += " ";
  cmd += g_athene_url;
  PRINTF("ended recording, now running dictate mode\n");
  system(cmd.c_str());
  ctrlV();
  for (auto d : g_HACK_all_detectors)
    d->enabled_ = true;
}

void endRecNothing()
{
  lazy_voice_rec_finisher()->poke();
  lazy_wav_ready()->consumePoke();
  PRINTF("oops! ended recording, doing nothing with it\n");
  for (auto d : g_HACK_all_detectors)
    d->enabled_ = true;
}

void startRecording()
{
  PRINTF("starting recording for voice-to-LLM\n");
  for (auto d : g_HACK_all_detectors)
    d->enabled_ = false;
  g_recorder.reset(new AudioRecording(lazy_voice_rec_finisher(), "/tmp/scifiscribe_to_whisper.wav",
                                      lazy_wav_ready()));
}


// ================================Linux======================================
#ifdef CLICKITONGUE_LINUX

#include <cstring>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>

void readFIFO()
{
  mkfifo("/tmp/clickitongue_fifo", 0666);
  int fifo_fd = open("/tmp/clickitongue_fifo", O_RDONLY);
  if (fifo_fd == -1)
    crash("couldn't open fifo /tmp/clickitongue_fifo");
  char buf;
  bool recording = false;
  while (true)
  {
    int res = read(fifo_fd, &buf, 1);
    if (res < 0)
      crash("fifo read failed");
    else if (res == 0)
    {
      close(fifo_fd);
      fifo_fd = open("/tmp/clickitongue_fifo", O_RDONLY);
    }
    else
    {
      if (buf == 'r' && !recording)
      {
        recording = true;
        startRecording();
      }
      else if (recording)
      {
        if (buf == 'i')
        {
          recording = false;
          endRecDictate();
        }
        if (buf == 'e')
        {
          recording = false;
          endRecDescribeCode();
        }
        if (buf == 'c')
        {
          recording = false;
          endRecNothing();
        }
      }
    }
  }
}

void crash(const char* s);

int g_linux_uinput_fd = -1;
void initLinuxUinput()
{
  g_linux_uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (g_linux_uinput_fd == -1)
    crash("couldn't open /dev/uinput to write mouse clicks");

  ioctl(g_linux_uinput_fd, UI_SET_EVBIT, EV_KEY);
  ioctl(g_linux_uinput_fd, UI_SET_EVBIT, EV_SYN);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);

  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_LEFTSHIFT);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_UP);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_DOWN);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_RIGHT);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_LEFTCTRL);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_C);
  ioctl(g_linux_uinput_fd, UI_SET_KEYBIT, KEY_V);

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
    PRINTERR(stderr, "uinput write %d %d failed\n", code, val);

  struct input_event ie2;
  ie2.type = EV_SYN;
  ie2.code = SYN_REPORT;
  ie2.value = 0;
  ie2.time.tv_sec = 0;
  ie2.time.tv_usec = 0;
  if (write(g_linux_uinput_fd, &ie2, sizeof(ie2)) == -1)
    PRINTERR(stderr, "uinput EV_SYN SYN_REPORT write failed\n");
}

void copyPrevLines(int lines)
{
  // Hold down left shift
  uinputWrite(KEY_LEFTSHIFT, 1);
  for (int i = 0; i < lines; i++)
  {
    uinputWrite(KEY_UP, 1);
    uinputWrite(KEY_UP, 0);
  }
  // Release left shift
  uinputWrite(KEY_LEFTSHIFT, 0);

  // Press Ctrl+C to copy
  uinputWrite(KEY_LEFTCTRL, 1);
  uinputWrite(KEY_C, 1);
  uinputWrite(KEY_C, 0);
  uinputWrite(KEY_LEFTCTRL, 0);

  // return to original position
  uinputWrite(KEY_RIGHT, 1);
  uinputWrite(KEY_RIGHT, 0);
}

void ctrlC()
{
  uinputWrite(KEY_LEFTCTRL, 1);
  uinputWrite(KEY_C, 1);
  uinputWrite(KEY_C, 0);
  uinputWrite(KEY_LEFTCTRL, 0);
}

void ctrlV()
{
  uinputWrite(KEY_LEFTCTRL, 1);
  uinputWrite(KEY_V, 1);
  uinputWrite(KEY_V, 0);
  uinputWrite(KEY_LEFTCTRL, 0);
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
void ActionDispatcher::copyPaste()
{
  if (currently_pasting_)
    justPaste();
  else
    justCopy();
  currently_pasting_ = !currently_pasting_;
}
void ActionDispatcher::justCopy()
{
  system("xdotool key control+c");
}
void ActionDispatcher::justPaste()
{
  system("xdotool key control+v");
}

#endif // CLICKITONGUE_LINUX
// ==============================End Linux====================================


// ===============================Windows=====================================
#ifdef CLICKITONGUE_WINDOWS

#include <windows.h>

void readFIFO()
{
  // TODO implement some sort of global hotkey reading to support voice-to-LLM on windows
}

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
void ActionDispatcher::copyPaste()
{ // TODO
}
void ActionDispatcher::justCopy()
{
}
void ActionDispatcher::justPaste()
{
}

#endif // CLICKITONGUE_WINDOWS
// =============================End Windows==================================


// ================================OSX========================================
#ifdef CLICKITONGUE_OSX

#include <chrono>
#include <ApplicationServices/ApplicationServices.h>

uint64_t curTimeMSSE()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}
uint64_t g_osx_last_lclick_msse = 0;
int g_osx_lclicks = 1;
uint64_t g_osx_last_rclick_msse = 0;
int g_osx_rclicks = 1;

CGPoint getCursorPosition()
{
  CGEventRef whereami = CGEventCreate(NULL);
  CGPoint ret = CGEventGetLocation(whereami);
  CFRelease(whereami);
  return ret;
}

void ActionDispatcher::leftDown()
{
  uint64_t now_msse = curTimeMSSE();
  if (now_msse - g_osx_last_lclick_msse < kOSXDoubleClickMs)
  {
    g_osx_lclicks++;
    if (g_osx_lclicks > 3)
      g_osx_lclicks = 3;
  }
  else
    g_osx_lclicks = 1;

  CGEventType event_type = kCGEventLeftMouseDown;
  CGMouseButton button = kCGMouseButtonLeft;

  CGEventRef event = CGEventCreateMouseEvent(NULL, event_type,
                                             getCursorPosition(), button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, g_osx_lclicks);
  CGEventPost(kCGSessionEventTap, event);
  CFRelease(event);
}
void ActionDispatcher::leftUp()
{
  uint64_t now_msse = curTimeMSSE();
  g_osx_last_lclick_msse = now_msse;

  CGEventType event_type = kCGEventLeftMouseUp;
  CGMouseButton button = kCGMouseButtonLeft;

  CGEventRef event = CGEventCreateMouseEvent(NULL, event_type,
                                             getCursorPosition(), button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, g_osx_lclicks);
  CGEventPost(kCGSessionEventTap, event);
  CFRelease(event);
}
void ActionDispatcher::rightDown()
{
  uint64_t now_msse = curTimeMSSE();
  if (now_msse - g_osx_last_rclick_msse < kOSXDoubleClickMs)
  {
    g_osx_rclicks++;
    if (g_osx_rclicks > 3)
      g_osx_rclicks = 3;
  }
  else
    g_osx_rclicks = 1;

  CGEventType event_type = kCGEventRightMouseDown;
  CGMouseButton button = kCGMouseButtonRight;

  CGEventRef event = CGEventCreateMouseEvent(NULL, event_type,
                                             getCursorPosition(), button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, g_osx_rclicks);
  CGEventPost(kCGSessionEventTap, event);
  CFRelease(event);
}
void ActionDispatcher::rightUp()
{
  uint64_t now_msse = curTimeMSSE();
  g_osx_last_rclick_msse = now_msse;

  CGEventType event_type = kCGEventRightMouseUp;
  CGMouseButton button = kCGMouseButtonRight;

  CGEventRef event = CGEventCreateMouseEvent(NULL, event_type,
                                             getCursorPosition(), button);
  CGEventSetIntegerValueField(event, kCGMouseEventClickState, g_osx_rclicks);
  CGEventPost(kCGSessionEventTap, event);
  CFRelease(event);
}

// TODO test that this actually works on OSX
#include <sys/types.h>
#include <sys/stat.h>
void readFIFO()
{
  mkfifo("/tmp/clickitongue_fifo", 0666);
  int fifo_fd = open("/tmp/clickitongue_fifo", O_RDONLY);
  if (fifo_fd == -1)
    crash("couldn't open fifo /tmp/clickitongue_fifo");
  char buf;
  bool recording = false;
  while (true)
  {
    int res = read(fifo_fd, &buf, 1);
    if (res < 0)
      crash("fifo read failed");
    else if (res == 0)
    {
      close(fifo_fd);
      fifo_fd = open("/tmp/clickitongue_fifo", O_RDONLY);
    }
    else
    {
      if (buf == 'r' && !recording)
      {
        recording = true;
        startRecording();
      }
      else if (recording)
      {
        if (buf == 'i')
        {
          recording = false;
          endRecDictate();
        }
        if (buf == 'e')
        {
          recording = false;
          endRecDescribeCode();
        }
        if (buf == 'c')
        {
          recording = false;
          endRecNothing();
        }
      }
    }
  }
}

#endif // CLICKITONGUE_OSX
// ==============================End OSX======================================
