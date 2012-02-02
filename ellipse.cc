// ellipse.cc -- Ellipse surface
//
//  Copyright (C) 2007-2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect.h"
#include "disk-sample.h"

#include "ellipse.h"


using namespace snogray;


// If this surface intersects RAY, change RAY's maximum bound (Ray::t1) to
// reflect the point of intersection, and return a Surface::IsecInfo object
// describing the intersection (which should be allocated using
// placement-new with CONTEXT); otherwise return zero.
//
const Surface::IsecInfo *
Ellipse::intersect (Ray &ray, RenderContext &context) const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    {
      ray.t1 = t;
      return new (context) IsecInfo (ray, *this);
    }
  return 0;
}


// Ellipse::IsecInfo::IsecDetails class

// This class encapsulates calculation of some ellipse intersection
// details.
//
struct Ellipse::IsecInfo::IsecDetails
{
  // Construct a texture-calculator at POINT, for ellipse ELLIPSE.
  //
  IsecDetails (const Pos &point, const Ellipse &ellipse)
  {
    // [We do initialization in the constructor body, rather than
    // using initialization forms, because it allows us to use
    // temporary variables.]

    // The ellipse's two "radii".
    //
    Vec rad1 = ellipse.edge1 / 2;
    Vec rad2 = ellipse.edge2 / 2;
    dist_t inv_rad1_len = 1 / rad1.length ();
    dist_t inv_rad2_len = 1 / rad1.length ();

    // Center of ellipse.
    //
    Pos center = ellipse.corner + rad1 + rad2;

    // Tangent vectors.
    //
    Vec s = rad1 * inv_rad1_len;
    Vec t = cross (s, ellipse.normal);

    // Normal frame.
    //
    norm_frame = Frame (point, s, t, ellipse.normal);

    // 2d texture coordinates.
    //
    Vec ocent = norm_frame.to (center);
    tex_coords = UV (float (-ocent.x * inv_rad1_len) * 0.5f + 0.5f,
		     float (-ocent.y * inv_rad2_len) * 0.5f + 0.5f);
    //
    // TEX_COORDS will not be "correct" in case where edge1 and edge2 are
    // skewed (not perpendicular); it's not really hard to calculate it
    // correctly in that case, but a bit annoying.

    // Calculate partial derivatives of texture coordinates dTds and dTdt,
    // where T is the texture coordinates (for bump mapping).
    //
    dTds = UV (0.5f * float (inv_rad1_len), 0);
    dTdt = UV (0, 0.5f * float (inv_rad2_len));
  }

  // Normal frame.
  //
  Frame norm_frame;

  // Ellipse texture coordinates.
  //
  UV tex_coords;

  // Texture-coordinate partial derivatives (for bump mapping).
  //
  UV dTds, dTdt;
};


// Ellipse::IsecInfo methods

// Create an Intersect object for this intersection.
//
Intersect
Ellipse::IsecInfo::make_intersect (const Media &media, RenderContext &context)
  const
{
  IsecDetails isec_details (ray.end (), ellipse);

  return Intersect (ray, media, context, *ellipse.material,
		    isec_details.norm_frame, isec_details.tex_coords,
		    isec_details.dTds, isec_details.dTdt);
}

// Return the texture-coordinates of this intersection.
//
TexCoords
Ellipse::IsecInfo::tex_coords () const
{
  Pos point = ray.end ();
  IsecDetails isec_details (point, ellipse);

  return TexCoords (point, isec_details.tex_coords);
}

// Return the normal of this intersection (in the world frame).
//
Vec
Ellipse::IsecInfo::normal () const
{
  return ellipse.normal;
}

// Return true if this surface intersects RAY.
//
bool
Ellipse::intersects (const Ray &ray, RenderContext &) const
{
  dist_t t, u, v;
  return intersects (ray, t, u, v);
}

// Return true if this surface completely occludes RAY.  If it does
// not completely occlude RAY, then return false, and multiply
// TOTAL_TRANSMITTANCE by the transmittance of the surface in medium
// MEDIUM.
//
// Note that this method does not try to handle non-trivial forms of
// transparency/translucency (for instance, a "glass" material is
// probably considered opaque because it changes light direction as
// well as transmitting it).
//
bool
Ellipse::occludes (const Ray &ray, const Medium &medium,
		   Color &total_transmittance, RenderContext &)
  const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    {
      // avoid calculating texture coords if possible
      if (material->fully_occluding ())
	return true;

      IsecInfo isec_info (Ray (ray, t), *this);
      return material->occludes (isec_info, medium, total_transmittance);
    }
  return false;
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

// Return a sampler for this surface, or zero if the surface doesn't
// support sampling.  The caller is responsible for destroying
// returned samplers.
//
Surface::Sampler *
Ellipse::make_sampler () const
{
  return new Sampler (*this);
}


// Ellipse::Sampler

// Return a sample of this surface.
//
Surface::Sampler::AreaSample
Ellipse::Sampler::sample (const UV &param) const
{
  dist_t dx, dy;
  disk_sample (dist_t (0.5), param, dx, dy);
  dx += dist_t (0.5);
  dy += dist_t (0.5);
  Pos pos = ellipse.corner + ellipse.edge1 * dx + ellipse.edge2 * dy;

  return AreaSample (pos, ellipse.normal, pdf);
}

// If a ray from VIEWPOINT in direction DIR intersects this
// surface, return an AngularSample as if the
// Surface::Sampler::sample_from_viewpoint method had returned a
// sample at the intersection position.  Otherwise, return an
// AngularSample with a PDF of zero.
//
Surface::Sampler::AngularSample
Ellipse::Sampler::eval_from_viewpoint (const Pos &viewpoint, const Vec &dir)
  const
{
  dist_t t;
  dist_t u, v;
  if (ellipse.intersects (viewpoint, dir, t, u, v))
    {
      Pos pos = viewpoint + t * dir;
      return AngularSample (AreaSample (pos, ellipse.normal, pdf), viewpoint);
    }
  return AngularSample ();
}
