// renderer.cc -- Output rendering object
//
//  Copyright (C) 2006, 2007, 2008, 2009, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "scene.h"
#include "camera.h"
#include "filter.h"
#include "sample-set.h"
#include "unique-ptr.h"

#include "renderer.h"

using namespace snogray;

Renderer::Renderer (const Scene &_scene, const Camera &_camera,
		    unsigned _width, unsigned _height,
		    ImageOutput &_output, unsigned _offs_x, unsigned _offs_y,
		    unsigned max_y_block_size,
		    const GlobalRenderState &_global_state)
  : scene (_scene), camera (_camera), width (_width), height (_height),
    output (_output),
    lim_x (_offs_x), lim_y (_offs_y),
    lim_w (_output.width), lim_h (_output.height),
    context (_global_state),
    camera_samples (context.samples.add_channel<UV> ()),
    focus_samples (context.samples.add_channel<UV> ())
{
  output.set_num_buffered_rows (max_y_block_size);
}



// Render a block of pixels between X,Y and X+W,Y+H.  The coordinates
// are clamped to fit the global rendering limit.
// Note that blocks must be rendered in an order which fits within the
// output's row buffering.
//
void
Renderer::render_block (int x, int y, int w, int h)
{
  // Clamp the parameters to fit our limit (LIM_X, LIM_Y, LIM_W, LIM_H).
  // We also clamp with respect to the physical output boundaries.

  int min_y = lim_y + output.min_y, max_y = lim_y + lim_h;
  int min_x = lim_x, max_x = lim_x + lim_w;

  if (y < min_y)
    {
      h -= (min_y - y);
      y = min_y;
    }
  if (y + h > max_y)
    h = max_y - y;

  if (x < min_x)
    {
      w -= (min_x - x);
      x = min_x;
    }
  if (x + w > max_x)
    w = max_x - x;

  // Now if there's anything left after clamping, render it
  //
  if (h > 0 && w > 0)
    {
      unsigned filt_rad = output.filter_radius ();

      // If rendering the first or last row, and the output filter we're
      // using covers more than a single pixel, we must do a number of rows
      // preceding/following the first/last row for their effect on
      // following/preceding rows.
      //
      if (filt_rad != 0 && y == min_y)
	{
	  y -= filt_rad;
	  h += filt_rad;
	}
      if (filt_rad != 0 && y + h == max_y)
	h += filt_rad;

      // Do the same thing for columns.
      //
      if (filt_rad != 0 && x == min_x)
	{
	  x -= filt_rad;
	  w += filt_rad;
	}
      if (filt_rad != 0 && x + w == max_x)
	w += filt_rad;

      // Render the desired rows row by row, and pixel by pixel
      //
      for (int py = y; py < y + h; py++)
	for (int px = x; px < x + w; px++)
	  render_pixel (px, py);
    }
}



// Render a single output pixel at X,Y.  X and Y will be correctly
// handled even when they're outside the global rendering limit (such
// out-of-bounds pixels may still affect the output, because they are
// included in an in-bound pixel by the output filter).
//
void
Renderer::render_pixel (int x, int y)
{
  SampleSet &samples = context.samples;

  samples.generate ();

  SurfaceInteg &surface_integ = *context.surface_integ;
  Media media (context.default_medium);

  for (unsigned snum = 0; snum < samples.num_samples; snum++)
    {
      SampleSet::Sample sample (samples, snum);

      UV camera_samp = sample.get (camera_samples);
      UV focus_samp = sample.get (focus_samples);

      // The X/Y coordinates of the specific sample S
      //
      float sx = x + camera_samp.u, sy = y + camera_samp.v;

      // Calculate normalized image coordinates U and V (we flip the V
      // coordinate vertically because the output image has zero at the
      // top, whereas rendering coordinates use zero at the bottom).
      //
      float u = sx / width, v = (height - sy) / height;

      // Translate the image position U, V into a ray coming from the
      // camera.
      //
      Ray camera_ray = camera.eye_ray (u, v, focus_samp.u, focus_samp.v);

      // .. calculate what light arrives via that ray.
      //
      Tint tint = surface_integ.Li (camera_ray, media, sample);

      output.add_sample (sx - lim_x, sy - lim_y, tint);

      context.mempool.reset ();
    }
}


// arch-tag: 4c2c754d-4caa-487d-acd2-04bf97d849d3
