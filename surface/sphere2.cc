// sphere2.cc -- Alternative sphere surface
//
//  Copyright (C) 2007-2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect/intersect.h"
#include "geometry/sphere-isec.h"
#include "geometry/sphere-sample.h"
#include "light/surface-light.h"

#include "sphere2.h"


using namespace snogray;


// If this surface intersects RAY, change RAY's maximum bound (Ray::t1) to
// reflect the point of intersection, and return a Surface::IsecInfo object
// describing the intersection (which should be allocated using
// placement-new with CONTEXT); otherwise return zero.
//
const Surface::IsecInfo *
Sphere2::intersect (Ray &ray, RenderContext &context) const
{
  Ray oray = world_to_local (ray);
  
  dist_t t;
  if (sphere_intersects (Pos(0,0,0), dist_t(1), oray, t))
    {
      ray.t1 = t;
      return new (context) IsecInfo (ray, *this, Vec (oray (t)));
    }
  return 0;
}

// Create an Intersect object for this intersection.
//
Intersect
Sphere2::IsecInfo::make_intersect (const Media &media, RenderContext &context) const
{
  Pos point = ray.end ();

  // Choose the second tangent vector (perpendicular to ONORM) in object
  // space, OT (this is convenient for later calculating the first tangent
  // vector in world space -- it will then point towards the north pole).
  //
  // We try to make OT point "around" the sphere, by calculating it as
  // the cross product of ONORM and an "up" vector (0,0,1).  However if
  // ONORM itself is (0,0,1) or (0,0,-1), we can't do that; in that
  // case, we choose an arbitrary vector for OT instead.
  //
  Vec ot;
  if (abs (onorm.x) < Eps && abs (onorm.y) < Eps)
    ot = Vec (1, 0, 0);
  else
    ot = cross (onorm, Vec (0, 0, 1));

  // Calculate the normal and tangent vectors in world space.  NORM and
  // T are just ONORM and OT converted from the local coordinate system
  // to world space, and S is just the cross product of NORM and T.
  //
  Vec norm = sphere.normal_to_world (onorm).unit ();
  Vec t = sphere.local_to_world (ot).unit ();
  Vec s = cross (norm, t);

  // Calculate partial derivatives of texture coordinates dTds and dTdt,
  // where T is the texture coordinates (for bump mapping).
  //
  UV dTds (INV_PIf * 0.5f, 0), dTdt (0, INV_PIf);

  return Intersect (ray, media, context, *sphere.material,
		    Frame (point, s, t, norm),
		    sphere.tex_coords (onorm), dTds, dTdt);
}

// Return the texture-coordinates of this intersection.
//
TexCoords
Sphere2::IsecInfo::tex_coords () const
{
  return TexCoords (ray.end(), sphere.tex_coords (onorm));
}

// Return the normal of this intersection (in the world frame).
//
Vec
Sphere2::IsecInfo::normal () const
{
  return sphere.normal_to_world (onorm).unit ();
}

// Return true if this surface intersects RAY.
//
bool
Sphere2::intersects (const Ray &ray, RenderContext &) const
{
  Ray oray = world_to_local (ray);
  dist_t t;
  return sphere_intersects (Pos(0,0,0), dist_t(1), oray, t);
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
Sphere2::occludes (const Ray &ray, const Medium &medium,
		   Color &total_transmittance, RenderContext &)
  const
{
  Ray oray = world_to_local (ray);
  dist_t t;
  if (sphere_intersects (Pos(0,0,0), dist_t(1), oray, t))
    {
      // Avoid unnecessary calculation if possible.
      if (material->fully_occluding ())
	return true;

      Vec onorm (oray (t));

      IsecInfo isec_info (Ray (ray, t), *this, onorm);
      if (material->occlusion_requires_tex_coords ())
	return material->occludes (isec_info,
				   TexCoords (ray (t), tex_coords (onorm)),
				   medium, total_transmittance);
      else
	return material->occludes (isec_info, medium, total_transmittance);
    }
  return false;
}

// Return a sampler for this surface, or zero if the surface doesn't
// support sampling.  The caller is responsible for destroying
// returned samplers.
//
Surface::Sampler *
Sphere2::make_sampler () const
{
  return new Sampler (*this);
}


// Sphere2::Sampler

namespace { // keep local to file

// A functor for Surface::Sampler::sample_with_approx_area_pdf.
// Returns a sample position in world-space based on an input
// parameter.
//
struct PosSampler
{
  PosSampler (const Sphere2 &_sphere) : sphere (_sphere) {}
  Pos operator() (const UV &param) const
  {
    return sphere.local_to_world (Pos (sphere_sample (param)));
  }
  const Sphere2 &sphere;
};

} // namespace


// Return a sample of this surface.
//
Surface::Sampler::AreaSample
Sphere2::Sampler::sample (const UV &param) const
{
  Vec norm = sphere.normal_to_world (sphere_sample (param)).unit ();
  return sample_with_approx_area_pdf (PosSampler (sphere), param, norm);
}

// Return a sample of this surface from VIEWPOINT, based on the
// parameter PARAM.
//
Surface::Sampler::AngularSample
Sphere2::Sampler::sample_from_viewpoint (const Pos &viewpoint, const UV &param)
  const
{
  Pos oviewpoint = sphere.world_to_local (viewpoint);
  Vec onorm = sphere_sample (param);

  // If the normal is not in the hemisphere facing VIEWPOINT, mirror
  // the sample about the sphere's center so that it is.
  //
  UV samp_param = param;
  if (dot (onorm, Vec (oviewpoint)) < 0)
    {
      onorm = -onorm;
      samp_param = sphere_sample_inverse (onorm);
    }

  // Now get an area sample for that point.
  //
  Vec norm = sphere.normal_to_world (onorm).unit ();
  Surface::Sampler::AreaSample area_sample =
    sample_with_approx_area_pdf (PosSampler (sphere), samp_param, norm);

  // Because we mirror samples to always point towards VIEWPOINT, double
  // the PDF, as the same number of samples is concentrated into half
  // the space (the hemisphere facing VIEWPOINT).
  //
  area_sample.pdf *= 2;

  return AngularSample (area_sample, viewpoint);
}

// If a ray from VIEWPOINT in direction DIR intersects this
// surface, return an AngularSample as if the
// Surface::Sampler::sample_from_viewpoint method had returned a
// sample at the intersection position.  Otherwise, return an
// AngularSample with a PDF of zero.
//
Surface::Sampler::AngularSample
Sphere2::Sampler::eval_from_viewpoint (const Pos &viewpoint, const Vec &dir)
  const
{
  // Convert parameters to object-space.
  //
  Pos oviewpoint = sphere.world_to_local (viewpoint);
  Vec odir = sphere.world_to_local (dir); // note, not normalized

  dist_t t;
  if (sphere_intersects (Pos(0,0,0), 1.f, oviewpoint, odir, t))
    {
      // Calculate an appropriate sampling parameter and call
      // Surface::Sampler::sample_from_viewpoint to turn that into a
      // sample.

      Pos opos = oviewpoint + t * odir;
      UV param = sphere_sample_inverse (Vec (opos));

      return sample_from_viewpoint (viewpoint, param);
    }

  return AngularSample ();
}


// arch-tag: 4eacb938-b015-4a82-936b-effbdc72eab5
