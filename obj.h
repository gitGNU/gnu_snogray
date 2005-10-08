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

namespace Snogray {

class Material;
class Voxtree;

class Obj 
{
public:
  Obj () : no_shadow (false) { }
  virtual ~Obj (); // stop gcc bitching

  virtual dist_t intersection_distance (const Ray &ray) const;

  // Making the following virtual slows things down measurably, and there
  // are no cases where it's needed yet:
  //
  //   virtual bool intersects (const Ray &ray) const;
  //
  bool intersects (const Ray &ray) const
  {
    dist_t dist = intersection_distance (ray);
    return dist > 0 && dist < ray.len;
  }

  // Returns the normal vector for this surface at POINT.
  // EYE_DIR points to the direction the objects is being viewed from;
  // this can be used by dual-sided objects to decide which side's
  // normal to return.
  //
  virtual Vec normal (const Pos &point, const Vec &eye_dir) const;

  // Return a bounding box for this object.
  //
  virtual BBox bbox () const;

  // Returns the material this object is made from
  //
  virtual const Material *material () const;

  // Add this (or some other ...) objects to SPACE
  //
  virtual void add_to_space (Voxtree &space);

  // Ideally this would be represented via some virtual method instead, but
  // it gets used _very_ often and doing so slows down tracing by about 2%.
  //
  bool no_shadow;
};

}

#endif /* __OBJ_H__ */

// arch-tag: 85997b65-c9ab-4542-80be-0c3a114593ba
