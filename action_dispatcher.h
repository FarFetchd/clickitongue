#ifndef CLICKITONGUE_ACTION_EFFECTOR_H_
#define CLICKITONGUE_ACTION_EFFECTOR_H_

#include "audio_recording.h"
#include "blocking_queue.h"
#include "constants.h"
#include "detector.h"

#ifdef CLICKITONGUE_LINUX
extern int g_linux_uinput_fd;
void initLinuxUinput();
#endif // CLICKITONGUE_LINUX

class ActionDispatcher
{
public:
  ActionDispatcher(BlockingQueue<Action>* action_queue);

  // returns false if we have shut down
  bool dispatchNextAction();

  void shutdown();

private:
  void leftDown();
  void leftUp();
  void rightDown();
  void rightUp();
//   void scrollUp();
//   void scrollDown();

  BlockingQueue<Action>* const action_queue_;
};

extern std::vector<Detector*> g_HACK_all_detectors;

// run me in my own thread
void actionDispatch(ActionDispatcher* me);
void readFIFO();

#endif // CLICKITONGUE_ACTION_EFFECTOR_H_
