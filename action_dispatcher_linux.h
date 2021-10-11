xdo_t* getXdoContext()
{
  static xdo_t* xdo = nullptr;
  if (!xdo)
    xdo = xdo_new(0);
  return xdo;
}

// 1 is left, 2 is middle, 3 is right, 4 is wheel up, 5 is wheel down.
void ActionDispatcher::clickLeft()
{
  xdo_click_window(getXdoContext(), CURRENTWINDOW, 1);
}
void ActionDispatcher::leftDown()
{
  xdo_mouse_down(getXdoContext(), CURRENTWINDOW, 1);
}
void ActionDispatcher::leftUp()
{
  xdo_mouse_up(getXdoContext(), CURRENTWINDOW, 1);
}
void ActionDispatcher::clickRight()
{
  xdo_click_window(getXdoContext(), CURRENTWINDOW, 3);
}
void ActionDispatcher::scrollUp()
{
  xdo_click_window(getXdoContext(), CURRENTWINDOW, 4);
}
void ActionDispatcher::scrollDown()
{
  xdo_click_window(getXdoContext(), CURRENTWINDOW, 5);
}
