// obj.h -- Root of object class hierarchy
//
//  Copyright (C) 2005  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __OBJ_H__
#define __OBJ_H__

#include "vec.h"
#include "color.h"
#include "ray.h"
#include "bbox.h"
#include "material.h"

namespace Snogray {

class Material;
class Voxtree;

class Obj 
{
public:

  Obj (Material::ShadowType _shadow_type = Material::SHADOW_OPAQUE)
    : shadow_type (_shadow_type)
  { }
  virtual ~Obj (); // stop gcc bitching

  // If this object intersects the bounded-ray RAY, change RAY's length to
  // reflect the point of intersection, and return true; otherwise return
  // false.
  //
  // NUM is which intersection to return, for non-flat objects that may
  // have multiple intersections -- 0 for the first, 1 for the 2nd, etc
  // (flat objects will return failure for anything except 0).
  //
  bool intersect (Ray &ray, unsigned num = 0) const
  {
    dist_t dist = intersection_distance (ray, num);

    if (dist > 0 && dist < ray.len)
      {
	ray.set_len (dist);
	return true;
      }
    else
      return false;
  }

  // A simpler interface to intersection: just returns true if this object
  // intersects the bounded-ray RAY.  Unlike the `intersect' method, RAY is
  // never modified.
  //
  bool intersects (const Ray &ray, unsigned num = 0) const
  {
    dist_t dist = intersection_distance (ray, num);
    return dist > 0 && dist < ray.len;
  }

  // Return the distance from RAY's origin to the closest intersection
  // of this object with RAY, or 0 if there is none.  RAY is considered
  // to be unbounded.
  //
  // NUM is which intersection to return, for non-flat objects that may
  // have multiple intersections -- 0 for the first, 1 for the 2nd, etc
  // (flat objects will return failure for anything except 0).
  //
  virtual dist_t intersection_distance (const Ray &ray, unsigned num = 0) const;

  // Returns the normal vector for this surface at POINT.
  // INCOMING is the direction of the incoming ray that has hit POINT;
  // this can be used by dual-sided objects to decide which side's
  // normal to return.
  //
  virtual Vec normal (const Pos &point, const Vec &incoming) const;

  // Return a bounding box for this object.
  //
  virtual BBox bbox () const;

  // Returns the material this object is made from
  //
  virtual const Material *material () const;

  // Add this (or some other ...) objects to SPACE
  //
  virtual void add_to_space (Voxtree &space);

  // What special handling this object needs when it casts a shadow.
  // This is initialized by calling the object's material's
  // `shadow_type' method -- it's too expensive to call that during
  // tracing.
  //
  Material::ShadowType shadow_type;

};

}

#endif /* __OBJ_H__ */

// arch-tag: 85997b65-c9ab-4542-80be-0c3a114593ba
