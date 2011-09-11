// sample-gen.h -- Sample generator
//
//  Copyright (C) 2010, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_SAMPLE_GEN_H
#define SNOGRAY_SAMPLE_GEN_H

#include <vector>

#include "uv.h"


namespace snogray {

class Random;


// A sample generator, which can generate a specified number of samples to
// cover a certain number of dimensions "evenly".
//
// This class is defined in generically using a templates, but only certain
// types of samples are supported:  float, UV
//
class SampleGen
{
public:

  // Using RANDOM as a source of randomness, add NUM samples to TABLE
  // through TABLE+NUM.
  //
  template<typename T>
  void gen_samples (Random &random,
		    const typename std::vector<T>::iterator &table,
		    unsigned num)
    const;

  // Return the number of samples we'd like to generate instead of NUM.
  //
  template<typename T>
  unsigned adjust_sample_count (unsigned num) const;

protected:

  // The actual sample generating methods, defined by subclasses.
  // Using RANDOM as a source of randomness, add NUM samples to TABLE
  // through TABLE+NUM.
  //
  virtual void gen_float_samples (Random &random,
				  const std::vector<float>::iterator &table,
				  unsigned num)
    const = 0;
  virtual void gen_uv_samples (Random &random,
			       const std::vector<UV>::iterator &table,
			       unsigned num)
    const = 0;

  // Sample-count adjusting methods defined by subclasses.  By default,
  // NUM is returned unchanged.
  //
  virtual unsigned adjust_float_sample_count (unsigned num) const
  {
    return num;
  }
  virtual unsigned adjust_uv_sample_count (unsigned num) const
  {
    return num;
  }
};


//
// Specializations of SampleGen::gen_samples for supported sample types.
//

template<>
inline void
SampleGen::gen_samples<float> (Random &random,
			       const std::vector<float>::iterator &table,
			       unsigned num)
  const
{
  gen_float_samples (random, table, num);
}

template<>
inline void
SampleGen::gen_samples<UV> (Random &random,
			    const std::vector<UV>::iterator &table,
			    unsigned num)
  const
{
  gen_uv_samples (random, table, num);
}


//
// Specializations of SampleGen::adjust_sample_count for supported
// sample types.
//

template<>
inline unsigned
SampleGen::adjust_sample_count<float> (unsigned num) const
{
  return adjust_float_sample_count (num);
}

template<>
inline unsigned
SampleGen::adjust_sample_count<UV> (unsigned num) const
{
  return adjust_uv_sample_count (num);
}

}

#endif // SNOGRAY_SAMPLE_GEN_H
