// trace-stats.cc -- Print post-rendering statistics
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
#include <iomanip>

#include "space.h"
#include "scene.h"
#include "string-funs.h"

#include "trace-stats.h"

using namespace Snogray;
using namespace std;

// Print post-rendering scene statistics
//
void
TraceStats::print (ostream &os, const Scene &scene)
{
  const Space::IsecStats &tistats1 = space_intersect;
  const Space::IsecStats &tistats2 = space_shadow;

  const Space::Stats tstats = scene.space.stats ();

  long long sc  = scene_intersect_calls;
  long long tnc = tistats1.node_intersect_calls;
  long long ocic = surface_intersect_calls;
  long long hhh = horizon_hint_hits;
  long long hhm = horizon_hint_misses;

  os << endl;
  os << "Rendering stats:" << endl;
  os << "  intersect:" << endl;
  os << "     rays:            " << setw (16) << commify (sc) << endl;
  os << "     horizon hint hits:" << setw (15) << commify (hhh)
     << " (" << setw(2) << (100 * hhh / sc) << "%)" << endl;
  os << "     horizon hint misses:" << setw (13) << commify (hhm)
     << " (" << setw(2) << (100 * hhm / sc) << "%)" << endl;
  if (tstats.num_nodes != 0)
    os << "     tree node tests: " << setw (16) << commify (tnc)
       << " (" << setw(2) << (100 * tnc / (sc * tstats.num_nodes)) << "%)" << endl;
  if (tstats.num_surfaces != 0)
    os << "     surface tests:   " << setw (16) << commify (ocic)
       << " (" << setw(2) << (100 * ocic / (sc * tstats.num_surfaces)) << "%)" << endl;

  long long sst = scene_shadow_tests;

  if (sst != 0)
    {
      long long shh = shadow_hint_hits;
      long long shm = shadow_hint_misses;
      long long sss = scene_slow_shadow_traces;
      long long oss = surface_slow_shadow_traces;
      long long tnt = tistats2.node_intersect_calls;
      long long ot  = surface_intersects_tests;

      os << "  shadow:" << endl;
      os << "     rays:            " << setw (16) << commify (sst)
	 << endl;
      os << "     shadow hint hits:" << setw (16) << commify (shh)
	 << " (" << setw(2) << (100 * shh / sst) << "%)" << endl;
      os << "     shadow hint misses:" << setw (14) << commify (shm)
	 << " (" << setw(2) << (100 * shm / sst) << "%)" << endl;
      if (sss != 0)
	os << "     non-opaque traces: " << setw (14) << commify (sss)
	   << " (" << setw(2) << (100 * sss / sst) << "%"
	   << "; average depth = " << (float (oss) / float (sss)) << ")"
	   << endl;
      if (tstats.num_nodes != 0)
	os << "     tree node tests: " << setw (16) << commify (tnt)
	   << " (" <<setw(2) << (100 * tnt / (tstats.num_nodes * (sst - shh))) << "%)"
	   << endl;
      if (tstats.num_surfaces != 0)
	os << "     surface tests:   " << setw (16) << commify (ot)
	   << " (" <<setw(2) << (100 * ot / (tstats.num_surfaces * (sst - shh))) << "%)"
	   << endl;
    }

  long long ic = illum_calls;

  if (ic != 0)
    {
      long long is = illum_samples;

      os << "  illum:" << endl;
      os << "     illum calls:     " << setw (16)
	 << commify (ic) << endl;
      os << "     average light samples: " << setw (10)
	 << setprecision(3) << (float (is) / float (ic)) << endl;
      os << "     average shadow rays:   " << setw (10)
	 << setprecision(3) << (float (sst) / float (ic))
	 << " (" << setw(2) << (sst * 100 / is) << "%)"
	 << endl;
    }
}

// arch-tag: b884b170-54ff-4f69-a847-0997e0b0f347
