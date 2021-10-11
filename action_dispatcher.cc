#include "action_dispatcher.h"

#ifdef CLICKITONGUE_LINUX
extern "C" {
#include "xdo.h"
}
#include "action_dispatcher_linux.h"
#endif // CLICKITONGUE_LINUX
#ifdef CLICKITONGUE_WINDOWS
#include "action_dispatcher_windows.h"
#endif

ActionDispatcher::ActionDispatcher(BlockingQueue<Action>* action_queue)
  : action_queue_(action_queue) {}

bool ActionDispatcher::dispatchNextAction()
{
  std::optional<Action> action = action_queue_->deque();
  if (!action.has_value())
    return false;
  switch (action.value())
  {
  case Action::SayClick:
    static int click_print_num = 0;
    printf("click %d!\n", ++click_print_num);
    break;
  case Action::ClickLeft:
    clickLeft();
    break;
  case Action::LeftDown:
    leftDown();
    break;
  case Action::LeftUp:
    leftUp();
    break;
  case Action::ClickRight:
    clickRight();
    break;
  case Action::ScrollUp:
    scrollUp();
    break;
  case Action::ScrollDown:
    scrollDown();
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
  printf("actionDispatch started\n");
  while (me->dispatchNextAction()) {}
  printf("actionDispatch done\n");
}
