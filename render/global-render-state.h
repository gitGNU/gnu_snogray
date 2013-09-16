// global-render-state.h -- global information used during rendering
//
//  Copyright (C) 2010, 2011, 2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_GLOBAL_RENDER_STATE_H
#define SNOGRAY_GLOBAL_RENDER_STATE_H

#include "util/val-table.h"
#include "util/unique-ptr.h"
#include "scene/scene.h"
#include "sample-gen.h"
#include "surface-integ.h"
#include "volume-integ.h"


namespace snogray {

class Surface;


// Global state; this contains various read-only global information,
// which will be shared by all rendering threads.
//
class GlobalRenderState
{
public:

  GlobalRenderState (const Surface &scene_contents, const ValTable &_params);

  // Scene being rendered.
  //
  const Scene scene;

  // Alpha value to use for background.
  //
  float bg_alpha;

  // Number of samples per pixel used for rendering.
  //
  unsigned num_samples;

  // A table of named parameters that can affect rendering.
  //
  const ValTable params;

  // Sample generator.
  //
  UniquePtr<SampleGen> sample_gen;

  // Global state for volume integrators.
  // 
  // This should be one of the last fields, so it will be initialized
  // after other fields -- the integrator creation method is passed a
  // reference to the GlobalRenderState object, so we want as much
  // GlobalRenderState state as possible to be valid at that point.
  //
  UniquePtr<VolumeInteg::GlobalState> volume_integ_global_state;

  // Global state for surface integrators.
  //
  // This should be one of the last fields, so it will be initialized
  // after other fields -- the integrator creation method is passed a
  // reference to the GlobalRenderState object, so we want as much
  // GlobalRenderState state as possible to be valid at that point.
  //
  // During initialization it may be also zero (in particular, while
  // initializing the volume_integ_global_state field).
  //
  UniquePtr<SurfaceInteg::GlobalState> surface_integ_global_state;

private:

  //
  // Helper methods, which basically create and return an appropriate
  // object based on what's in PARAMS.
  //

  static SampleGen *make_sample_gen (const ValTable &params);
  static SpaceBuilderFactory *make_space_builder_factory (
				const ValTable &params);

  // The following helper methods are called after initialization is
  // complete, so aren't static (and can't be, as they refer to this).
  //
  SurfaceInteg::GlobalState *make_surface_integ_global_state (
				      const ValTable &params);
  VolumeInteg::GlobalState *make_volume_integ_global_state (
				     const ValTable &params);
};


}

#endif // SNOGRAY_GLOBAL_RENDER_STATE_H
