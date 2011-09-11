// color-io.h -- Debugging output for Color type
//
//  Copyright (C) 2010, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_COLOR_IO_H
#define SNOGRAY_COLOR_IO_H

#include <iosfwd>

#include "color.h"

namespace snogray {

std::ostream& operator<< (std::ostream &os, const Color &color);

}

#endif // SNOGRAY_COLOR_IO_H
