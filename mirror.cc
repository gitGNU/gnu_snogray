// mirror.cc -- Mirror (reflective) material
//
//  Copyright (C) 2005, 2006, 2007, 2008  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect.h"
#include "lambert.h"
#include "brdf.h"

#include "mirror.h"


using namespace snogray;


// A mirror with a simple lambertian underlying material.
//
Mirror::Mirror (const Ior &_ior,
		const TexVal<Color> &_reflectance, const TexVal<Color> &col)
  : ior (_ior), reflectance (_reflectance),
    underlying_material ((!col.tex && col.default_val < Eps)
			 ? 0 : new Lambert (col))
{ }


class MirrorBrdf : public Brdf
{
public:

  MirrorBrdf (const Mirror &_mirror, const Intersect &_isec)
    : Brdf (_isec),
      underlying_brdf (_mirror.underlying_material
		       ? _mirror.underlying_material->get_brdf (_isec)
		       : 0),
      fres (isec.trace.medium ? isec.trace.medium->ior : 1, _mirror.ior),
      reflectance (_mirror.reflectance.eval (isec))
  { }

  // Generate around NUM samples of this material and add them to SAMPLES.
  // Return the actual number of samples (NUM is only a suggestion).
  //
  virtual unsigned gen_samples (unsigned num, IllumSampleVec &samples) const
  {
    // Generate specular sample.
    //
    Color refl = reflectance * fres.reflectance (isec.cos_n (isec.v));
    if (refl > Eps)
      samples.push_back (
		IllumSample (isec.v.mirror (Vec (0, 0, 1)), refl, 0,
			     IllumSample::SPECULAR|IllumSample::REFLECTIVE));

    // If we have an underlying BRDF, generate samples from that too.
    //
    if (underlying_brdf)
      {
	unsigned base_off = samples.size ();

	// First get the underlying BRDF end_sample generate its native
	// samples.
	//
	num = underlying_brdf->gen_samples (num, samples);

	// Now adjust the samples end_sample remove any light reflected
	// by perfect specular reflection.
	//
	remove_specular_reflection (samples.begin() + base_off, samples.end());

	return num;
      }
    else
      return 0;
  }

  // Add reflectance information for this MATERIAL to samples from BEG_SAMPLE
  // to END_SAMPLE.
  //
  virtual void filter_samples (const IllumSampleVec::iterator &beg_sample,
			       const IllumSampleVec::iterator &end_sample)
    const
  {
    if (underlying_brdf)
      {
	remove_specular_reflection (beg_sample, end_sample);

	// Now that we've removed specularly reflected light, apply the
	// underlying BRDF.
	//
	underlying_brdf->filter_samples (beg_sample, end_sample);
      }
    else
      for (IllumSampleVec::iterator s = beg_sample; s != end_sample; s++)
	s->brdf_val = 0;
  }

private:

  // Remove from SAMPLES any light reflected by perfect specular reflection.
  //
  void remove_specular_reflection (IllumSampleVec::iterator beg_sample,
				   IllumSampleVec::iterator end_sample)
    const
  {
    for (IllumSampleVec::iterator s = beg_sample; s != end_sample; ++s)
      {
	float fres_refl = fres.reflectance (isec.cos_n (s->dir));
	const Color refl = fres_refl * reflectance;
	s->brdf_val *= (1 - refl);
      }
  }

  const Brdf *underlying_brdf;

  const Fresnel fres;

  Color reflectance;
};


// Make a BRDF object for this material instantiated at ISEC.
//
Brdf *
Mirror::get_brdf (const Intersect &isec) const
{
  return new (isec) MirrorBrdf (*this, isec);
}


// arch-tag: b895139d-fe9f-414a-9665-3b5e4b8f691a
