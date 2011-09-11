// progress.h -- Progress indicator
//
//  Copyright (C) 2006, 2007, 2010, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_PROGRESS_H
#define SNOGRAY_PROGRESS_H

#include <iosfwd>
#include <string>

#include "timeval.h"

namespace snogray {

class Progress
{
public:

  enum Verbosity { QUIET, MINIMAL, CHATTY };

  static float default_update_interval () { return 10; }
  static float startup_interval () { return 20; }

  // Initialize with the desired bounds.  Note that nothing actually
  // happens until the `start' method is called.
  //
  Progress (std::ostream &stream, const std::string &_prefix,
	    int _start_pos, int num,
	    Verbosity _verbosity = CHATTY,
	    float _update_interval = default_update_interval())
    : prefix (_prefix),
      start_pos (_start_pos), end_pos (_start_pos + num),
      last_pos (0), last_update_time (0), update_pos (0),
      ticks_until_forced_update (0), start_time (0),
      verbosity (_verbosity), os (stream),
      update_interval (_update_interval)
  { }

  void start ();
  void update (int pos);
  void end ();

  // Prefix string printed on the progress line.
  //
  std::string prefix;

  // Overall rendering bounds
  //
  int start_pos, end_pos;

  // When we last updated the progress indicator
  //
  int last_pos;
  Timeval last_update_time;

  // When we will next update it
  //
  int update_pos;

  // After this many calls to Progress::update, an update will be forced
  // even if the position hasn't changed much.
  //
  unsigned ticks_until_forced_update;

  // When we started
  //
  Timeval start_time;

  // How chatty to be
  //
  Verbosity verbosity;

  // Where to send progress reports to
  //
  std::ostream &os;

  // How often (approximately) to update (in seconds)
  //
  Timeval update_interval;
};

}

#endif /* SNOGRAY_PROGRESS_H */

// arch-tag: 54fddbd6-cd1a-4b36-81b6-8260d700dd24
