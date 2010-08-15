// instance.cc -- Transformed object subspace
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect.h"
#include "space.h"
#include "ray.h"
#include "subspace.h"

#include "instance.h"


using namespace snogray;


// If this surface intersects RAY, change RAY's maximum bound (Ray::t1) to
// reflect the point of intersection, and return a Surface::IsecInfo object
// describing the intersection (which should be allocated using
// placement-new with CONTEXT); otherwise return zero.
//
Surface::IsecInfo *
Instance::intersect (Ray &ray, RenderContext &context) const
{
  // Transform the ray for searching our subspace.
  //
  Ray xformed_ray = world_to_local (ray);

  const Surface::IsecInfo *subspace_isec_info
    = subspace->intersect (xformed_ray, context);

  if (subspace_isec_info)
    {
      ray.t1 = xformed_ray.t1;
      return new (context) IsecInfo (ray, this, subspace_isec_info);
    }
  else
    return 0;
}

// Create an Intersect object for this intersection.
//
Intersect
Instance::IsecInfo::make_intersect (const Media &media, RenderContext &context)
  const
{
  // First make an intersection in our subspace.
  //
  Intersect isec = subspace_isec_info->make_intersect (media, context);

  // Now transform parts of it to be in the global space.
  //
  isec.normal_frame.origin
    = instance->local_to_world (isec.normal_frame.origin);
  isec.normal_frame.x
    = instance->local_to_world (isec.normal_frame.x).unit ();
  isec.normal_frame.y
    = instance->local_to_world (isec.normal_frame.y).unit ();
  isec.normal_frame.z
    = instance->normal_to_world (isec.normal_frame.z).unit ();

  // Self-shadowing is detected via object identity, and object identity is
  // a murky concept for anything in an instance.
  //
  isec.no_self_shadowing = 0;

  return isec;
}

// Return true if this surface intersects RAY.
//
bool
Instance::intersects (const Ray &ray, RenderContext &context) const
{
  // Transform the ray for searching our subspace.
  //
  Ray xformed_ray = world_to_local (ray);
  return subspace->intersects (xformed_ray, context);
}

// Return a bounding box for this surface.
//
BBox
Instance::bbox () const
{
  return local_to_world (subspace->bbox ());
}


// arch-tag: 8b4091cf-bd1e-4355-a880-3919f8e5b1d0
