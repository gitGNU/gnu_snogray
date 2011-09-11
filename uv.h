// uv.h -- UV datatype, for holding 2d texture coordinates
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

#ifndef SNOGRAY_UV_H
#define SNOGRAY_UV_H

#include "xform-base.h"


namespace snogray {


// Pair of values
//
template<typename T>
class TUV
{
public:

  TUV (T _u, T _v) : u (_u), v (_v) { }
  TUV () { }

  // Return this UV transformed by XFORM.
  //
  TUV transformed (const XformBase<T> &xform) const
  {
    return TUV ((u * xform (0, 0) + v * xform (1, 0) + xform (3, 0)),
		(u * xform (0, 1) + v * xform (1, 1) + xform (3, 1)));
  }

  // Transform this UV by XFORM.
  //
  void transform (const XformBase<T> &xform)
  {
    *this = xform (*this);
  }

  TUV operator* (const TUV &uv) const { return TUV (u * uv.u, v * uv.v); }
  TUV operator+ (const TUV &uv) const { return TUV (u + uv.u, v + uv.v); }
  TUV operator- (const TUV &uv) const { return TUV (u - uv.u, v - uv.v); }
  TUV operator/ (const TUV &uv) const { return TUV (u / uv.u, v / uv.v); }

  TUV operator* (T s) const { return TUV (u * s, v * s); }
  TUV operator/ (T s) const { return TUV (u / s, v / s); }

  T u, v;
};

typedef TUV<float> UV;


}

#endif // SNOGRAY_UV_H


// arch-tag: b1f1e25c-0a80-4db0-a0e4-a4fcf52bfa6e
