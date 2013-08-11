// arith-tex.h -- arithmetic on textured values
//
//  Copyright (C) 2008, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_ARITH_TEX_H
#define SNOGRAY_ARITH_TEX_H

#include "tex.h"


namespace snogray {


// A texture which is the result of doing an arithmetic operation
//
template<typename T>
class ArithTex : public Tex<T>
{
public:

  enum Op
  {
    ADD, SUB, MUL, DIV, MOD, POW,
    FLOOR, CEIL, TRUNC,		// floor/ceil/trunc (X / Y) * Y
    MIN, MAX, AVG,
    MIRROR,			// abs (X - Y)
    SIN, COS, TAN,		// sin/cos/tan (X * 2 * PI / Y)
    ATAN2
  };

  ArithTex (Op _op, const TexVal<T> &_arg1, const TexVal<T> &_arg2)
    : op (_op), arg1 (_arg1), arg2 (_arg2)
  { }

  // Evaluate this texture at TEX_COORDS.
  //
  virtual T eval (const TexCoords &tex_coords) const;

  // The operation.
  //
  Op op;

  // Arguments to the operation.
  //
  TexVal<T> arg1, arg2;
};


} // namespace snogray


// Include method definitions
//
#include "arith-tex.tcc"


#endif // SNOGRAY_ARITH_TEX_H
