// tessel.cc -- Surface tessellation
//
//  Copyright (C) 2005, 2007, 2008, 2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

//
// The algorithms used here are from:
//
//   Velho, L., de Figueiredo, L. H., and Gomes, J. 1999,
//   "A unified approach for hierarchical adaptive tesselation of surfaces"
//   ACM Trans. Graph. 18, 4 (Oct. 1999), 329-360.
//   DOI=http://doi.acm.org/10.1145/337680.337717
//


#include "mesh.h"

#include "tessel.h"


using namespace snogray;


// A tessellation of FUN, where MAX_ERR_CALC is used to calculate the
// permissible error at a given location.
//
Tessel::Tessel (const Function &_fun, const MaxErrCalc &_max_err_calc)
  : fun (_fun),
    free_vertices (_fun.vertex_size ()),
    max_err_calc (_max_err_calc)
{
  fun.define_basis (*this);	// Define the rough basis of the shape

  structure ();			// Refine the basis

  // We're now finished with all subdivs, so free the memory they used.
  //
  free_subdivs.clear ();

  // Assign each vertex's index
  //
  unsigned index = 0;
  for (LinkedList<Vertex>::iterator vi = vertices.begin();
       vi != vertices.end(); vi++)
    vi->index = index++;
}


// Tessel::Function methods

// Tesselate this function and add the results to MESH, using
// _MAX_ERR_CALC to calculate the maximum allowable error.
//
void
Tessel::Function::tessellate (Mesh *mesh, const MaxErrCalc &_max_err_calc) const
{
  Tessel tes (*this, _max_err_calc);
  tes.add_to_mesh (mesh);
}


// MaxErr stuff

Tessel::MaxErrCalc::~MaxErrCalc () { }

Tessel::err_t
Tessel::ConstMaxErr::max_err (const Pos &) const
{
  return err;
}


// Retrieving results of tessellation

// Clear out VEC, and deallocate any memory it has reserved.
//
template<typename T>
void
zero_vec (std::vector<T> &vec)
{
  // We make sure VEC is deallocated by using the standard trick of
  // swapping its contents with an empty temp vector.
  //
  std::vector<T> empty;
  vec.swap (empty);
}


// Add the results of this tessellation to MESH.
//
void
Tessel::add_to_mesh (Mesh *mesh)
{
  //
  // Add vertices
  //

  std::vector<Mesh::MPos> mesh_verts;

  mesh_verts.reserve (vertices.size ());

  for (LinkedList<Vertex>::iterator vi = vertices.begin();
       vi != vertices.end(); vi++)
    mesh_verts.push_back (Mesh::MPos (vi->pos));

  Mesh::vert_index_t base_vert = mesh->add_vertices (mesh_verts);

  zero_vec (mesh_verts);	// reclaim memory used by MESH_VERTS

  //
  // Add triangles
  //

  std::vector<Mesh::vert_index_t> tri_vert_indices;

  tri_vert_indices.reserve (cells.size() * 3);

  for (LinkedList<Cell>::iterator ci = cells.begin(); ci != cells.end(); ci++)
    {
      tri_vert_indices.push_back (ci->e1->beg->index);
      tri_vert_indices.push_back (ci->e2->beg->index);
      tri_vert_indices.push_back (ci->e3->beg->index);
    }

  mesh->add_triangles (tri_vert_indices, base_vert);

  zero_vec (tri_vert_indices);	// reclaim memory used by TRI_VERT_INDICES

  //
  // Add normals
  //

  std::vector<Mesh::MVec> mesh_normals;

  // We know nothing about normals, so FUN must deal with them.
  //
  fun.get_vertex_normals (vertices.begin(), vertices.end(), mesh_normals);

  if (! mesh_normals.empty ())
    mesh->add_normals (mesh_normals, base_vert);

  //
  // Add UV values
  //

  std::vector<UV> mesh_uvs;

  // We know nothing about UV values, so FUN must deal with them.
  //
  fun.get_vertex_uvs (vertices.begin(), vertices.end(), mesh_uvs);

  if (! mesh_uvs.empty ())
    mesh->add_uvs (mesh_uvs, base_vert);
}


// Edge subdivision trees

Tessel::Subdiv::Subdiv (Vertex *mid, err_t corr,
			Subdiv *_bef_mid, Subdiv *_aft_mid, err_t _err)
  : curve_midpoint (mid), midpoint_correction (corr),
    bef_mid (_bef_mid), aft_mid (_aft_mid), err (_err)
{ }

// Allocate and return a new subdiv.
//
Tessel::Subdiv *
Tessel::add_subdiv (Vertex *mid, dist_t corr, Subdiv *bef, Subdiv *aft,
		    err_t err)
{
  return new (free_subdivs) Subdiv (mid, corr, bef, aft, err);
}

void
Tessel::remove_subdiv (Subdiv *subdiv)
{
  free_subdivs.put (subdiv);
}

// Build a subdivision tree to full resolution between VERT1 and VERT2.
//
Tessel::Subdiv *
Tessel::sample (const Vertex *vert1, const Vertex *vert2)
{
  const Pos &pos1 = vert1->pos;
  const Pos &pos2 = vert2->pos;

  dist_t samp_res = fun.sample_resolution (max_err (pos1));
  dist_t sep_sq = (pos2 - pos1).length_squared ();

  if (sep_sq > samp_res * samp_res)
    {
      Vertex *mid = fun.midpoint (*this, vert1, vert2);

      const Pos edge_mid = midpoint (pos1, pos2);
      dist_t corr = (mid->pos - edge_mid).length ();

      Subdiv *bef_mid = sample (vert1, mid);
      Subdiv *aft_mid = sample (mid, vert2);

      err_t err = corr;
      if (bef_mid && err < bef_mid->err)
	err = bef_mid->err;
      if (aft_mid && err < aft_mid->err)
	err = aft_mid->err;

      return add_subdiv (mid, corr, bef_mid, aft_mid, err);
    }
  else
    return 0;
}

// Prune the subdivision tree rooted at TREE, removing any levels that
// are beneath the error threshold.
//
void
Tessel::simplify (Subdiv *&tree)
{
  if (tree)
    {
      simplify (tree->bef_mid);
      simplify (tree->aft_mid);

      if (!tree->bef_mid && !tree->aft_mid
	  && tree->midpoint_correction < max_err (tree->curve_midpoint->pos))
	{
	  remove_vertex (tree->curve_midpoint);
	  remove_subdiv (tree);
	  tree = 0;
	}
    }
}
  
// Delete the subdiv tree TREE; if FREE_VERTICES is true, also free any
// vertices it references.
//
void
Tessel::prune (Subdiv *tree, bool free_vertices)
{
  if (tree)
    {
      prune (tree->bef_mid, free_vertices);
      prune (tree->aft_mid, free_vertices);

      if (free_vertices)
	remove_vertex (tree->curve_midpoint);

      remove_subdiv (tree);
    }
}

// Return a reversed version of SUBDIV
//
Tessel::Subdiv *
Tessel::reverse (const Subdiv *subdiv)
{
  if (subdiv)
    return add_subdiv (subdiv->curve_midpoint, subdiv->midpoint_correction,
		       reverse (subdiv->aft_mid), reverse (subdiv->bef_mid),
		       subdiv->err);
  else
    return 0;
}


// Edges

Tessel::Edge::Edge (const Vertex *vert1, const Vertex *vert2,
		    Subdiv *_subdiv, Subdiv *_rev_subdiv, err_t _err)
  : beg (vert1), end (vert2),
    subdiv (_subdiv), reverse_subdiv (_rev_subdiv),
    err (_err)
//   , length ((end->pos - beg->pos).length())
{ }

// Add a new edge to the edge list.
//
Tessel::Edge *
Tessel::add_edge (const Vertex *vert1, const Vertex *vert2,
		  Subdiv *subdiv, Subdiv *rev_subdiv, err_t err)
{
  return new (free_edges) Edge (vert1, vert2, subdiv, rev_subdiv, err);
}

// Remove an edge.
//
void
Tessel::remove_edge (Edge *edge)
{
  free_edges.put (edge);
}

// Returns the midpoint of this edge; the edge must be non-simple.
//
const Tessel::Vertex *
Tessel::Edge::midpoint () const
{
  return subdiv->curve_midpoint;
}


// Root edges

// Add and return a new root edge from VERT1 to VERT2.  A root edge is
// one which does not share subdiv structure with any previous edges.
//
Tessel::Edge *
Tessel::add_root_edge (const Vertex *vert1, const Vertex *vert2)
{
  // Make a full-resolution subdiv tree
  //
  Subdiv *subdiv = sample (vert1, vert2);

  err_t err = subdiv ? subdiv->err : 0;

  // Remove unnecessary branches in the subdiv tree
  //
  simplify (subdiv);

  return add_edge (vert1, vert2, subdiv, reverse (subdiv), err);
}

// Remove a root edge.  The only real difference from `remove_edge' is that
// we also free the subdiv trees.
//
void
Tessel::remove_root_edge (Edge *edge)
{
  prune (edge->subdiv, true);
  prune (edge->reverse_subdiv, false);

  remove_edge (edge);
}


// Derivative (non-root) edges

// Add and return a new edge which is the reverse of EDGE.
//
Tessel::Edge *
Tessel::add_reverse_edge (const Edge *edge)
{
  return add_edge (edge->end, edge->beg,
		   edge->reverse_subdiv, edge->subdiv,
		   edge->err);
}

// Add and return a new edge from EDGE's curve midpoint to its end
// (EDGE must not be a leaf edge).
//
Tessel::Edge *
Tessel::add_edge_after_midpoint (const Edge *edge)
{
  return add_edge (edge->subdiv->curve_midpoint, edge->end,
		   edge->subdiv->aft_mid, edge->reverse_subdiv->bef_mid,
		   (edge->subdiv->aft_mid ? edge->subdiv->aft_mid->err : 0));
}

// Add and return a new edge from EDGE's beginning to its curve midpoint
// (EDGE must not be a leaf edge).
//
Tessel::Edge *
Tessel::add_edge_before_midpoint (const Edge *edge)
{
  return add_edge (edge->beg, edge->subdiv->curve_midpoint,
		   edge->subdiv->bef_mid, edge->reverse_subdiv->aft_mid,
		   (edge->subdiv->bef_mid ? edge->subdiv->bef_mid->err : 0));
}


// Edge maps (for use of subclasses)

// Return an edge from VERT1 to VERT2, creating it if necessary.
//
// The vertex->edge mapping is only for the convenience of subclasses
// while defining the basis, and is not maintained at other times
// (e.g., during structuring).
//
Tessel::Edge *
Tessel::get_edge (const Vertex *vert1, const Vertex *vert2)
{
  VertexEdgeMap::key_type key (vert1, vert2);
  VertexEdgeMap::iterator prev_entry = edge_map.find (key);

  // This method is probably overly general -- a manifold edge should
  // have at most two uses of a given edge, and if the cells are defined
  // consistently, the two uses will be in opposite directions.  So we
  // could probably get away with simply automatically storing the
  // reverse of a new edge.  Maybe we should just signal an error if any
  // attempt is made to re-use an edge in the same direction?

  if (prev_entry != edge_map.end ())
    return prev_entry->second;
  else
    {
      VertexEdgeMap::key_type rkey (vert2, vert1);
      VertexEdgeMap::iterator prev_rentry = edge_map.find (rkey);
      
      if (prev_rentry != edge_map.end ())
	{
	  Edge *redge = prev_rentry->second;
	  Edge *edge = add_reverse_edge (redge);

	  edge_map.insert (prev_rentry, VertexEdgeMap::value_type(rkey, edge));

	  return edge;
	}
      else
	{
	  Edge *edge = add_root_edge (vert1, vert2);

	  edge_map.insert (prev_entry, VertexEdgeMap::value_type (key, edge));

	  return edge;
	}
    }
}


// Cells

// Add a new triangular cell with the given edges.
//
void
Tessel::add_cell (Edge *e1, Edge *e2, Edge *e3)
{
  Cell *cell = new Cell (e1, e2, e3);

  if (e1->beg->pos == e2->beg->pos
      || e2->beg->pos == e3->beg->pos
      || e1->beg->pos == e3->beg->pos
      || e1->end->pos == e2->end->pos
      || e2->end->pos == e3->end->pos
      || e1->end->pos == e3->end->pos
      || e1->end->pos != e2->beg->pos
      || e2->end->pos != e3->beg->pos
      || e3->end->pos != e1->beg->pos)
    abort ();

  cells.append (cell);
}

// "structure" CELL by recursively subdividing it into sub-cells; if CELL
// is subdivided, its contents are replaced by that of some (arbitrary)
// sub-cell.  Thus only leaf cells actually exist.
//
void
Tessel::structure (Cell &cell)
{
  // Try all possible subdivisions of CELL, and choose the best one

  // CELL's edges in a convenient format for iterating over; we want access
  // to the bef_mid and aft_mid of each edge too.
  //
  Edge *edge[3] = { cell.e1, cell.e2, cell.e3 };
  Edge *next[3] = { cell.e2, cell.e3, cell.e1 };
  Edge *prev[3] = { cell.e3, cell.e1, cell.e2 };

  // The root results of structuring:  we create new edges to split
  // the cell from each non-simple existing edge's midpoint, and see the
  // cost of each.  In cases where the bef_mid or aft_mid edge (relative to an
  // edge being split) is also non-simple, we split that too.
  //
  Edge *split[3];
  Edge *next_split[3];
  Edge *prev_split[3];

  // We rotate around the triangular cell, trying to split from each
  // edge, and calculate the cost of each resulting set of new edges.
  //
  int best = -1;
  err_t best_err = 0;
//   double best_aspect_ratio = 0;

  for (unsigned i = 0; i < 3; i++)
    if (! edge[i]->simple())
      {
	// This edge isn't simple, so try splitting the cell from its midpoint

	const Vertex *mid = edge[i]->midpoint ();

	// Rootly add a new edge splitting CELL from the middle of
	// edge[i] to the opposing vertex of CELL.
	//
	split[i] = add_root_edge (mid, next[i]->end);

	// Keep track of the maximum error in this split.
	//
	err_t err = split[i]->err;

	// If the adjacent edges are also non-simple, similarly add
	// root new edges to their midpoints, so that for a given
	// "solution", all edges of CELL will be reduced (this
	// apparently yields better results than simply splitting along
	// one edge and leaving further splits for handling
	// recursively).

	if (next[i]->simple ())
	  next_split[i] = 0;
	else
	  {
	    next_split[i] = add_root_edge (mid, next[i]->midpoint ());

	    if (next_split[i]->err > err)
	      err = next_split[i]->err;
	  }

	if (prev[i]->simple ())
	  prev_split[i] = 0;
	else
	  {
	    prev_split[i] = add_root_edge (mid, prev[i]->midpoint ());

	    if (prev_split[i]->err > err)
	      err = prev_split[i]->err;
	  }

	// XXX we should avoid calculating the cost in the case where only
	// one split is possible.

// 	// Calculate the the aspect ratio of the generated triangles to
// 	// use in our determination of the best split -- closer to 1 is
// 	// better.
// 	//
// 	dist_t height = split[i]->length ();
// 	dist_t base = edge[i]->length ();
// 	double aspect_ratio = height / base;

	// Update our record of which is the best solution so far.
	//
	// Our heuristic is:  if the error from one split is more than
	// twice as small as another, use the better one, otherwise, if
	// one of the aspect ratios is more than 10% closer to 1 than
	// the other, use that, otherwise choose base on the lowest
	// error (even if the difference is small).
	//
	if (best < 0
    // [1]:
	    || err < best_err)
    // [4]:
// 	    || aspect_ratio < best_aspect_ratio)
    // [3]:
// 	    || (err <= best_err && aspect_ratio <= best_aspect_ratio))
    // [2]:
// 	    || (err * 1.2 <= best_err) // was 2
// 	    || (best_err * 1.2 <= err && aspect_ratio * 1.1 <= best_aspect_ratio)
// 	    || err < best_err)
	  {
	    best = i;
	    best_err = err;
// 	    best_aspect_ratio = aspect_ratio;
	  }
      }

  // Now we choose the best split, and create new sub-cells from it;
  // other less optimal splits are deleted.
  //
  for (int i = 0; i < 3; i++)
    if (i == best)
      {
	// This is the best split, use it to make sub-cells.  Note that
	// we overwrite CELL with the 1st sub-cell (effectively deleting
	// CELL), and recursively structure it immediately.

	if (next_split[i])
	  {
	    // The next edge is also split, so we need to add two new
	    // cells to that half of the main split.

	    cell.set_edges (add_edge_after_midpoint (edge[i]),
			    add_edge_before_midpoint (next[i]),
			    add_reverse_edge (next_split[i]));

	    add_cell (next_split[i],
		      add_edge_after_midpoint (next[i]),
		      add_reverse_edge (split[i]));
	  }
	else
	  cell.set_edges (add_edge_after_midpoint (edge[i]),
			  next[i],
			  add_reverse_edge (split[i]));

	if (prev_split[i])
	  {
	    // The previous edge is also split, so we need to add two
	    // new cells to that half of the main split.

	    add_cell (split[i],
		      add_edge_before_midpoint (prev[i]),
		      add_reverse_edge (prev_split[i]));
	    add_cell (prev_split[i],
		      add_edge_after_midpoint (prev[i]),
		      add_edge_before_midpoint (edge[i]));
	  }
	else
	  add_cell (add_edge_before_midpoint (edge[i]),
		    split[i],
		    prev[i]);

	// As we have replaced CELL's edges, clean up the old ones
	//
	remove_edge (edge[i]);
	if (next_split[i])
	  remove_edge (next[i]);
	if (prev_split[i])
	  remove_edge (prev[i]);

	// CELL has been replaced by some subcell, so recursively structure
	// it (the caller will take care of any newly added cells, but we
	// must handle this one).
	//
	structure (cell);
      }
    else if (! edge[i]->simple ())
      {
	// We tried to split this edge, but it didn't work out, so undo
	// our work.

	remove_root_edge (split[i]);

	if (next_split[i])
	  remove_root_edge (next_split[i]);
	if (prev_split[i])
	  remove_root_edge (prev_split[i]);
      }
}

// "structure" all cells by recursively subdividing them into sub-cells.
// Note that the number of cells may grow during structuring; any newly
// added cells will also be handled.
//
void
Tessel::structure ()
{
  for (LinkedList<Cell>::iterator ci = cells.begin(); ci != cells.end(); ci++)
    structure (*ci);
}

// arch-tag: dfedeed4-b347-43e7-8edb-50ffdce2a82f
