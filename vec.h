// vec.h -- Vector datatype
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __VEC_H__
#define __VEC_H__

#include <fstream>
#include <iomanip>

#include "snogmath.h"
#include "tuple3.h"

namespace Snogray {

template<typename T>
class TVec : public Tuple3<T>
{
public:

  using Tuple3<T>::x;
  using Tuple3<T>::y;
  using Tuple3<T>::z;

  TVec (T _x = 0, T _y = 0, T _z = 0)
    : Tuple3<T> (_x, _y, _z)
  { }

  // Allow easy down-casting for sharing code
  //
  template<typename T2>
  TVec (const Tuple3<T2> &t) : Tuple3<T> (t) { }

  using Tuple3<T>::operator*=;
  using Tuple3<T>::operator/=;

  bool null () const { return x == 0 && y == 0 && z == 0; }

  TVec operator+ (const TVec &v2) const
  {
    return TVec (x + v2.x, y + v2.y, z + v2.z);
  }
  TVec operator- (const TVec &v2) const
  {
    return TVec (x - v2.x, y - v2.y, z - v2.z);
  }
  TVec operator- () const
  {
    return TVec (-x, -y, -z);
  }

  TVec operator* (T scale) const
  {
    return TVec (x * scale, y * scale, z * scale);
  }
  TVec operator/ (T denom) const
  {
    return operator* (1 / denom);
  }

  void operator+= (const TVec &v2)
  {
    x += v2.x; y += v2.y; z += v2.z;
  }
  void operator-= (const TVec &v2)
  {
    x -= v2.x; y -= v2.y; z -= v2.z;
  }

  TVec operator* (const Matrix4<T> &xform) const
  {
    return TVec (Tuple3<T>::operator* (xform));
  }
  const TVec &operator*= (const Matrix4<T> &xform)
  {
    TVec temp = *this * xform;
    *this = temp;
    return *this;
  }

  T length_squared () const
  {
    return x * x + y * y + z * z;
  }
  T length () const
  {
    return sqrt (x * x + y * y + z * z);
  }

  TVec unit () const
  {
    T len = length ();
    if (len == 0)
      return TVec (0, 0, 0);
    else
      return operator* (1 / len);
  }

  T latitude () const { return atan2 (y, sqrt (x * x + z * z)); }
  T longitude () const { return atan2 (x, z); }

  // Return this vector reflected around NORMAL
  //
  TVec reflection (const TVec &normal) const
  {
    // Rr =  Ri - 2 N (Ri . N)

    return  *this - normal * dot (*this, normal) * 2;
  }

  // Return this vector refracted through a medium transition across a
  // surface with normal NORMAL.  IOR_IN and IOR_OUT are the indices of
  // refraction for the incoming and outgoing media.
  //
  TVec refraction (const TVec &normal, T ior_in, T ior_out) const
  {
    // From:
    //
    //   Heckbert, Paul S., Pat Hanrahan, "Beam Tracing Polygonal Objects,",
    //   _Computer Graphics (SIGGRAPH '84 Proceedings)_, vol. 18, no. 3,
    //   July 1984, pp. 119-127.
    // 

    T ior_ratio = ior_in / ior_out;

    T c1 = -dot (*this, normal);
    T c2_sq = 1 - (ior_ratio * ior_ratio) * (1 - c1 * c1);

    if (c2_sq < -Eps)
      return TVec (0, 0, 0);	// Total internal reflection

    if (c2_sq < 0)
      c2_sq = 0;

    return (*this * ior_ratio) + (normal * (ior_ratio * c1 - sqrt (c2_sq)));
  }
};

template<typename T>
static inline T
dot (const TVec<T> &vec1, const TVec<T> &vec2)
{
  return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

template<typename T>
static inline TVec<T>
cross (const TVec<T> &vec1, const TVec<T> &vec2)
{
  return TVec<T> (vec1.y * vec2.z - vec1.z * vec2.y,
		  vec1.z * vec2.x - vec1.x * vec2.z,
		  vec1.x * vec2.y - vec1.y * vec2.x);
}

template<typename T>
static inline TVec<T>
operator* (T scale, const TVec<T> &vec)
{
  return vec * scale;
}

template<typename T>
static inline TVec<T> abs (const TVec<T> &vec)
{
  return TVec<T> (abs (vec.x), abs (vec.y), abs (vec.z));
}

template<typename T>
static std::ostream&
operator<< (std::ostream &os, const TVec<T> &vec)
{
  os << "vec<" << std::setprecision (5) << lim (vec.x)
     << ", " << std::setprecision (5) << lim (vec.y)
     << ", " << std::setprecision (5) << lim (vec.z)
     << ">";
  return os;
}

typedef TVec<dist_t>  Vec;
typedef TVec<sdist_t> SVec;

}

#endif /* __VEC_H__ */

// arch-tag: f86f6a3f-def9-477b-84a0-0935f0b76e9b
