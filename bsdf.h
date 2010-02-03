// bsdf.h -- Bi-directional scattering distribution function
//
//  Copyright (C) 2005, 2006, 2007, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __BSDF_H__
#define __BSDF_H__

#include "color.h"
#include "vec.h"
#include "uv.h"


namespace snogray {

class Intersect;


// A Bsdf object represents the state of a Material object at an
// intersection (a particular point on the surface, viewed from a
// particular direction), and is used to calculate how light scatters from
// the surface.
//
class Bsdf
{
public:

  enum Flags {
    REFLECTIVE		= 0x02,
    TRANSMISSIVE 	= 0x04,
    // Mask for all sample directions
    SAMPLE_DIR		= REFLECTIVE|TRANSMISSIVE,

    SPECULAR		= 0x08,
    GLOSSY		= 0x10,
    DIFFUSE		= 0x20,
    // Mask for all surface classes
    SURFACE_CLASS	= SPECULAR|GLOSSY|DIFFUSE,

    // Mask of all flags
    ALL			= SAMPLE_DIR|SURFACE_CLASS
  };

  struct Sample
  {
    Sample (const Color &_val, float _pdf, const Vec &_dir,
	    unsigned _flags = 0)
      : val (_val), pdf (_pdf), dir (_dir), flags (_flags)
    { }
    Sample () : val (0), pdf (0), flags (0) { }

    // The value of the BSDF for this sample.
    //
    Color val;

    // The value of the "probability density function" for this sample
    // in the BSDF's sample distribution.
    //
    // However, if this is a specular sample (with the SPECULAR flag
    // set), the value is not defined (theoretically the value is
    // infinity for specular samples).
    //
    float pdf;

    // The sample direction (the origin is implicit), in the
    // surface-normal coordinate system (where the surface normal is
    // (0,0,1)).
    //
    Vec dir;

    // Flags applying to this sample (see the Flags enum for various
    // values).
    //
    unsigned flags;
  };

  struct Value
  {
    Value (const Color &_val, float _pdf)
      : val (_val), pdf (_pdf)
    { }
    Value () : val (0), pdf (0) { }

    // The value of the BSDF for this value.
    //
    Color val;

    // The value of the "probability density function" for this value in the
    // BSDF's value distribution.
    //
    // However, if this is a purely specular bsdf the pdf is not defined
    // (theoretically the value is infinity for specular values).
    //
    float pdf;
  };

  Bsdf (const Intersect &_isec) : isec (_isec) { }
  virtual ~Bsdf () {}

  // Return a sample of this BSDF, based on the parameter PARAM.
  // FLAGS is the types of samples we'd like.
  //
  virtual Sample sample (const UV &param, unsigned flags = ALL) const = 0;

  // Evaluate this BSDF in direction DIR, and return its value and pdf.
  //
  virtual Value eval (const Vec &dir) const = 0;

  // The intersection where this Bsdf was created.
  //
  const Intersect &isec;
};


}

#endif // __BSDF_H__


// arch-tag: 8360ddd7-dc17-40b8-8319-8f6d61fe62bf