// direct-illum.h -- Direct-lighting calculations
//
//  Copyright (C) 2010-2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_DIRECT_ILLUM_H
#define SNOGRAY_DIRECT_ILLUM_H

#include "color.h"
#include "bsdf.h"
#include "sample-set.h"


namespace snogray {


class Intersect;
class ValTable;
class Light;


class DirectIllum
{
public:

  // Global state for this illuminator, for rendering an entire scene.
  //
  class GlobalState
  {
  public:

    // Constructor that allows explicitly setting the number of samples.
    //
    GlobalState (unsigned num_samples);

    unsigned num_samples;
  };

  DirectIllum (RenderContext &context, const GlobalState &global_state);

  // Variant constructor which allows specifying a SampleSet other than the
  // one in CONTEXT.
  //
  DirectIllum (SampleSet &samples, RenderContext &context,
	       const GlobalState &global_state);

  // Given an intersection resulting from a cast ray, sample lights
  // in the scene, and return their contribution in that ray's
  // direction.  FLAGS specifies what part of the BSDF will be used.
  //
  Color sample_lights (const Intersect &isec, const SampleSet::Sample &sample,
		       unsigned flags = (Bsdf::ALL & ~Bsdf::SPECULAR))
    const
  {
    // XXX  For now, just do all lights.  In the future we should add a way
    // to limit the number of light samples in the case where there are
    // many lights (e.g., divide the desired number of light samples among
    // lights in the scene).
    //
    return sample_all_lights (isec, sample, flags);
  }

  // Given the intersection ISEC, resulting from a cast ray, sample
  // all lights in the scene, and return the sum of their
  // contribution in that ray's direction.  FLAGS specifies what
  // part of the BSDF will be used.
  //
  Color sample_all_lights (const Intersect &isec,
			   const SampleSet::Sample &sample,
			   unsigned flags = (Bsdf::ALL & ~Bsdf::SPECULAR))
    const;

  // Use multiple-importance-sampling to estimate the radiance of
  // LIGHT towards ISEC, using LIGHT_PARAM, BSDF_PARAM, and
  // BSDF_LAYER_PARAM to sample both the light and the BSDF.
  // FLAGS specifies what part of the BSDF will be used.
  //
  Color sample_light (const Intersect &isec, const Light *light,
		      const UV &light_param,
		      const UV &bsdf_param, float bsdf_layer_param,
		      unsigned flags = (Bsdf::ALL & ~Bsdf::SPECULAR))
    const;

private:

  // Common portion of constructors.
  //
  void finish_init (SampleSet &samples, RenderContext &context,
		    const GlobalState &global_state);

  // Sample channels for light sampling.
  //
  SampleSet::ChannelVec<UV> light_samp_channels;
  SampleSet::Channel<float> light_select_chan;

  // Sample channels for bsdf sampling.
  //
  SampleSet::ChannelVec<UV> bsdf_samp_channels;
  SampleSet::ChannelVec<float> bsdf_layer_channels;
};


}

#endif // SNOGRAY_DIRECT_ILLUM_H
