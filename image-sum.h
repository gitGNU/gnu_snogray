// image-sum.h -- Quick calculation of the sum of regions in an image
//
//  Copyright (C) 2006, 2007, 2008, 2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_IMAGE_SUM_H
#define SNOGRAY_IMAGE_SUM_H

#include "image.h"


namespace snogray {


// A class for quickly calculating the sum of any sub-region of an
// image, by using a "summed area table".
//
class ImageSum
{
public:

  template<typename DT>
  ImageSum (const TupleMatrix<Color, DT> &image)
    : sat (image.width, image.height)
  {
    // Fill in summed-area-table

    unsigned w = image.width, h = image.height;
    for (unsigned row = 0; row < h; row++)
      for (unsigned col = 0; col < w; col++)
	{
	  Color sum = image (col, row);

	  if (col != 0)
	    sum += sat (col - 1, row);
	  if (row != 0)
	    sum += sat (col, row - 1);

	  if (col != 0 && row != 0)
	    sum -= sat (col - 1, row - 1);

	  sat (col, row) = sum;
	}
  }

  // Return the sum of all pixels in given area from the input image.
  //
  Color sum (unsigned x, unsigned y, unsigned w, unsigned h) const
  {
    Color sum = 0;

    if (w != 0 && h != 0)
      {
	unsigned lx = x - 1, ly = y - 1;
	unsigned ux = lx + w, uy = ly + h;

	sum += sat (ux, uy);

	if (x != 0)
	  sum -= sat (lx, uy);
	if (y != 0)
	  sum -= sat (ux, ly);

	if (x != 0 && y != 0)
	  sum += sat (lx, ly);
      }

    // It seems that due to precision errors, we can sometimes return
    // (small) negative sums, which can screw up some algorithms.
    //
    // XXX check to see if there's a way to avoid doing this.
    //
    return max (sum, Color (0));
  }

  Color average (unsigned x, unsigned y, unsigned w, unsigned h) const
  {
    return sum (x, y, w, h) / (w * h);
  }

  // An alias for the "sum" method.
  //  
  Color operator() (unsigned x, unsigned y, unsigned w, unsigned h) const
  {
    return sum (x, y, w, h);
  }

// private:

  // The "summed area table": each pixel holds the sum of all pixels in the
  // input image to the left and right of its position.
  //
  // We don't use the default Image data type because it is optimized for
  // space, and doesn't always have enough range to hold the large values
  // used for sums.
  //
  DeepImage sat;
};


// An image-sum table that operatos on the _square_ of each pixel.
//
// This is not a space-efficient implementation -- it merely makes a
// temporary image holding the squared pixels, then constructs an
// ImageSum table from that.
//
class ImageSquareSum : public ImageSum
{
public:

  template<typename DT>
  ImageSquareSum (const TupleMatrix<Color, DT> &image)
    : ImageSum (squared_image (image))
  { }

private:

  template<typename DT>
  static DeepImage squared_image (const TupleMatrix<Color, DT> &image)
  {
    DeepImage sq (image.width, image.height);

    for (unsigned y = 0; y < image.height; y++)
      for (unsigned x = 0; x < image.width; x++)
	{
	  Color p = image (x, y);
	  sq (x, y) = p * p;
	}

    return sq;
  }
};


}

#endif /* SNOGRAY_IMAGE_SUM_H */


// arch-tag: 93672e60-184b-4abd-ac1e-14cae85d12b6
