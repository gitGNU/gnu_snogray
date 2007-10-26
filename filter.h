// filter.h -- Filter datatype
//
//  Copyright (C) 2006, 2007  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __FILTER_H__
#define __FILTER_H__

#include "snogmath.h"
#include "val-table.h"


namespace snogray {


// 2D filter
//
class Filter
{
public:

  // Return a new a filter depending on the parameters in PARAMS.
  //
  static Filter *make (const ValTable &params);

  virtual ~Filter ();

  virtual float val (float x, float y) const = 0;

  float operator() (float x, float y) const { return val (x, y); }

  float width;
  float inv_width;

protected:

  Filter (float _width)
    : width (_width), inv_width (1 / width)
  { } 

  Filter (const ValTable &params, float def_width)
    : width (params.get_float ("width,w", def_width)), inv_width (1 / width)
  { }
};


}

#endif /* __FILTER_H__ */


// arch-tag: 872c9e08-6d72-4d0b-89ca-d5423c1ea696
