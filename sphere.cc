// sphere.cc -- Sphere surface
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
#include "shadow-ray.h"
#include "sphere-isec.h"

#include "sphere.h"


using namespace snogray;


// If this surface intersects RAY, change RAY's maximum bound (Ray::t1) to
// reflect the point of intersection, and return a Surface::IsecInfo object
// describing the intersection (which should be allocated using
// placement-new with CONTEXT); otherwise return zero.
//
const Surface::IsecInfo *
Sphere::intersect (Ray &ray, RenderContext &context) const
{
  dist_t t
    = sphere_intersect (frame.origin, radius, ray.origin, ray.dir, ray.t0);
  if (t > ray.t0 && t < ray.t1)
    {
      ray.t1 = t;
      return new (context) IsecInfo (ray, this);
    }
  else
    return false;
}

// Create an Intersect object for this intersection.
//
Intersect
Sphere::IsecInfo::make_intersect (Trace &trace, RenderContext &context) const
{
  Pos point = ray.end ();

  // Calculate the normal and tangent vectors.
  //
  Vec norm = (point - sphere->frame.origin).unit ();
  Vec s = cross (norm, sphere->frame.z).unit ();
  if (s.length_squared() < Eps)
    s = norm.perpendicular ();	// degenerate case where NORM == AXIS
  Vec t = cross (s, norm);

  // 1 divided by the radius/circumference of the sphere.
  //
  dist_t inv_radius = 1 / sphere->radius;
  dist_t inv_circum = inv_radius * INV_PIf * 0.5f;

  // Intersection point in object space.
  //
  Vec opoint = sphere->frame.to (point);

  // 1 divided by the radius/circumference of a horizontal cut (in "object
  // space") through the sphere at the current location (height == z).
  //
  dist_t z_radius = sqrt (opoint.x*opoint.x + opoint.y*opoint.y);
  dist_t inv_z_radius = z_radius ? 1 / z_radius : 0;
  dist_t inv_z_circum = inv_z_radius * INV_PIf * 0.5f;

  // Calculate texture coordinates, T.
  //
  UV T (atan2 (opoint.y, opoint.x) * INV_PIf * 0.5f + 0.5f,
	asin (clamp (opoint.z * inv_radius, -1.f, 1.f)) * INV_PIf + 0.5f);

  // Calculate partial derivatives of texture coordinates dTds and dTdt,
  // where T is the texture coordinates (for bump mapping).
  //
  UV dTds (inv_z_circum, 0), dTdt (0, inv_circum * 2);

  Intersect isec (ray, trace, context, sphere,
		  Frame (point, s, t, norm), T, dTds, dTdt);

  isec.no_self_shadowing = !isec.back;

  return isec;
}

// Return true if this surface intersects RAY.
//
bool
Sphere::intersects (const ShadowRay &ray, RenderContext &) const
{
  dist_t t
    = sphere_intersect (frame.origin, radius, ray.origin, ray.dir, ray.t0);
  return (t > ray.t0 && t < ray.t1);
}

// Return a bounding box for this surface.
BBox
Sphere::bbox () const
{
  const Pos &cent = frame.origin;
  return BBox (Pos (cent.x - radius, cent.y - radius, cent.z - radius),
	       Pos (cent.x + radius, cent.y + radius, cent.z + radius));
}


// arch-tag: dc88fe85-ed78-4f90-bbe2-7e670fde73a6
