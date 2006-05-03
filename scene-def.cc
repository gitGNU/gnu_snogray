// scene-def.h -- Scene definition object
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <iostream>
#include <sstream>
#include <iomanip>

#include "scene.h"
#include "camera.h"
#include "cubetex.h"
#include "excepts.h"
#include "image-io.h"
#include "cmdlineparser.h"

#include "scene-def.h"

using namespace Snogray;
using namespace std;


// User command-line camera-commands

static char
eat (istream &stream, const char *choices, char *req_desc = 0)
{
  stream >> ws;

  char ch = stream.eof() ? 0 : stream.peek();

  for (const char *p = choices; *p; p++)
    if (ch == *p)
      {
	stream.get ();		// eat it
	return ch;
      }

  if (req_desc)
    {
      string msg;

      if (ch)
	msg += "Invalid ";
      else
	msg += "Missing ";

      msg += req_desc;

      if (ch)
	{
	  msg += " `";
	  msg += ch;
	  msg += "'";
	}

      msg += "; expected one of ";

      for (const char *p = choices; *p; p++)
	{
	  msg += "`";
	  msg += *p;
	  msg += "'";
	  if (p[1])
	    {
	      if (p > choices || p[2])
		msg += ",";
	      if (! p[2])
		msg += " or";
	      msg += " ";
	    }
	}
      
      throw (runtime_error (msg));
    }

  return 0;  
}

static void
eat_close (istream &stream, char open)
{
  if (open)
    {
      char close_delims[2] = { open, 0 };
      switch (open)
	{
	case '(': close_delims[0] = ')'; break;
	case '[': close_delims[0] = ']'; break;
	case '{': close_delims[0] = '}'; break;
	case '<': close_delims[0] = '>'; break;
	}
      eat (stream, close_delims, "close bracket");
    }
}

static double
read_float (istream &stream, const char *desc)
{
  double val;
  if (stream >> val)
    return val;
  else
    throw runtime_error (string ("Missing/invalid ") + desc);
}

static double
read_angle (istream &stream, const char *desc)
{
  return read_float (stream, desc) * M_PI / 180;
}

static dist_t
read_dist (istream &stream, const char *desc)
{
  return read_float (stream, desc);
}

static Pos
read_pos (istream &stream)
{
  Pos pos;
  char open = eat (stream, "(<[{");
  pos.x = read_float (stream, "x coord");
  eat (stream, ",", "comma");
  pos.y = read_float (stream, "y coord");
  eat (stream, ",", "comma");
  pos.z = read_float (stream, "z coord");
  eat_close (stream, open);
  return pos;
}

static Xform
read_rot_xform (istream &stream, const Camera &camera)
{
  char dir = eat (stream, "udlraxyz", "direction/axis");
  float angle = read_angle (stream, "angle");
  Xform xform;

  if (dir == 'u')
    xform.rotate (camera.right, -angle);
  else if (dir == 'd')
    xform.rotate (camera.right, angle);
  else if (dir == 'l')
    xform.rotate (camera.up, -angle);
  else if (dir == 'r')
    xform.rotate (camera.up, angle);
  else if (dir == 'a')
    xform.rotate (camera.forward, angle);
  else if (dir == 'x')
    xform.rotate_x (angle);
  else if (dir == 'y')
    xform.rotate_y (angle);
  else if (dir == 'z')
    xform.rotate_z (angle);

  return xform;
}

static void
interpret_camera_cmds (Camera &camera, const string &cmds)
{
  istringstream stream (cmds);

  try
    { 
      while (! stream.eof ())
	{
	  char cmd = eat (stream, "gtzmro", "command");

	  if (cmd == 'g')
	    camera.move (read_pos (stream));
	  else if (cmd == 't')
	    camera.point (read_pos (stream));
	  else if (cmd == 'z')
	    camera.zoom (read_float (stream, "zoom factor"));
	  else if (cmd == 'm')
	    {
	      char dir = eat (stream, "udlrfbxyz", "movement direction/axis");
	      dist_t dist = read_dist (stream, "movement distance");

	      if (dir == 'r')
		camera.move (camera.right * dist);
	      else if (dir == 'l')
		camera.move (-camera.right * dist);
	      else if (dir == 'u')
		camera.move (camera.up * dist);
	      else if (dir == 'd')
		camera.move (-camera.up * dist);
	      else if (dir == 'f')
		camera.move (camera.forward * dist);
	      else if (dir == 'b')
		camera.move (-camera.forward * dist);
	      else if (dir == 'x')
		camera.move (Vec (dist, 0, 0));
	      else if (dir == 'y')
		camera.move (Vec (0, dist, 0));
	      else if (dir == 'z')
		camera.move (Vec (0, 0, dist));
	    }
	  else if (cmd == 'r')
	    camera.rotate (read_rot_xform (stream, camera));
	  else if (cmd == 'o')
	    camera.orbit (read_rot_xform (stream, camera).inverse ());

	  eat (stream, ",;/");	// eat delimiter
	}
    }
  catch (runtime_error &err)
    {
      throw runtime_error (cmds + ": Error interpreting camera commands: "
			   + err.what ());
    }
}


// Command-line parsing

SceneDef::Spec
SceneDef::cin_spec ()
{
  std::string explicit_fmt = params.get_string ("format");
  if (explicit_fmt == "test")
    throw runtime_error ("No test-scene name specified");
  else if (explicit_fmt.empty ())
    throw runtime_error ("Scene format must be specified for stream input");

  return Spec ("", "", explicit_fmt);
}

// Parse any scene-definition arguments necessary from CLP.
// At most MAX_SPECS scene specifications will be consumed from CLP.
// The exact aguments required may vary depending on previous options.
//
void
SceneDef::parse (CmdLineParser &clp, unsigned max_specs)
{
  unsigned num = clp.num_remaining_args();

  if (num > max_specs)
    num = max_specs;

  if (num == 0)
    specs.push_back (cin_spec ());
  else
    while (num > 0)
      {
	std::string user_name = clp.get_arg ();

	if (user_name == "-")
	  specs.push_back (cin_spec ());
	else
	  {
	    std::string name = user_name;
	    std::string fmt = params.get_string ("format");

	    if (fmt.empty() && name.substr (0, 5) == "test:")
	      {
		fmt = "test";
		name = name.substr (5);
	      }

	    specs.push_back (Spec (user_name, name, fmt));
	  }

	num--;
      }
}


// Scene loading

// Load a scene using arguments from CLP, into SCENE and CAMERA
//
void
SceneDef::load (Scene &scene, Camera &camera)
{
  // Read in scene file (or built-in test scene)
  //
  for (std::vector<Spec>::iterator spec = specs.begin();
       spec != specs.end(); spec++)
    try
      {
	if (spec->scene_fmt == "test")
	  def_test_scene (spec->name, scene, camera);
	else if (spec->name.empty ())
	  scene.load (cin, spec->scene_fmt, camera);
	else
	  scene.load (spec->name, spec->scene_fmt, camera);
      }
    catch (runtime_error &err)
      {
	string tag = spec->user_name;
	if (tag.empty ())
	  tag = "<standard input>";
	throw runtime_error (tag + ": Error reading scene: " + err.what ());
      }

  // Correct for bogus "gamma correction in lighting"
  //
  float assumed_gamma = params.get_float ("gamma", 1);
  if (assumed_gamma != 1)
    scene.set_assumed_gamma (assumed_gamma);

  // Correct scene lighting
  //
  float light_scale = params.get_float ("light-adj", 1);
  if (light_scale != 1)
    for (Scene::light_iterator_t li = scene.lights.begin();
	 li != scene.lights.end(); li++)
      {
	Light *light = *li;
	light->scale_intensity (light_scale);
      }

  // Override scene parameters specified on command-line
  //
  std::string bg_spec = params.get_string ("background");
  if (! bg_spec.empty ())
    {
      unsigned len = bg_spec.length ();
      if (bg_spec.substr (0, 5) == "cube:")
	scene.set_background (new Cubetex (bg_spec.substr (5)));
      else if (len > 4 && bg_spec.substr (len - 4) == ".ctx"
	       || ImageIo::recognized_filename (bg_spec))
	scene.set_background (new Cubetex (bg_spec));
      else
	scene.set_background (atof (bg_spec.c_str()));
    }

  if (camera_cmds.length () > 0)
    interpret_camera_cmds (camera, camera_cmds);
}



// Returns a string containing the parsed scene specs.
//
std::string
SceneDef::specs_rep () const
{
  std::string rep;

  for (std::vector<Spec>::const_iterator spec = specs.begin();
       spec != specs.end(); spec++)
    {
      if (spec != specs.begin ())
	rep += " ";
      rep += spec->user_name;
    }

  return rep;
}

// arch-tag: b48e19f8-8e7b-46bf-9812-03eeb57fef7e
