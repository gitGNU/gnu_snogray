// misc-map-tex.h -- Miscellaneous coordinate mappings textures
//
//  Copyright (C) 2008, 2011, 2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_MISC_MAP_TEX_H
#define SNOGRAY_MISC_MAP_TEX_H

#include "util/snogmath.h"
#include "geometry/spherical-coords.h"

#include "tex.h"


namespace snogray {


// Texture for mapping from 3d x-y plane to 2d texture coordinates.
//
template<typename T>
class PlaneMapTex : public Tex<T>
{
public:

  PlaneMapTex (const Ref<Tex<T> > &_tex) : tex (_tex) { }

  virtual T eval (const TexCoords &coords) const
  {
    return tex->eval (TexCoords (coords.pos, UV (coords.pos.x, coords.pos.y)));
  }

  const Ref<Tex<T> > tex;
};


// Texture for mapping from a 3d cylinder to 2d texture coordinates.
//
template<typename T>
class CylinderMapTex : public Tex<T>
{
public:

  CylinderMapTex (const Ref<Tex<T> > &_tex) : tex (_tex) { }

  virtual T eval (const TexCoords &coords) const
  {
    const Pos &pos = coords.pos;
    UV uv (atan2 (pos.x, pos.y) * INV_PI * 0.5f + 0.5f, pos.z);
    return tex->eval (TexCoords (pos, uv));
  }

  const Ref<Tex<T> > tex;
};


// Texture for mapping from a 3d sphere to 2d texture coordinates, using
// a "latitude-longitude" mapping.
//
template<typename T>
class LatLongMapTex : public Tex<T>
{
public:

  LatLongMapTex (const Ref<Tex<T> > &_tex) : tex (_tex) { }

  virtual T eval (const TexCoords &coords) const
  {
    const Pos &pos = coords.pos;
    return tex->eval (TexCoords (pos, z_axis_latlong (Vec (pos))));
  }

  const Ref<Tex<T> > tex;
};


}

#endif // SNOGRAY_MISC_MAP_TEX_H
