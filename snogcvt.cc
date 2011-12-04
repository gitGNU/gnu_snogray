// snogcvt.cc -- Image-type conversion utility
//
//  Copyright (C) 2005-2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <iostream>
#include <cstring>

#include "cmdlineparser.h"
#include "image-input.h"
#include "image-output.h"
#include "image-cmdline.h"
#include "string-funs.h"
#include "unique-ptr.h"

using namespace snogray;
using namespace std;



static void
usage (CmdLineParser &clp, ostream &os)
{
  os << "Usage: " << clp.prog_name()
     << "[OPTION...] [SOURCE_IMAGE [OUTPUT_IMAGE]]" << endl;
}

static void
help (CmdLineParser &clp, ostream &os)
{
  usage (clp, os);

  // These macros just makes the source code for help output easier to line up
  //
#define s  << endl <<
#define n  << endl

  os <<
  "Change the format of or transform an image file"
n
s "  -p, --pad-bottom=NUM_ROWS  Add NUM_ROWS black rows at the bottom of the image"
s "                               (before doing any size conversion)"
n
s "      --underlay=UND_IMAGE   Use pixels from UND_IMAGE when they are brighter"
s "                               than the corresponding pixel in SOURCE_IMAGE"
s "                               (UND_IMAGE must be the same size as SOURCE_IMAGE)"
n
s IMAGE_INPUT_OPTIONS_HELP
n
s IMAGE_OUTPUT_OPTIONS_HELP
n
s CMDLINEPARSER_GENERAL_OPTIONS_HELP
n
s "If no filenames are given, standard input or output is used.  Input/output"
s "image formats are guessed using the corresponding filenames when possible"
s "(using the file's extension)."
n
    ;

#undef s
#undef n
}

#define OPT_UNDERLAY	1

int main (int argc, char *const *argv)
{
  // Command-line option specs
  //
  static struct option long_options[] = {
    { "pad-bottom",	required_argument, 0, 'p' },
    { "underlay",	required_argument, 0, OPT_UNDERLAY },
    IMAGE_INPUT_LONG_OPTIONS,
    IMAGE_OUTPUT_LONG_OPTIONS,
    CMDLINEPARSER_GENERAL_LONG_OPTIONS,
    { 0, 0, 0, 0 }
  };
  char short_options[] =
    "p:"
    IMAGE_OUTPUT_SHORT_OPTIONS
    IMAGE_INPUT_SHORT_OPTIONS
    CMDLINEPARSER_GENERAL_SHORT_OPTIONS;
  //
  CmdLineParser clp (argc, argv, short_options, long_options);

  // Parameters set from the command line
  //
  unsigned dst_width = 0, dst_height = 0; // zero means copy from source image
  unsigned pad_bottom = 0;		  // rows of padding to add to src img
  string underlay_image;		  // image file to use as underlay
  ValTable src_params, dst_params;

  // Parse command-line options
  //
  int opt;
  while ((opt = clp.get_opt ()) > 0)
    switch (opt)
      {
      case 'p':
	pad_bottom = clp.unsigned_opt_arg ();
	break;

      case OPT_UNDERLAY:
	underlay_image = clp.opt_arg ();
	break;

	IMAGE_OUTPUT_OPTION_CASES (clp, dst_params);
	IMAGE_INPUT_OPTION_CASES (clp, src_params);
	CMDLINEPARSER_GENERAL_OPTION_CASES (clp);
      }

  if (clp.num_remaining_args() > 2)
    {
      usage (clp, cerr);
      cerr << "Try `" << clp.prog_name() << " --help' for more information"
	   << endl;
      exit (10);
    }

  // Open the input image
  //
  ImageInput src (clp.get_arg(), src_params);

  unsigned padded_src_height = src.height + pad_bottom;

  float src_aspect_ratio = float (src.width) / float (src.height);
  unsigned src_size = max (src.width, src.height);

  get_image_size (dst_params, src_aspect_ratio, src_size,
		  dst_width, dst_height);

  // If the user didn't specify a filter, maybe pick a default
  //
  if (! dst_params.contains ("filter"))
    {
      if (dst_width == src.width || dst_height == padded_src_height)
	//
	// If the image size is not being changed, force no filtering
	//
	dst_params.set ("filter", "none");
    }

  // If the input has an alpha-channel, try to preserve it.
  //
  if (src.has_alpha_channel ())
    dst_params.set ("alpha-channel", true);

  // We catch any exceptions thrown while the output file is open (and
  // then just rethrow them), which ensures that all destructors are
  // called, and thus that the output file's buffers are flushed even
  // if an error occurs while processing.
  //
  // This is necessary because the C++ standard allows an unhandled
  // exception to call std::terminate immediately, without unwinding
  // the stack.
  try
    {
      // Open the output image.
      //
      std::string dst_name = clp.get_arg ();
      ImageOutput dst (dst_name, dst_width, dst_height, dst_params);

      if (src.has_alpha_channel() && !dst.has_alpha_channel())
	std::cerr << clp.err_pfx()
		  << dst_name << ": warning: alpha-channel not preserved"
		  << std::endl;

      // Open the underlay image if necessary.
      //
      ImageInput *underlay = 0;
      if (! underlay_image.empty ())
	{
	  underlay = new ImageInput (underlay_image);

	  if (underlay->width != src.width || underlay->height != src.height)
	    clp.err (underlay_image
		     + ": Underlay image size ("
		     + stringify (underlay->width)
		     + " x " + stringify (underlay->height)
		     + ") must match source image ("
		     + stringify (src.width) + " x " + stringify (src.height)
		     + ")");
	}

      // The scaling we apply during image conversion.
      //
      float x_scale = float (dst_width) / float (src.width);
      float y_scale = float (dst_height) / float (padded_src_height);

      // Copy input image to output image, doing any processing
      //
      ImageRow src_row (src.width);
      ImageRow underlay_row (src.width);
      for (unsigned y = 0; y < src.height; y++)
	{
	  // Read one row of the source image.
	  //
	  src.read_row (src_row);

	  // If there's an underlay, we essentially take the maximum of it and
	  // the source image.  This is useful for HDR light-maps which only
	  // cover one hemisphere, if a whole-sphere low-dynamic-range image
	  // also exists:  the LDR info will used wherever the HDR image is
	  // black (and for light-maps, it doesn't really matter that much if
	  // the alignment between the two images isn't perfect).
	  //
	  if (underlay)
	    {
	      underlay->read_row (underlay_row);

	      for (unsigned x = 0; x < src.width; x++)
		src_row[x] = max (src_row[x], underlay_row[x]);
	    }

	  // Write to the output image, scaling as necessary.
	  //
	  for (unsigned x = 0; x < src.width; x++)
	    dst.add_sample ((x + 0.5f) * x_scale, (y + 0.5f) * y_scale, src_row[x]);
	}

      delete underlay;
    }
  catch (...) { throw; }
}

// arch-tag: 9852837a-ecf5-4400-9b79-f0cca96a6736
