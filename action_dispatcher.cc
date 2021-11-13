#include "action_dispatcher.h"

#ifdef CLICKITONGUE_LINUX
#include "action_dispatcher_linux.h"
#endif // CLICKITONGUE_LINUX
#ifdef CLICKITONGUE_WINDOWS
#include "action_dispatcher_windows.h"
#endif // CLICKITONGUE_WINDOWS
#ifdef CLICKITONGUE_OSX
#include "action_dispatcher_osx.h"
#endif // CLICKITONGUE_OSX

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
