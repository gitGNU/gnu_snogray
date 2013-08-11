// image-rgbe.cc -- Radiance RGBE / .hdr (aka .pic) format image handling
//
//  Copyright (C) 2006-2009, 2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

//
// The RGBE format is a shared-exponent floating-point image format used
// by the Radiance rendering system.
//
// It is basically an ASCII header terminated by a blank line, followed
// by an ASCII size-indicator line, and then a series of binary scanlines.
//
// The ASCII size-indicator line is of the format:  -Y height +X width
// where "height" and "width" are integers.
//
// There are two different scanline formats:
// 
// * In "new" format, each scanline consists of two constant bytes with
//   the value 2, a 16-bit big-endian line-length, and four
//   run-length-encoded sections, containing all the red bytes for the
//   scanline, then all the blue bytes, all the green bytes, then all
//   the exponent bytes.
//
// * In "old" format, each scanline consists of (R,G,B,EXP) tuples;
//   special "repeat tuples" with R=G=B=1 store a repeat count in the EXP
//   byte, and cause the previous color to be repeated that many times.
//   If more than one repeat tuple occurs in a row, each subsequent
//   repeat tuple has its repeat count multiplied by 256 more than the
//   previous repeat tuple.
//
// The run-length-encoding consists of a series of runs, each run containing
// a length byte, LEN, and either a single data byte (if LEN is greater than
// 128, in which case the data byte should be repeated LEN - 128 times), or
// a series of LEN data bytes.
//
// The pixel values are by default in units of watts/steradian/meter^2.
// However if the ASCII header defines an EXPOSURE setting, pixels are
// multiples of that value in watts/steradian/meter^2.
//

#include <ostream>
#include <istream>

#include "image-rgbe.h"


using namespace snogray;


// Output

RgbeImageSink::RgbeImageSink (const std::string &filename,
			      unsigned width, unsigned height,
			      const ValTable &params)
  : ImageSink (filename, width, height, params),
    outf (filename.c_str(),
	  std::ios_base::out|std::ios_base::binary|std::ios_base::trunc),
    row_buf (width)
{
  outf << "#?RGBE\n";
  outf << "# Written by snogray\n";
  outf << "\n";
  outf << "-Y " << height << " +X " << width << "\n";
}

void
RgbeImageSink::write_rle_component (byte RgbeColor::*component)
{
  unsigned i = 0;

  while (i < width)
    {
      unsigned run_start, run_len = 0;

      // find start of a run

      for (run_start = i; run_start < width; run_start += run_len)
	{
	  run_len = 1;

	  while (run_len < 127
		 && run_start + run_len < width
		 && (row_buf[run_start + run_len].*component
		     == row_buf[run_start].*component))
	    run_len++;

	  if (run_len >= MIN_RUN_LEN)
	    break;
	}

      if (run_start - i > 1 && run_start - i < MIN_RUN_LEN)
	{
	  unsigned j = i + 1;

	  while (row_buf[j++].*component == row_buf[i].*component)
	    if (j == run_start)
	      {
		outf.put (128 + run_start - i);
		outf.put (row_buf[i].*component);
		i = run_start;
		break;
	      }
	}

      while (i < run_start)
	{
	  unsigned len = run_start - i;

	  if (len > 128)
	    len = 128;

	  outf.put (len);
	  while (len--)
	    outf.put (row_buf[i++].*component);
	}

      if (run_len >= MIN_RUN_LEN)
	{
	  outf.put (128 + run_len);
	  outf.put (row_buf[run_start].*component);
	}
      else
	run_len = 0;

      i += run_len;
    }
}

void
RgbeImageSink::write_row (const ImageRow &row)
{
  // Scanline header
  //
  outf.put (2);
  outf.put (2);
  outf.put ((width >> 8) & 0xFF);
  outf.put (width & 0xFF);

  // Transform our internal color format to RGBE encoded colors.
  //
  for (unsigned i = 0; i < width; i++)
    row_buf[i] = row[i].alpha_scaled_color ();

  // Write the various components of the RGBE-encoded colors in
  // run-length-encoded form.
  //
  write_rle_component (&RgbeColor::r);
  write_rle_component (&RgbeColor::g);
  write_rle_component (&RgbeColor::b);
  write_rle_component (&RgbeColor::exp);
}


// Input

RgbeImageSource::RgbeImageSource (const std::string &filename,
				  const ValTable &params)
  : ImageSource (filename, params),
    inf (filename.c_str(), std::ios_base::binary)
{
  // Check magic number
  //
  std::string magic;
  getline (inf, magic);
  if (magic != "#?RGBE" && magic != "#?RADIANCE")
    open_err ("not a Radiance RGBE file");

  // Skip lines until we find a blank line
  //
  while (!inf.eof () && inf.peek () != '\n')
    while (!inf.eof () && inf.get () != '\n')
      ;
  inf.get(); // eat blank line

  // Parse the size line; the format is:  -Y height +X width
  //
  bool ok = (inf.get() == '-' && inf.get() == 'Y');
  if (ok)
    {
      inf >> height;

      ok = ! inf.fail ();
      if (ok)
	{
	  inf >> std::ws;

	  ok = (inf.get() == '+' && inf.get() == 'X');
	  if (ok)
	    {
	      inf >> width;

	      ok = !inf.fail () && inf.get () == '\n';
	    }
	}
    }
  if (! ok)
    open_err ("malformed dimension line");

  row_buf.resize (width);
}

void
RgbeImageSource::read_rle_component (byte RgbeColor::*component)
{
  unsigned i = 0;

  while (i < width)
    {
      byte len = inf.get ();

      if (len > 128)
	{
	  byte val = inf.get ();

	  len -= 128;

	  while (len-- > 0)
	    row_buf[i++].*component = val;
	}
      else
	while (len-- > 0)
	  row_buf[i++].*component = inf.get ();
    }
}

void
RgbeImageSource::read_row (ImageRow &row)
{
  // We try to distinguish between "old" or "new" formats, by examining
  // the first few bytes, and seeing if they look like a valid "new"
  // format line header.

  // The first two bytes should be have the constant value 2 in "new"
  // format.
  //
  unsigned lh0 = inf.get ();
  unsigned lh1 = inf.get ();

  // The next two bytes are the length of the line in pixels encoded as
  // a big-endian 16-bit number.  This should match the width of the
  // image we got from the image header.
  //
  unsigned ll_hi = inf.get ();
  unsigned ll_lo = inf.get ();

  if (lh0 == 2 && lh1 == 2 && (ll_hi << 8) + ll_lo == width)
    //
    // Looks like "new" format
    {
      // Read in the various components of the rgbe-encoded colors.
      //
      read_rle_component (&RgbeColor::r);
      read_rle_component (&RgbeColor::g);
      read_rle_component (&RgbeColor::b);
      read_rle_component (&RgbeColor::exp);
    }
  else
    //
    // Didn't make any sense in "new" format, must be "old" format.
    {
      // Interpret the apparently bogus line header as the first pixel.
      //
      row_buf[0].r = lh0;
      row_buf[0].g = lh1;
      row_buf[0].b = ll_hi;
      row_buf[0].exp = ll_lo;

      // Pixel index; we already have pixel 0.
      //
      unsigned pix = 1;

      // "repeat-count shift" -- 8-bit repeat counts are left-shifted by
      // this amount.  Consecutive magic repeat-count pixels bump this
      // by 8 each time, allowing very large repeat counts to be
      // specified.
      //
      unsigned rep_shift = 0;

      while (pix < width)
	{
	  // Read next pixel.
	  //
	  row_buf[pix].r = inf.get ();
	  row_buf[pix].g = inf.get ();
	  row_buf[pix].b = inf.get ();
	  row_buf[pix].exp = inf.get ();

	  // See if the pixel "color" is really a repeat count for
	  // previous pixel.
	  //
	  if (row_buf[pix].r == 1 && row_buf[pix].g == 1 && row_buf[pix].b == 1)
	    //
	    // Yes
	    {
	      unsigned rep_count = row_buf[pix].exp << rep_shift;

	      if (rep_count > width - pix)
		err ("pixel repeat-count too large in old-format RGBE line");

	      while (rep_count-- != 0)
		row_buf[pix] = row_buf[pix - 1];

	      rep_shift += 8;	// make next repeat count larger
	    }
	  else
	    //
	    // No, just a normal color.
	    {
	      pix++;
	      rep_shift = 0;
	    }
	}
    }
    
  // Transform them to our internal color format.
  //
  for (unsigned i = 0; i < width; i++)
    row[i] = row_buf[i];
}


// arch-tag: 07aa953d-c887-434a-ad33-405ab1976006
