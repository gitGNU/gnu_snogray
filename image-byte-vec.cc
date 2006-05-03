// image-byte-vec.cc -- Common code for image formats based on vectors of bytes
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <cmath>
#include <string>

#include "excepts.h"
#include "string-funs.h"

#include "image-byte-vec.h"

using namespace Snogray;


// Output

ByteVecImageSink::ByteVecImageSink (const std::string &filename,
				    unsigned width, unsigned height,
				    const Params &params)
  : ImageSink (filename, width, height, params),
    gamma_correction (params.get_float ("gamma", 0)),
    output_row (width * 3)
{
  if (gamma_correction != 0)
    gamma_correction = 1 / gamma_correction; // we actually want the inverse
  else
    gamma_correction = 1 / DEFAULT_TARGET_GAMMA;
}

void
ByteVecImageSink::write_row (const ImageRow &row)
{
  unsigned width = row.width;
  ByteVec::iterator p = output_row.begin ();

  for (unsigned x = 0; x < width; x++)
    {
      const Color &col = row[x];
      *p++ = color_component_to_byte (col.r ());
      *p++ = color_component_to_byte (col.g ());
      *p++ = color_component_to_byte (col.b ());
    }

  write_row (output_row);
}

float
ByteVecImageSink::max_intens () const
{
  return 1;
}


// Input

// Called by subclass (usually after reading image header) to finish
// setting up stuff.
//
void
ByteVecImageSource::set_specs (unsigned _width, unsigned _height,
			       unsigned _num_channels, unsigned bit_depth)
{
  width = _width;
  height = _height;
  num_channels = _num_channels;

  // Make sure bit-depth is rational: <= 16, and a power of two
  //
  if (bit_depth <= 0 || bit_depth > 16 || (bit_depth & (bit_depth - 1)))
    throw bad_format (std::string ("unsupported bit depth: ")
		      + stringify (bit_depth));

  // Make sure it's a number of channels we support
  //
  if (_num_channels < 1 || _num_channels > 4)
    throw bad_format (std::string ("unsupported number of channels: ")
		      + stringify (_num_channels));

  // We allocate either one or two bytes per pixel per channel
  // [we don't _really_ support sub-byte bit-depeths -- we rely on
  // subclasses to unpack those into bytes where needed]
  //
  bytes_per_component = bit_depth <= 8 ? 1 : 2;

  component_scale = 1 / (Color::component_t)((1 << bit_depth) - 1);

  input_row.resize (_width * _num_channels * bytes_per_component);
}

void
ByteVecImageSource::read_row (ImageRow &row)
{
  read_row (input_row);

  unsigned width = row.width;
  ByteVec::const_iterator p = input_row.begin ();

  for (unsigned x = 0; x < width; x++)
    {
      Color::component_t r, g, b, a;

      r = next_color_component (p);

      if (num_channels >= 3)
	{
	  g = next_color_component (p);
	  b = next_color_component (p);
	}
      else
	g = b = r;

      if (num_channels == 2 || num_channels == 4)
	a = next_alpha_component (p);
      else
	a = 1;

      row[x] = Color (r, g, b);	// alpha????
    }
}

ByteVecImageSink::~ByteVecImageSink () { } // stop gcc bitching
ByteVecImageSource::~ByteVecImageSource () { } // stop gcc bitching


// arch-tag: ee0370d1-7cdb-42d4-96e3-4cf7757cc2cf
