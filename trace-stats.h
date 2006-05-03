// trace-stats.h -- Print post-rendering statistics
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __TRACE_STATS_H__
#define __TRACE_STATS_H__

#include <ostream>

#include "space.h"

namespace Snogray {

class Scene;

struct TraceStats
{
  TraceStats () : scene_intersect_calls (0),
	     surface_intersect_calls (0),
	     scene_shadow_tests (0),
	     shadow_hint_hits (0), shadow_hint_misses (0),
	     scene_slow_shadow_traces (0), surface_slow_shadow_traces (0),
	     horizon_hint_hits (0), horizon_hint_misses (0),
	     surface_intersects_tests (0),
	     illum_calls (0), illum_samples (0)
  { }
  unsigned long long scene_intersect_calls;
  unsigned long long surface_intersect_calls;
  unsigned long long scene_shadow_tests;
  unsigned long long shadow_hint_hits;
  unsigned long long shadow_hint_misses;
  unsigned long long scene_slow_shadow_traces;
  unsigned long long surface_slow_shadow_traces;
  unsigned long long horizon_hint_hits;
  unsigned long long horizon_hint_misses;
  unsigned long long surface_intersects_tests;
  unsigned long long illum_calls;
  unsigned long long illum_samples;
  Space::IsecStats space_intersect;
  Space::IsecStats space_shadow;

  void print (std::ostream &os, const Scene &scene);
};

}

#endif /*__TRACE_STATS_H__ */

// arch-tag: b7800699-80ca-46da-9f30-732a78beb547
