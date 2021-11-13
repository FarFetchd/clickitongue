#ifndef CLICKITONGUE_CMDLINE_OPTIONS_H_
#define CLICKITONGUE_CMDLINE_OPTIONS_H_

#include "structopt.hpp"

struct ClickitongueCmdlineOpts
{
  std::optional<std::string> mode;
  std::optional<std::string> detector;
  std::optional<int> duration_seconds = 5;

  // record, play
  std::optional<std::string> filename;
};
STRUCTOPT(ClickitongueCmdlineOpts, mode, detector, duration_seconds, filename);

#endif // CLICKITONGUE_CMDLINE_OPTIONS_H_
