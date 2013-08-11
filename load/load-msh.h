// load-msh.h -- Load a .msh format mesh file
//
//  Copyright (C) 2006-2007, 2010-2011  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//


#ifndef SNOGRAY_LOAD_MSH_H
#define SNOGRAY_LOAD_MSH_H


#include <string>


namespace snogray {

class Mesh;
class MaterialDict;
class ValTable;


// Load mesh from a .msh format mesh file into MESH.  Materials are
// filtered through MAT_DICT.
//
extern void load_msh_file (const std::string &filename, Mesh &mesh,
			   const ValTable &params);


}

#endif // SNOGRAY_LOAD_MSH_H
