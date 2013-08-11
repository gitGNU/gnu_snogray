// cubemap.cc -- Texture wrapped around a cube
//
//  Copyright (C) 2005-2008, 2012, 2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <string>
#include <fstream>
#include <sstream>

#include "util/snogmath.h"
#include "util/excepts.h"
#include "image/image-io.h"
#include "matrix-tex.h"

#include "cubemap.h"

using namespace snogray;

Color
Cubemap::map (const Vec &dir) const
{
  // Choose the main axis of view

  unsigned axis = 0;
  dist_t axis_val = dir.x;

  if (abs (dir.y) > abs (axis_val))
    {
      axis = 1;
      axis_val = dir.y;
    }

  if (abs (dir.z) > abs (axis_val))
    {
      axis = 2;
      axis_val = dir.z;
    }

  // Choose one of the six faces, depending on the axis and diretion
  //
  const Face &face = faces[axis * 2 + (axis_val < 0)];

  // Calculate u and v -- basically the non-axis components of DIR
  // divided by the axis component.
  //
  float u = cos_angle (dir, face.u_dir) / float (axis_val);
  float v = cos_angle (dir, face.v_dir) / float (axis_val);

  // Translate [-1, 1] params into [0, 1] for texture lookup
  //
  UV uv ((u + 1) / 2, (v + 1) / 2);

  // Lookup the value
  //
  return face.tex->eval (TexCoords (Pos (dir), uv));
}



// Return a "light-map" -- a lat-long format spheremap image
// containing light values of the environment map -- for this
// environment map.
//
Ref<Image>
Cubemap::light_map () const
{
  throw std::runtime_error ("Cubemap::light_map");
}

#if 0
void
Cubemap::fill (FillDst &) const
{
  throw std::runtime_error ("Cubemap::fill");

//   for (unsigne fnum = 0; fnum < 6; fnum++)
//     {
//       const Face &face = faces[fnum];
//
//       for (unsigned y = 0; y < face.tex.matrix.height; y++)
//         for (unsigned x = 0; x < face.tex.matrix.width; x++)
//           dst.put (mapping.map (tex.map_coords (x, y)));
//     }
}
#endif


// Cubemap general loading interface

void
Cubemap::load (const std::string &filename)
{
  if (ImageIo::recognized_filename (filename))
    //
    // Load from a single image file
    {
      Ref<Image> image = new Image (filename);

      try
	{
	  load (image);
	}
      catch (std::runtime_error &err)
	{
	  throw file_error (filename + ": Error loading cubemap image: "
				 + err.what ());
	}
    }

  else
    // Load from a "descriptor" file
    {
      std::ifstream stream (filename.c_str ());

      if (stream)
	try
	  { 
	    // Compute filename prefix used for individual image files from
	    // the path used to open the cubemap file.
	    //
	    std::string filename_pfx;
	    unsigned pfx_end = filename.find_last_of ("/");
	    if (pfx_end > 0)
	      filename_pfx = filename.substr (0, pfx_end + 1);

	    load (stream, filename_pfx);
	  }
	catch (std::runtime_error &err)
	  {
	    throw file_error (filename + ": Error loading cubemap file: "
			      + err.what ());
	  }
      else
	throw file_error (filename + ": Cannot open cubemap file");
    }
}


// Loading of a .ctx "descriptor" file

void
Cubemap::load (std::istream &stream, const std::string &filename_pfx)
{
  unsigned num_faces_loaded = 0;

  while (num_faces_loaded < 6)
    {
      bool skipped_comment;
      do
	{
	  skipped_comment = false;
	  stream >> std::ws;
	  if (stream.peek () == '#')
	    {
	      char ch;
	      while (stream.get (ch) && ch != '\n')
		/* nothing */;
	      skipped_comment = true;
	    }
	}
      while (skipped_comment);

      std::string kw;
      stream >> kw;

      unsigned face_num;
      if (kw == "right" || kw == "rgt")
	face_num = 0;
      else if (kw == "left" || kw == "lft")
	face_num = 1;
      else if (kw == "top" || kw == "up")
	face_num = 2;
      else if (kw == "bottom" || kw == "bot" || kw == "down")
	face_num = 3;
      else if (kw == "front" || kw == "fwd" || kw == "forward")
	face_num = 4;
      else if (kw == "back" || kw == "rear" || kw == "bwd" || kw == "backward")
	face_num = 5;
      else
	throw bad_format (kw + ": Unknown face name");

      Face &face = faces[face_num];

      if (face.tex.get ())
	throw bad_format (kw + ": Face defined multiple times");
      else
	num_faces_loaded++;

      std::string u_spec, v_spec;
      stream >> u_spec;
      stream >> v_spec;

      face.u_dir = parse_axis_dir (u_spec);
      face.v_dir = parse_axis_dir (v_spec);

      std::string tex_filename;
      stream >> std::ws;
      getline (stream, tex_filename);

      if (tex_filename[0] != '/' && filename_pfx.length() > 0)
	tex_filename.insert (0, filename_pfx);

      try
	{ 
	  face.tex.reset (new MatrixTex<Color> (tex_filename));
	}
      catch (std::runtime_error &err)
	{
	  throw file_error (std::string ("Error loading texture: ") + err.what ());
	}
    }
}

Vec
Cubemap::parse_axis_dir (const std::string &str)
{
  dist_t val = 1;
  bool bad = false;
  unsigned offs = 0;

  if (str[0] == '+')
    offs++;
  else if (str[0] == '-')
    {
      offs++;
      val = -val;
    }
  else
    bad = true;

  Vec vec;
  if (str[offs] == 'x')
    vec = Vec (val, 0, 0);
  else if (str[offs] == 'y')
    vec = Vec (0, val, 0);
  else if (str[offs] == 'z')
    vec = Vec (0, 0, val);
  else
    bad = true;

  if (str.length() - offs > 1)
    bad = true;

  if (bad)
    throw std::runtime_error (str + ": Illegal axis spec");

  return vec;
}


// Loading of a single background image

void
Cubemap::load (const Ref<Image> &image)
{
  unsigned size;
  unsigned w = image->width, h = image->height;

  if ((size = w / 3) * 3 == w && size * 4 == h)
    //
    // "vertical cross" format
    {
      // Back
      faces[5].tex.reset (
		     new MatrixTex<Color> (image, size, size * 3, size, size));
      faces[5].u_dir = Vec (-1, 0, 0);
      faces[5].v_dir = Vec (0, 1, 0);
    }
  else if ((size = w / 4) * 4 == w && size * 3 == h)
    //
    // "horizontal cross" format
    {
      // Back
      faces[5].tex.reset (
		     new MatrixTex<Color> (image, size * 3, size, size, size));
      faces[5].u_dir = Vec (1, 0, 0);
      faces[5].v_dir = Vec (0, -1, 0);
    }
  else
    throw bad_format ("unrecognized cube-texture image size");

  // Common parts of the two "cross" formats

  // Right
  faces[0].tex.reset (new MatrixTex<Color> (image, size * 2, size, size,size));
  faces[0].u_dir = Vec (0, 0, -1);
  faces[0].v_dir = Vec (0, 1, 0);

  // Left
  faces[1].tex.reset (new MatrixTex<Color> (image, 0, size, size, size));
  faces[1].u_dir = Vec (0, 0, -1);
  faces[1].v_dir = Vec (0, -1, 0);

  // Top
  faces[2].tex.reset (new MatrixTex<Color> (image, size, 0, size, size));
  faces[2].u_dir = Vec (1, 0, 0);
  faces[2].v_dir = Vec (0, 0, -1);

  // Bottom
  faces[3].tex.reset (new MatrixTex<Color> (image, size, size * 2, size,size));
  faces[3].u_dir = Vec (-1, 0, 0);
  faces[3].v_dir = Vec (0, 0, -1);

  // Front
  faces[4].tex.reset (new MatrixTex<Color> (image, size, size, size, size));
  faces[4].u_dir = Vec (1, 0, 0);
  faces[4].v_dir = Vec (0, 1, 0);
}


// arch-tag: 6f62ca7f-6a3e-47d7-a558-3f321b11fd70
