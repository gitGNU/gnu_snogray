// image-jpeg.h -- JPEG format image handling
//
//  Copyright (C) 2005-2007, 2010-2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_IMAGE_JPEG_H
#define SNOGRAY_IMAGE_JPEG_H

#include <cstdio>
#include <csetjmp>

// Note: this include should be _before_ the include of <jpeglib.h>,
// to avoid the problems with redefinition of HAVE_STDLIB_H mentioned
// below.
//
#include "image-byte-vec.h"

// Some versions of the libjpeg header files define "HAVE_STDLIB_H" (which
// they probably shouldn't, but...), which results in the compiler
// complaining (or aborting, if --pedantic-errors or -Werror is used).
// To avoid this, undefine HAVE_STDLIB_H before including <jpeglib.h>, and
// then maybe redefine it again afterwards if necessary ... arghh...
//
#ifdef HAVE_STDLIB_H
#define SNOGRAY_HAD_STDLIB_H 1
#undef HAVE_STDLIB_H
#endif
#include <jpeglib.h>
#if !defined(HAVE_STDLIB_H) && SNOGRAY_HAD_STDLIB_H
#define HAVE_STDLIB_H 1
#endif



namespace snogray {

class JpegErrState : public jpeg_error_mgr
{
public:

  JpegErrState (const std::string &_filename);

  // This is similar to the standard C `setjmp': it should be called
  // before attempting a libjpeg operation that might yield an error,
  // and will return false; if an error subsquently happens during the
  // _following_ operation, this call will essentially return a second
  // time, this time with a return value of true.
  //
  // It must be inline, because `setjmp' uses special compiler support.
  //
  bool trap_err () { return err || setjmp (jmpbuf) != 0; }

  // If an error was seen, throw an appropriate exception.
  //
  void throw_err ();

  // True if we saw an error.
  //
  bool err;

  std::string err_msg;

  // Just reference to the filename stored elsewhere.
  //
  const std::string &err_filename;

private:

  // Used for jump from one of our error handlers back past libjpeg
  // library routines into the nearest calling C++ function.
  // Unfortunately we can't just throw an error directly from the error
  // handler, because the C++ runtime seems to abort as soon as it hits
  // a non-C++ function on the stack.
  //
  jmp_buf jmpbuf;

  // Called for fatal errors.
  //
  static void libjpeg_err_handler (j_common_ptr cinfo);

  // Called for warnings (MSG_LEVEL < 0) and "trace messages" (>= 0).
  //
  static void libjpeg_warn_handler (j_common_ptr cinfo, int msg_level);

  // Call to output a message.
  //
  static void libjpeg_msg_handler (j_common_ptr cinfo);
};

class JpegImageSink : public ByteVecImageSink
{  
public:

  static const int DEFAULT_QUALITY = 98;

  JpegImageSink (const std::string &filename,
		 unsigned width, unsigned height,
		 const ValTable &params = ValTable::NONE);
  ~JpegImageSink ();

  virtual void write_row (const ByteVec &rgb_bytes);

  // Write previously written rows to disk, if possible.  This may flush
  // I/O buffers etc., but will not in any way change the output (so for
  // instance, it will _not_ flush the compression state of a PNG output
  // image, as that can make the resulting compression worse).
  //
  virtual void flush ();

private:

  FILE *stream;

  jpeg_compress_struct jpeg_info;

  JpegErrState jpeg_err;
};

class JpegImageSource : public ByteVecImageSource
{  
public:

  JpegImageSource (const std::string &filename,
		   const ValTable &params = ValTable::NONE);
  ~JpegImageSource ();

  virtual void read_row (ByteVec &rgb_bytes);

private:

  FILE *stream;

  struct jpeg_decompress_struct jpeg_info;

  JpegErrState jpeg_err;
};

}

#endif /* SNOGRAY_IMAGE_JPEG_H */


// arch-tag: 354fa041-9c04-419b-a6e5-5c76fb3734cb
