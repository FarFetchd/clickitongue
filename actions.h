#ifndef CLICKITONGUE_ACTIONS_H_
#define CLICKITONGUE_ACTIONS_H_

enum class Action
{
  ClickLeft, ClickRight, LeftDown, LeftUp, RightDown, RightUp, ScrollDown, ScrollUp,
  // these two are only used in training. RecordCurFrame requires having called
  // setCurFrameDest() and setCurFrameSource().
  SayClick, RecordCurFrame
};

#endif // CLICKITONGUE_ACTIONS_H_
