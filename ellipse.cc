// ellipse.cc -- Ellipse surface
//
//  Copyright (C) 2007, 2008, 2009  Miles Bader <miles@gnu.org>
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

#include "ellipse.h"


using namespace snogray;


// If this surface intersects RAY, change RAY's maximum bound (Ray::t1) to
// reflect the point of intersection, and return a Surface::IsecInfo object
// describing the intersection (which should be allocated using
// placement-new with ISEC_CTX); otherwise return zero.
//
const Surface::IsecInfo *
Ellipse::intersect (Ray &ray, const IsecCtx &isec_ctx) const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    {
      ray.t1 = t;
      return new (isec_ctx) IsecInfo (ray, this);
    }
  return 0;
}

// Create an Intersect object for this intersection.
//
Intersect
Ellipse::IsecInfo::make_intersect (Trace &trace) const
{
  Pos point = ray.end ();

  // The ellipse's two "radii".
  //
  Vec rad1 = ellipse->edge1 / 2;
  Vec rad2 = ellipse->edge2 / 2;
  dist_t inv_rad1_len = 1 / rad1.length ();
  dist_t inv_rad2_len = 1 / rad1.length ();

  // Center of ellipse.
  //
  Pos center = ellipse->corner + rad1 + rad2;

  // Calculate the normal and tangent vectors.
  //
  Vec norm = cross (rad2, rad1).unit ();
  Vec s = rad1 * inv_rad1_len;
  Vec t = cross (s, norm);

  // Normal frame.
  //
  Frame norm_frame (point, s, t, norm);

  // 2d texture coordinates.
  //
  Vec ocent = norm_frame.to (center);
  UV tex_coords (-ocent.x * inv_rad1_len * 0.5f + 0.5f,
		 -ocent.y * inv_rad2_len * 0.5f + 0.5f);
  //
  // TEX_COORDS will not be "correct" in case where edge1 and edge2 are
  // skewed (not perpendicular); it's not really hard to calculate it
  // correctly in that case, but a bit annoying.

  // Calculate partial derivatives of texture coordinates dTds and dTdt,
  // where T is the texture coordinates (for bump mapping).
  //
  UV dTds (0.5f * inv_rad1_len, 0), dTdt (0, 0.5f * inv_rad2_len);

  Intersect isec (ray, trace, ellipse, norm_frame, tex_coords, dTds, dTdt);

  isec.no_self_shadowing = true;

  return isec;
}

// Return the strongest type of shadowing effect this surface has on
// RAY.  If no shadow is cast, Material::SHADOW_NONE is returned;
// otherwise if RAY is completely blocked, Material::SHADOW_OPAQUE is
// returned; otherwise, Material::SHADOW_MEDIUM is returned.
//
Material::ShadowType
Ellipse::shadow (const ShadowRay &ray, const IsecCtx &) const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    return material->shadow_type;
  else
    return Material::SHADOW_NONE;
}

// Return a bounding box for this surface.
//
BBox
Ellipse::bbox () const
{
  // This could be a bit more tight...
  //
  BBox bbox (corner);
  bbox += corner + edge1;
  bbox += corner + edge2;
  bbox += corner + edge1 + edge2;
  return bbox;
}
