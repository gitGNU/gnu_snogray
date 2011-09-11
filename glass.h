// glass.h -- Glass (transmissive, reflective) material
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2010, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_GLASS_H
#define SNOGRAY_GLASS_H

#include "material.h"
#include "medium.h"


namespace snogray {

class GlassBsdf;


class Glass : public Material
{
public:

  Glass (const Medium &medium) : _medium (medium) { }

  // Return a new BSDF object for this material instantiated at ISEC.
  //
  virtual Bsdf *get_bsdf (const Intersect &isec) const;

  // Return the medium of this material (used only for refraction).
  //
  virtual const Medium *medium () const { return &_medium; }

private:

  friend class GlassBsdf;

  Medium _medium;
};


}

#endif /* SNOGRAY_GLASS_H */


// arch-tag: 4f86bd63-4099-40de-b81b-c5d397002a3e
