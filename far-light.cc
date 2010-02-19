// far-light.cc -- Light at infinite distance
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "bbox.h"
#include "scene.h"
#include "intersect.h"
#include "sample-cone.h"
#include "sample-tangent-disk.h"

#include "far-light.h"

using namespace snogray;


// FarLight::sample

// Return a sample of this light from the viewpoint of ISEC (using a
// surface-normal coordinate system, where the surface normal is
// (0,0,1)), based on the parameter PARAM.
//
Light::Sample
FarLight::sample (const Intersect &isec, const UV &param) const
{
  // Sample a cone pointing at our light.
  //
  Vec s_dir = isec.normal_frame.to (frame.from (sample_cone (angle, param)));

  if (isec.cos_n (s_dir) > 0 && isec.cos_geom_n (s_dir) > 0)
    return Sample (intensity, pdf, s_dir, 0);

  return Sample ();
}



// Return a "free sample" of this light.
//
Light::FreeSample
FarLight::sample (const UV &param, const UV &dir_param) const
{
  // Note that the sample position and direction are decoupled, as a
  // far-light is "really really far away" from the scene.  A given
  // sample point will appear in the same direction from any location
  // in the scene.

  Vec s_dir = frame.from (sample_cone (angle/2, dir_param));
  Pos s_pos = sample_tangent_disk (scene_center, scene_radius, s_dir, param);

  // Adjust pdf to include disk sampling.
  //
  float s_pdf = pdf / (PIf * scene_radius * scene_radius);

  return FreeSample (intensity, s_pdf, s_pos, -s_dir);
}


// FarLight::eval

// Evaluate this light in direction DIR from the viewpoint of ISEC
// (using a surface-normal coordinate system, where the surface normal
// is (0,0,1)).
//
Light::Value
FarLight::eval (const Intersect &isec, const Vec &dir) const
{
  Vec light_normal_dir = isec.normal_frame.to (frame.z);

  if (dot (dir, light_normal_dir) >= min_cos)
    return Value (intensity, pdf, 0);
  else
    return Value ();
}



// Evaluate this environmental light in direction DIR (in world-coordinates).
//
Color
FarLight::eval_environ (const Vec &dir) const
{
  // Cosine of the angle between DIR and the direction of this light.
  //
  float cos_light_dir = dot (dir, frame.z);

  // If COS_LIGHT_DIR is greater than MIN_COS, then DIR must be within
  // ANGLE/2 of the light direction, so return the light's color;
  // otherwise just return 0.
  //
  return (cos_light_dir > min_cos) ? intensity : 0;
}



// Do any scene-related setup for this light.  This is is called once
// after the entire scene has been loaded.
//
void
FarLight::scene_setup (const Scene &scene)
{
  // Record the center and radius of a bounding sphere for the scene.

  BBox scene_bbox = scene.surfaces.bbox ();

  scene_center = scene_bbox.min + scene_bbox.extent () / 2;
  scene_radius = scene_bbox.extent ().length () / 2;
}


// arch-tag: 879b496d-2a8d-4a7e-8d0a-f92d67d4f165
