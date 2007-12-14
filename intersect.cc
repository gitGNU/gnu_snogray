// intersect.h -- Datatype for recording surface-ray intersection results
//
//  Copyright (C) 2005, 2006, 2007  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "surface.h"
#include "material.h"
#include "brdf.h"
#include "trace.h"
#include "scene.h"
#include "global-tstate.h"

#include "intersect.h"

using namespace snogray;



// Finish initialization in a constructor.
//
inline void
Intersect::finish_init ()
{
  // Make sure V (in the normal frame of reference) always has a
  // positive Z component.
  //
  if (back)
    {
      v.z = -v.z;
      normal_frame.z = -normal_frame.z;
    }

  // Setup the "brdf" field by calling Intersect::get_brdf.  This is done
  // separately from the constructor initialization, because we pass the
  // intersect object as argument to Material::get_brdf, and we want it to
  // be in a consistent state.
  //
  brdf = material->get_brdf (*this);
}


Intersect::Intersect (const Ray &_ray, const Surface *_surface,
		      const Pos &_pos, const Vec &_normal, Trace &_trace)
  : ray (_ray), surface (_surface),
    normal_frame (_pos, _normal.unit ()),
    v (normal_frame.to (-_ray.dir.unit ())), back (v.z < 0),
    material (_surface->material), brdf (0),
    smoothing_group (0), no_self_shadowing (false),
    trace (_trace)
{
  finish_init ();
}

  
Color
Intersect::illum () const
{
  trace.global.stats.illum_calls++;
  if (v.z > Eps)
    return trace.illuminator().illum (*this);
  else
    return 0;
}  


Intersect::~Intersect ()
{
  // Destroy the BRDF object.  The memory will be implicitly freed later.
  //
  if (brdf)
    brdf->~Brdf ();
}


// arch-tag: 4e2a9676-9a81-4f69-8702-194e8b9158a9
