// direct-integ.h -- Direct-lighting-only surface integrator
//
//  Copyright (C) 2010, 2011, 2012, 2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_DIRECT_INTEG_H
#define SNOGRAY_DIRECT_INTEG_H

#include "global-render-state.h"
#include "direct-illum.h"

#include "recursive-integ.h"


namespace snogray {


// This is a simple surface-integrator, which includes only
// direct-lighting (light falling on surfaces directly from lights).
//
// It is a subclass of RecursiveInteg, and so also handles perfectly
// specular using recursion, and emissive surfaces.
//
class DirectInteg : public RecursiveInteg
{
public:

  // Global state for DirectInteg, for rendering an entire scene.
  //
  class GlobalState;

protected:

  // This method is called by RecursiveInteg to return any radiance
  // not due to specular reflection/transmission or direct emission.
  //
  virtual Color Lo (const Intersect &isec, const Media &media,
		    const SampleSet::Sample &sample)
  {
    return direct_illum.sample_lights (isec, sample);
  }

private:

  // Integrator state for rendering a group of related samples.
  //
  DirectInteg (RenderContext &context, GlobalState &global_state);

  // State used by the direct-lighting calculator.
  //
  DirectIllum direct_illum;
};



// DirectInteg::GlobalState

// Global state for DirectInteg, for rendering an entire scene.
//
class DirectInteg::GlobalState : public SurfaceInteg::GlobalState
{
public:

  GlobalState (const GlobalRenderState &rstate, const ValTable &params)
    : SurfaceInteg::GlobalState (rstate),
      direct_illum (params.get_uint ("light_samples,samples,samps",
				     rstate.params.get_uint ("light_samples",
							     16)))
  { }

  // Return a new integrator, allocated in context.
  //
  virtual SurfaceInteg *make_integrator (RenderContext &context)
  {
    return new DirectInteg (context, *this);
  }

private:

  friend class DirectInteg;

  DirectIllum::GlobalState direct_illum;
};



// inline methods

// Integrator state for rendering a group of related samples.
//
inline
DirectInteg::DirectInteg (RenderContext &context, GlobalState &global_state)
  : RecursiveInteg (context),
    direct_illum (context, global_state.direct_illum)
{ }


}

#endif // SNOGRAY_DIRECT_INTEG_H
