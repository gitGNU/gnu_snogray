// sphere2.h -- Alternative sphere surface
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

#ifndef __SPHERE2_H__
#define __SPHERE2_H__


#include "local-surface.h"


namespace snogray {


// This is a sphere surface which transforms a canonical sphere instead of
// using a simple center position and radius like the Sphere class.
// Sphere2 objects use more memory than Sphere objects, but the transform
// allows more flexibility.
//
class Sphere2 : public LocalSurface
{
public:

  Sphere2 (const Ref<const Material> &mat, const Xform &local_to_world_xform)
    : LocalSurface (mat, local_to_world_xform)
  { }

  // If this surface intersects RAY, change RAY's maximum bound (Ray::t1)
  // to reflect the point of intersection, and return a Surface::IsecInfo
  // object describing the intersection (which should be allocated using
  // placement-new with ISEC_CTX); otherwise return zero.
  //
  virtual const IsecInfo *intersect (Ray &ray, const IsecCtx &isec_ctx) const;

  // Return the strongest type of shadowing effect this surface has on
  // RAY.  If no shadow is cast, Material::SHADOW_NONE is returned;
  // otherwise if RAY is completely blocked, Material::SHADOW_OPAQUE is
  // returned; otherwise, Material::SHADOW_MEDIUM is returned.
  //
  virtual Material::ShadowType shadow (const ShadowRay &ray,
				       const IsecCtx &isec_ctx)
    const;

private:

  struct IsecInfo : public Surface::IsecInfo
  {
    IsecInfo (const Ray &ray, const Sphere2 *_sphere, const Vec &_onorm)
      : Surface::IsecInfo (ray), sphere (_sphere), onorm (_onorm)
    { }

    virtual Intersect make_intersect (Trace &trace) const;

    virtual const Surface *surface () const { return sphere; }

    const Sphere2 *sphere;

    // Intersection normal in SPHERE's local coordinate system.
    //
    Vec onorm;
  };

};


}


#endif /* __SPHERE2_H__ */

// arch-tag: e633a2ec-7f36-4a52-89de-5ab76bdd934f
