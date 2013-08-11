// sphere-light.cc -- Spherical light
//
//  Copyright (C) 2006-2008, 2010, 2012, 2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <stdexcept>

#include "snogmath.h"
#include "material/cos-dist.h"
#include "intersect/intersect.h"
#include "geometry/sphere-isec.h"
#include "geometry/sphere-sample.h"

#include "sphere-light.h"


using namespace snogray;


SphereLight::SphereLight (const Pos &_pos, float _radius,
			  const TexVal<Color> &_intensity)
  : pos (_pos), radius (_radius), intensity (_intensity.default_val) 
{
  if (_intensity.tex)
    throw std::runtime_error
      ("textured intensity not supported by SphereLight");
}


// Return the solid angle subtended by this light, where LIGHT_CENTER_VEC
// is a vector from the viewer to the light's center.
//
float
SphereLight::solid_angle (const Vec &light_center_vec) const
{
  dist_t dist = light_center_vec.length ();
  if (dist < radius)
    return 4 * PIf;
  else
    return 2 * PIf * float (1 - cos (asin (radius / dist)));
}


// SphereLight::sample

// Return a sample of this light from the viewpoint of ISEC (using a
// surface-normal coordinate system, where the surface normal is
// (0,0,1)), based on the parameter PARAM.
//
Light::Sample
SphereLight::sample (const Intersect &isec, const UV &param) const
{
  // Offset of the center of the light sphere from the intersection origin,
  // in the intersection's normal frame of reference.
  //
  const Vec light_center_vec = isec.normal_frame.to (pos);

  // Only do anything if this light is "above the horizon", and so can
  // conceivably be seen from ISEC.
  //
  if (light_center_vec.z >= -radius)
    {
      // The distribution used here is constant over a solid angle when
      // viewed by an external observer, meaning that it also has a
      // constant pdf equal to 1 / solid_angle.
      //
      float pdf = 1 / solid_angle (light_center_vec);

      // The following distribution which is constant over a solid angle
      // when viewed by an external observer.  The algorithm is from the
      // paper "Lightcuts: a scalable approach to illumination", by Bruce
      // Walters, et al.
      //
      dist_t r_sqrt_rand1 = radius * dist_t (sqrt (param.u));
      float rand2_ang = param.v * 2 * PIf;
      dist_t x = r_sqrt_rand1 * dist_t (cos (rand2_ang));
      dist_t y = r_sqrt_rand1 * dist_t (sin (rand2_ang));
      // Note -- the abs here is just to avoid negative numbers caused by
      // floating-point imprecision.
      dist_t z
	= (sqrt (abs (radius * radius - x * x - y * y))
	   * dist_t (sin (PIf * (isec.context.random() - 0.5f))));

      // A vector from the intersection origin to the point (X, Y, Z)
      // within the sphere, in the intersection's normal frame of
      // reference.
      //
      const Vec s_vec = light_center_vec + Vec (x, y, z);

      if (isec.cos_n (s_vec) > 0 && isec.cos_geom_n (s_vec) > 0)
	{
	  const Vec s_dir = s_vec.unit ();

	  // The "real" distance must terminate at the surface of the
	  // sphere, so we need to do that calculation too...
	  //
	  dist_t dist;
	  if (sphere_intersects (Pos(0,0,0), radius,
				 Pos (-light_center_vec), s_dir,
				 dist))
	    return Sample (intensity, pdf, s_dir, dist);
	}
    }

  return Sample ();
}


// SphereLight::sample

// Return a "free sample" of this light.
//
Light::FreeSample
SphereLight::sample (const UV &param, const UV &dir_param) const
{
  // Sample position on sphere's surface.
  //
  Vec s_pos_vec = sphere_sample (param);
  Pos s_pos = pos + s_pos_vec * radius;
  float pos_pdf = 1 / (radius*radius * dist_t (4 * PI));   // 1 / area

  // Sample direction from that position, using a cosine-weighted
  // distribution.
  //
  CosDist dist;
  Vec dir = dist.sample (dir_param);

  // Convert direction sample to world-coordinates.
  //
  Frame frame (s_pos_vec);
  Vec s_dir = frame.from (dir);

  // The PDF is actually POS_PDF * (DIR_PDF * (dA/dw)), where DIR_PDF
  // is the distribution DIST's PDF for DIR, in angular terms, and
  // (dA/dw) is a conversion factor from angular to area terms.
  //
  // However, as we know that DIST is a cosine distribution, whose PDF
  // is cos(theta)/pi (where theta is the angle between DIR and the
  // distribution normal), and since (dA/dw) is 1/cos(theta), the
  // cosine terms cancel out, and we can just use POS_PDF / pi
  // instead.
  //
  float s_pdf = pos_pdf * INV_PIf;

  return FreeSample (intensity, s_pdf, s_pos, s_dir);
}


// SphereLight::eval

// Evaluate this light in direction DIR from the viewpoint of ISEC (using
// a surface-normal coordinate system, where the surface normal is
// (0,0,1)).
//
Light::Value
SphereLight::eval (const Intersect &isec, const Vec &dir) const
{
  // Offset of the center of the light sphere from the intersection origin,
  // in the intersection's normal frame of reference.
  //
  const Vec light_center_vec = isec.normal_frame.to (pos);

  // Only do anything if this light is "above the horizon", and so can
  // conceivably be seen from ISEC.
  //
  if (light_center_vec.z >= -radius)
    {
      dist_t dist;
      if (sphere_intersects (Pos (0,0,0), radius,
			     Pos (-light_center_vec), dir,
			     dist))
	{
	  // The distribution used here is constant over a solid angle when
	  // viewed by an external observer, meaning that it also has a
	  // constant pdf equal to 1 / solid_angle.
	  //
	  float pdf = 1 / solid_angle (light_center_vec);

	  return Value (intensity, pdf, dist);
	}
    }

  return Value ();
}


// arch-tag: 1caf0ba2-7ec6-4814-be51-b57bbda71fe8
