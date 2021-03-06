// model.cc -- A surface encapsulated into its own model
//
//  Copyright (C) 2007, 2009-2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <memory>

#include "util/snogassert.h"
#include "space/space.h"
#include "space/space-builder.h"

#include "model.h"


using namespace snogray;


Model::Model (Surface *surf,
		    const SpaceBuilderFactory &space_builder_factory)
  : _surface (surf),
    space_builder (space_builder_factory.make_space_builder ())
{ }


// Setup our acceleration structure.
//
void
Model::make_space () const
{
  LockGuard guard (make_space_lock);

  if (! space)
    {
      ASSERT (space_builder);

      _surface->add_to_space (*space_builder);

      space.reset (space_builder->make_space ());

      space_builder.reset ();
    }
}
