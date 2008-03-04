// check-tex.h -- Texture coordinate transform
//
//  Copyright (C) 2008  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __CHECK_TEX_H__
#define __CHECK_TEX_H__

#include "tex.h"


namespace snogray {


// A texture which transforms the texture coordinates.
// Both 2d and 3d coordinates are transformed.
//
template<typename T>
class CheckTex : public Tex<T>
{
public:

  CheckTex (const TexVal<T> &_tex1, const TexVal<T> &_tex2)
    : tex1 (_tex1), tex2 (_tex2)
  { }

  // Evaluate this texture at TEX_COORDS.
  //
  virtual T eval (const TexCoords &tex_coords) const
  {
    // See which sub-texture to use.
    //
    bool use1 = (fmod (tex_coords.uv.u, 1) < 0.5f);
    if (fmod (tex_coords.uv.v, 1) < 0.5f)
      use1 = !use1;

    return use1 ? tex1.eval (tex_coords) : tex2.eval (tex_coords);
  }

  // Sub-textures which form the two parts of the check pattern.
  //
  TexVal<T> tex1, tex2;
};


}

#endif // __CHECK_TEX_H__