// rect-light.cc -- Rectangular light
//
//  Copyright (C) 2005, 2006, 2007, 2008  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect.h"
#include "grid-iter.h"
#include "tripar-isec.h"

#include "rect-light.h"


using namespace snogray;


// Generate around NUM samples of this light and add them to SAMPLES.
// Return the actual number of samples (NUM is only a suggestion).
//
unsigned
RectLight::gen_samples (const Intersect &isec, unsigned num,
			IllumSampleVec &samples)
  const
{
  // The position and edges of the light, converted to the intersection
  // normal frame of reference.
  //
  Vec org = isec.normal_frame.to (pos);
  Vec s1 = isec.normal_frame.to (side1);
  Vec s2 = isec.normal_frame.to (side2);

  // First detect cases where the light isn't visible at all, by
  // examining the dot product of the surface normal with rays to the
  // four corners of the light.
  //
  if (isec.cos_n (org) < 0
      && isec.cos_n (org + s1) < 0
      && isec.cos_n (org + s2) < 0
      && isec.cos_n (org + s1 + s2) < 0)
    return 0;

  // The light normal in the intersection normal frame of reference.
  //
  Vec light_norm (isec.normal_frame.to (normal));

  GridIter grid_iter (num);

  float u, v;
  while (grid_iter.next (u, v))
    {
      // Compute the position of the sample at U,V within the light.
      //
      const Vec s_vec = org + s1 * u + s2 * v;

      if (isec.cos_n (s_vec) > 0 && isec.cos_geom_n (s_vec) > 0)
	{
	  float dist = s_vec.length ();
	  float inv_dist = 1 / dist;
	  const Vec s_dir = s_vec * inv_dist;

	  // Area to solid-angle conversion, dw/dA
	  //   = cos (light_normal, -sample_dir) / distance^2
	  //
	  float dw_dA = dot (light_norm, s_dir) * inv_dist * inv_dist;

	  if (dw_dA > Eps)
	    {
	      // Pdf Is (1 / Area) * (Dw/Da)
	      //
	      float pdf = 1 / (area * dw_dA);

	      samples.push_back
		(IllumSample (s_dir, intensity, pdf, dist, this));
	    }
	}
    }

  return grid_iter.num_samples ();
}



// For every sample from BEG_SAMPLE to END_SAMPLE which intersects this
// light, and where light is closer than the sample's previously recorded
// light distance (or the previous distance is zero), overwrite the
// sample's light-related fields with information from this light.
//
void
RectLight::filter_samples (const Intersect &isec, 
			   const IllumSampleVec::iterator &beg_sample,
			   const IllumSampleVec::iterator &end_sample)
  const
{
  // The light normal in the intersection normal frame of reference.
  //
  Vec light_norm (isec.normal_frame.to (normal));

  for (IllumSampleVec::iterator s = beg_sample; s != end_sample; s++)
    {
      dist_t dist, u, v;
      Ray ray (isec.normal_frame.origin,
	       isec.normal_frame.from (s->dir),
	       s->light_dist);

      if (parallelogram_intersect (pos, side1, side2, ray, dist, u, v))
	{
	  dist_t dist = ray.t1;

	  // Area to solid-angle conversion, dw/dA
	  //   = cos (light_normal, -sample_dir) / distance^2
	  //
	  float dw_dA = -dot (light_norm, s->dir) / (dist * dist);

	  if (dw_dA > Eps)
	    {
	      // Pdf Is (1 / Area) * (Dw/Da)
	      //
	      s->light_pdf = 1 / (area * dw_dA);

	      s->light_val = intensity; //XXX * s->light_pdf;
	      s->light_dist = dist;
	      s->light = this;
	    }
	}
    }
}


// arch-tag: 60165b73-d34e-4f49-9a90-958daefdeb78
