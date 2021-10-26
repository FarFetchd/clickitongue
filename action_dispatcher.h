#ifndef CLICKITONGUE_ACTION_EFFECTOR_H_
#define CLICKITONGUE_ACTION_EFFECTOR_H_

#include "blocking_queue.h"
#include "constants.h"

class ActionDispatcher
{
public:
  ActionDispatcher(BlockingQueue<Action>* action_queue);

  // returns false if we have shut down
  bool dispatchNextAction();

  void shutdown();

private:
  void clickLeft();
  void leftDown();
  void leftUp();
  void clickRight();
  void scrollUp();
  void scrollDown();

  BlockingQueue<Action>* const action_queue_;
};

// run me in my own thread
void actionDispatch(ActionDispatcher* me);

#endif // CLICKITONGUE_ACTION_EFFECTOR_H_
