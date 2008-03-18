-- snogray.lua -- Lua scene interface for snogray
--
--  Copyright (C) 2007, 2008  Miles Bader <miles@gnu.org>
--
-- This source code is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License as
-- published by the Free Software Foundation; either version 3, or (at
-- your option) any later version.  See the file COPYING for more details.
--
-- Written by Miles Bader <miles@gnu.org>
--


module ("snogray", package.seeall)

local raw = require "snograw"


----------------------------------------------------------------
--
-- A facility for adding hooks into the swig metatable for an object

-- Return a table attached to OBJ (a userdata object or table), which is
-- used as to satisfy __index queries on OBJ.  If such a table already
-- exists for OBJ, it is returned, otherwise a new one is added and
-- returned.  The added table is consulted before any previously
-- existing __index hook, and the previous hook called only when a
-- request is not found in the wrapper table.
--
local function index_wrappers (obj)
   local mt = getmetatable (obj)

   if not mt.__raw_index then
      mt.__raw_index = mt.__index

      -- Note that we can't just make the old __index function an
      -- __index for our new __index table, because the first argument
      -- passed to it would be the wrong thing (the metatable, not the
      -- underlying object).
      --
      mt.__index =
	 function (obj, key)
	    local mt = getmetatable (obj)
	    local rawi = mt.__raw_index
	    local wraps = mt.__index_wrappers
	    return (wraps and wraps[key]) or rawi (obj, key)
	 end
      mt.__index_wrappers = {}
   end

   return mt.__index_wrappers
end

local function has_index_wrappers (obj)
   return getmetatable(obj).__index_wrappers
end

-- Lookup KEY in the wrapped object OBJ without invoking any wrapper.
--
local function nowrap_index (obj, key)
   return getmetatable (obj).__raw_index (obj, key)
end

-- Call method METH in the wrapped object OBJ without invoking any wrapper.
--
local function nowrap_meth_call (obj, meth, ...)
   return getmetatable (obj).__raw_index (obj, meth) (obj, ...)
end


----------------------------------------------------------------
--
-- Swig type handling

-- Type name stripping for old-style mangled swig type names.
--
local function strip_old_swig_type (name)
   name = name:gsub (" const","")
   name = name:gsub (" ","")
   name = name:gsub ("_p_", "")
   name = name:gsub ("snogray__", "")
   name = name:gsub ("^RefT(.*)_t$", "%1")

   -- This is not reliable, because we can't really distinguish a "T" meaning
   -- "<" from a "T" which is part of a type name, but it should wort for the
   -- particular type names we use, because none of the types we use inside
   -- brackets (in templates) contain a real "T".
   --
   name = name:gsub ("T([^T]*)_t$", "<%1>")

   return name
end

-- Return the type name NAME with all extraneous junk removed.  
local function strip_swig_type (name)
   -- If the first character is an underscore, assume it's an old-style
   -- mangled swig type name.
   --
   if name:byte() == 95 then -- 95 is '_'
      return strip_old_swig_type (name)
   end

   name = name:gsub ("snogray::","")
   name = name:gsub (" const","")
   name = name:gsub (" ","")
   name = name:gsub ("[*]*$","")
   name = name:gsub ("^Ref<(.*)>$", "%1")

   return name
end

local swig_type_names = {}

local function nice_type (obj)
   local type = swig_type (obj) or type (obj)
   local best = swig_type_names[type]
   if not best then
      for comp in type:gmatch ("[^|]+") do
	 comp = strip_swig_type (comp)
	 if not best or #comp < #best then
	    best = comp
	 end
      end
      swig_type_names[type] = best
   end
   return best
end


----------------------------------------------------------------

-- Return a table containing every key in KEYS as a key, with value true.
--
local function set (keys)
   local s = {}
   for i,v in ipairs (keys) do
      s[v] = true
   end
   return s
end


----------------------------------------------------------------
--
-- Vector/position/bounding-box manipulation

pos = raw.Pos
vec = raw.Vec
bbox = raw.BBox

-- Handy scene origin position.
--
origin = pos (0, 0, 0)

midpoint = raw.midpoint
dot = raw.dot
cross = raw.cross


----------------------------------------------------------------
--
-- colors

local colors = {}

function define_color (name, val)
   colors[name] = color (val)
end

function is_color (val)
   return nice_type (val) == 'Color'
end

local color_keys = set{
   'r', 'red', 'g', 'green', 'b', 'blue', 'grey', 'gray',
   'i', 'intens', 'intensity', 'bright', 'brightness',
   's', 'scale'
}

function is_color_spec (obj)
   local ot = type (obj)
   if ot == 'number' or is_color (obj) or (ot == 'string' and colors[obj]) then
      return true
   elseif ot ~= 'table' then
      return false
   end
   
   for k,v in pairs (obj) do
      local kt = type (k)
      local vt = type (v)
      local inh = false
      if kt == 'number' then
	 if k == 1 and is_color_spec (v) then
	    inh = true
	 elseif k > 1 and inh then
	    return false
	 elseif k > 3 or vt ~= 'number' then
	    return false
	 end
      elseif not color_keys[k] or vt ~= 'number' then
	 return false
      end
   end

   return true
end

function color (val, ...)
   if is_color (val) then
      return val
   else
      local t = type (val)

      if t == "number" then
	 return raw.Color (val, ...)
      elseif t == "string" then
	 return colors[val] or error ("unknown color name: "..val, 2)
      elseif t == "table" then
	 local r,g,b

	 if not next (val) then
	    return white      -- default to white if _nothing_ specified
	 end

	 if type (val[1]) == "number" then
	    if #val == 1 then
	       r, g, b = val[1], val[1], val[1]
	    else
	       r, g, b = val[1], val[2], val[3]
	    end
	 elseif val[1] then
	    local inherit = color (val[1])
	    r, g, b = inherit:r(), inherit:g(), inherit:b()
	 end

	 local grey = val.grey or val.gray
	 r = val.red or val.r or grey or r or 0
	 g = val.green or val.g or grey or g or 0
	 b = val.blue or val.b or grey or b or 0

	 local intens =
	    val.intensity
	    or val.intens
	    or val.i
	    or val.brightness
	    or val.bright

	 if intens then
	    local max = math.max (r,g,b)
	    if max > 0 then
	       local scale = intens / max
	       r = r * scale
	       g = g * scale
	       b = b * scale
	    end
	 end

	 local scale = val.scale or val.s
	 if scale then
	    r = r * scale
	    g = g * scale
	    b = b * scale
	 end

	 return raw.Color (r, g, b)
	 
      else
	 error ("invalid color specification: "..tostring(val), 2)
      end
   end
end

function grey (level)
   return raw.Color (level)
end
gray = grey

white = grey (1)
black = grey (0)

define_color ("white",	white)
define_color ("black",	black)
define_color ("red",	{red=1})
define_color ("green",	{green=1})
define_color ("blue",	{blue=1})
define_color ("cyan",	{blue=1, green=1})
define_color ("magenta",{blue=1, red=1})
define_color ("yellow",	{red=1, green=1})


----------------------------------------------------------------
--
-- Basic texture support

function is_float_tex (val)
   return nice_type (val) == 'Tex<float>'
end

function is_color_tex (val)
   return nice_type (val) == 'Tex<Color>'
end

-- Return VAL, which should either be a color or a color texture, boxed
-- into a TexVal<Color> container.
--
local function color_tex_val (val)
   if is_float_tex (val) then
      val = raw.grey_tex (raw.FloatTexVal (val))
   elseif is_color_spec (val) then
      val = color (val)
   end
   return raw.ColorTexVal (val)
end

-- Return TEX, which should be a texture, converted into a
-- floating-poing texture if it isn't one already.
--
local function float_tex (tex)
   if is_color_tex (tex) then
      tex = raw.intens_tex (raw.ColorTexVal (tex))
   end
   return tex
end

-- Return VAL, which should either be a number or a float texture, boxed
-- into a TexVal<float> container.
--
local function float_tex_val (val)
   if is_color_tex (val) then
      val = raw.intens_tex (raw.ColorTexVal (val))
   end
   return raw.FloatTexVal (val)
end

-- Return VAL boxed into either a TexVal<Color> or a TexVal<float>
-- container, whichever is appropriate.
--
local function tex_val (tex)
   if is_float_tex (tex) then
      return raw.FloatTexVal (tex)
   elseif is_color_tex (tex) then
      return raw.ColorTexVal (tex)
   elseif type (tex) == 'number' then
      return raw.FloatTexVal (tex)
   else
      return raw.ColorTexVal (color (tex))
   end
end

-- Return VAL1 and VAL2 boxed into a pair of either TexVal<Color> or
-- TexVal<float> containers, whichever are appropriate.  Both VAL1 and
-- VAL2 are examined to make the decision; a mixture of Color and float
-- values results in the floating value being automatically converted to
-- a Color value to match.
--
local function tex_vals (val1, val2)
   if is_color_tex (val1) or is_color_tex (val2)
      or (type(val1) ~= 'number' and is_color_spec (val1))
      or (type(val2) ~= 'number' and is_color_spec (val2))
   then
      return color_tex_val (val1), color_tex_val (val2)
   else
      return float_tex_val (val1), float_tex_val (val2)
   end
end


----------------------------------------------------------------
--
-- materials

function is_material (val)
   return nice_type (val) == 'Material'
end

function is_ior (val)
   return nice_type (val) == "Ior"
end

function is_ior_spec (obj)
   local ot = type (obj)
   if is_ior (obj) or ot == 'number' then
      return true
   elseif ot == 'table' then
      for k,v in pairs (obj) do
	 if type (v) ~= 'number' then
	    return false
	 elseif k ~= 1 and k ~= 2 and k ~= 'n' and k ~= 'k' then
	    return false
	 end
      end
      return true
   else
      return false
   end
end

-- Index of Refraction:  { n = REAL_IOR, k = IMAG_IOR }
--
function ior (n, k)
   if (type (n) == "number" or  is_ior (n)) and not k then
      return n
   elseif n and k then
      return raw.Ior (n, k)
   else
      local params = n
      return raw.Ior (params.n or params[1], params.k or params[2])
   end
end

-- Do common material post-processing to the material MAT, using
-- parameters from the table PARAMS, and return MAT.  If PARAMS is not a
-- table, it is ignored.
--
local function postproc_material (mat, params)
   if type (params) == 'table' then
      local bump = params.bump_map or params.bump
      if bump then mat.bump_map = float_tex (bump) end
   end
   return mat
end   

-- Lambertian material:  { diffuse|color = 
--
function lambert (params)
   local diff
   if is_color_spec (params) or is_color_tex (params) then
      diff = params
   else
      diff = params.diffuse or params.color or params[1] or 1
   end
   diff = color_tex_val (diff)
   return postproc_material (raw.lambert (diff), params)
end

function cook_torrance (params)
   local diff, spec, m, i

   if is_color_spec (params) or is_color_tex (params) then
      diff = params
      spec = white
      m = 0.1
      i = 1.5
   else
       diff = params.diffuse or params.diff or params.d
	 or params.color or params[1] or 1
       spec = params.specular or params.spec or params.s
	 or params[2] or 1
       m = params.m or params[3] or 1
       i = ior (params.ior or params[4] or 1.5)
   end

   diff = color_tex_val (diff)
   spec = color_tex_val (spec)
   m = float_tex_val (m)

   return postproc_material (raw.cook_torrance (diff, spec, m, i), params)
end

local default_mirror_ior = ior (0.25, 3)

-- Return a mirror material.
-- PARAMS can be:
--   REFLECTANCE
--   {ior=IOR, reflect=REFLECTANCE, color=COLOR}
--   {REFLECTANCE, ior=IOR, color=COLOR}
-- etc
--
function mirror (params)
   local _ior = default_mirror_ior
   local _reflect = white
   local _col = black
   local _under

   if is_color_spec (params) or is_color_tex (params) then
      _reflect = params
   elseif is_ior_spec (params) then
      _ior = params
   elseif params then
      _reflect = params.reflect or params.reflectance or params.refl or params[1] or _reflect
      _ior = params.ior or params[2] or _ior
      _col = params.color or params[3] or _col
      _under = params.underlying or params.under or params[4]
   end

   if not _under then
      _under = color_tex_val (_col)
   end

   _ior = ior (_ior)
   _reflect = color_tex_val (_reflect)

   return postproc_material (raw.mirror (_ior, _reflect, _under), params)
end

-- Return a glass material.
-- PARAMS can be:
--   IOR
--   {ior=IOR, absorb=ABSORPTION}
--   {ABSORPTION, ior=IOR}
-- etc
--
function glass (params)
   local _ior = 1.5
   local _absorb = black

   if type (params) == "number" then
      _ior = params
   elseif type (params) == "table"
      and (params.ior or params.absorb or params.absorption)
   then
      _ior = params.ior or params[1] or _ior
      _absorb = params.absorb or params.absorption or params[2] or _absorb
   elseif type (params) == "table" and (params.n or params.k) then
      _ior = params
   else
      _absorb = params
   end

   _ior = ior (_ior)
   _absorb = color (_absorb)

   return postproc_material (raw.glass (raw.Medium (_ior, _absorb)), params)
end

function glow (col)
   return raw.glow (color (col))
end   

function norm_glow (intens)
   return raw.norm_glow (intens or 1)
end   


----------------------------------------------------------------
--
-- material dicts

function is_material_dict (val)
   return nice_type (val) == 'MaterialDict'
end

function material_dict (init)
   local mdict = raw.MaterialDict ()

   if init then
      if type (init) == "table" then
	 for name, mat in pairs (init) do
	    if type (name) == "boolean" then
	       mdict:set_default (mat)
	    else
	       mdict[name] = mat
	    end
	 end
      elseif is_material (init) then
	 mdict:set_default (init)
      end
   end

   return mdict
end


----------------------------------------------------------------
--
-- transforms

-- Make a transform.
--
xform = raw.Xform

identity_xform = raw.Xform_identity

function is_xform (val)
   return nice_type (val) == 'Xform'
end

-- Various transform constructors.
--
scale = raw.Xform_scaling
translate = raw.Xform_translation
rotate = raw.Xform_rotation
rotate_x = raw.Xform_x_rotation
rotate_y = raw.Xform_y_rotation
rotate_z = raw.Xform_z_rotation

-- ... and some abbreviations for them (a bit silly, but composed
-- transforms can get rather long...).
--
trans = translate
rot = rotate
rot_x = rotate_x
rot_y = rotate_y
rot_z = rotate_z

-- Transform which converts the z-axis to the y-axis; this is useful
-- because many scene files are set up that way.
--
xform_z_to_y = rotate_x (-math.pi / 2)
xform_y_to_z = xform_z_to_y:inverse ()

-- Transform which converts the x-axis to the y-axis.
--
xform_x_to_y = rotate_z (-math.pi / 2)
xform_y_to_x = xform_x_to_y:inverse ()


-- Transform which inverts the z-axis (as many models use a different
-- convention for the z-axis).
--
xform_flip_x = scale (-1, 1, 1)
xform_flip_y = scale (1, -1, 1)
xform_flip_z = scale (1, 1, -1)


----------------------------------------------------------------
--
-- GC protection
--
-- Swig's handling of garbage collection trips us up in various cases:
-- objects stored in the scene get GCed because the Lua garbage
-- collector doesn't know that there's a reference from one userdata
-- object to another.
--
-- To prevent this, we keep a table in Lua of external object
-- references, in a form that the garbage collector can follow.
--

local gc_refs = {}

-- Make the keys in GC_REFS weak so that entries don't prevent referring
-- objects from being garbage collected (if nobody else refers to them).
--
setmetatable (gc_refs, { __mode = 'k' })

-- Add a reference from FROM to TO for the garbage-collector to follow.
-- This is for adding references that the normal gc mechanism cannot
-- deduce by itself, e.g. in userdata objects.
--
function gc_ref (from, to)
   local refs = gc_refs[from]
   if refs then
      refs[#refs + 1] = to
   else
      gc_refs[from] = { to }
   end
end


----------------------------------------------------------------
--
-- scene object
--
-- We don't use the raw scene object directly because we need to
-- gc-protect objects handed to the scene.

local function init_scene (raw_scene)
   scene = raw_scene		-- this is exported

   if not has_index_wrappers (scene) then
      local wrap = index_wrappers (scene)

      function wrap:add (thing)
	 gc_ref (self, thing)
	 return nowrap_meth_call (self, "add", thing)
      end
   end
end


----------------------------------------------------------------
--
-- meshes

mesh = raw.Mesh

mesh_vertex_group = raw.mesh_vertex_group
mesh_vertex_normal_group = raw.mesh_vertex_normal_group

-- Return a transform which will warp SURF to be in a 2x2x2 box centered
-- at the origin.  Only a single scale factor is used for all
-- dimensions, so that a transformed object isn't distorted, merely
-- resized/translated.
--
function normalize_xform (surf)
   local bbox = surf:bbox ()
   local center = midpoint (bbox.max, bbox.min)
   local max_size = bbox:max_size ()

   return translate (-center.x, -center.y, -center.z) * scale (2 / max_size)
end

-- Return a transform which will warp SURF to be in a 2x2x2 box centered
-- at the origin in the x and z dimensions, but with a minimum y value
-- of zero (so it has a "zero y base").  Only a single scale factor is
-- used for all dimensions, so that a transformed object isn't
-- distorted, merely resized/translated.
--
function y_base_normalize_xform (surf)
   local bbox = surf:bbox ()
   local center = midpoint (bbox.max, bbox.min)
   local size = bbox.max - bbox.min
   local max_size = bbox:max_size ()

   return translate (-center.x, size.y / 2 - center.y, -center.z)
      * scale (2 / max_size)
end

-- Resize a mesh to fit in a 1x1x1 box, centered at the origin (but with
-- the bottom at y=0).  Returns MESH.
--
function normalize (mesh, xf)
   local norm = y_base_normalize_xform (mesh)
   if xf then norm = norm * xf end
   mesh:transform (norm)
   return mesh
end


----------------------------------------------------------------
--
-- Misc surface types

sphere = raw.Sphere
sphere2 = raw.Sphere2

tripar = raw.Tripar

function triangle (mat, v0, e1, e2)
   return tripar (mat, v0, e1, e2, false)
end

function rectangle (mat, v0, e1, e2)
   return tripar (mat, v0, e1, e2, true)
end

-- Return an elliptical surface.
--
-- args: MAT, XFORM
--   or: MAT, BASE, AXIS, RADIUS
--
ellipse = raw.Ellipse

-- Return a cylindrical surface (with no ends).
--
-- args: MAT, XFORM [, END_MAT1 [, END_MAT2]]
--   or: MAT, BASE, AXIS, RADIUS [, END_MAT1 [, END_MAT2]]
--
cylinder = raw.Cylinder

-- solid_cylinder is just like cylinder, but has endcaps as well.
--
-- Optionally, specific materials can be specified for the ends by at
-- the end of the argument list.
--
-- args: MAT, XFORM [, END_MAT1 [, END_MAT2]]
--   or: MAT, BASE, AXIS, RADIUS [, END_MAT1 [, END_MAT2]]
--
function solid_cylinder (mat, arg1, ...)

   -- There are two argument conventions for cylinders, which we handle
   -- separately.
   --
   if is_xform (arg1) then -- args: MAT, XFORM [, END_MAT1 [, END_MAT2]]
      local xform = arg1
      local emat1, emat2 = select (1, ...), select (2, ...)

      local base = pos(0,0,-1)*xform
      local axis = vec(0,0,2)*xform
      local r1 = vec(1,0,0)*xform
      local r2 = vec(0,1,0)*xform

      return surface_group {
	 cylinder (mat, xform);
	 ellipse (emat1 or mat, base, r1, r2);
	 ellipse (emat2 or emat1 or mat, base + axis, r1, r2);
      }
   else	      -- args: MAT, BASE, AXIS, RADIUS [, END_MAT1 [, END_MAT2]]
      local base = arg1
      local axis = select (1, ...)
      local radius = select (2, ...)
      local emat1, emat2 = select (3, ...), select (4, ...)

      local au = axis:unit()
      local r1u = au:perpendicular()
      local r1 = r1u * radius
      local r2 = cross (r1u, au) * radius

      return surface_group {
	 cylinder (mat, base, axis, radius);
	 ellipse (emat1 or mat, base, r1, r2);
	 ellipse (emat2 or emat1 or mat, base + axis, r1, r2);
      }
   end
end

-- Wrap the subspace constructor to record the GC link between a
-- subspace and the surface in it.
--
function subspace (surf)

   -- If SURF is actually a table, make a surface-group to hold its
   -- members, and wrap that instead.
   --
   if type (surf) == "table" then
      if #surf == 1 then
	 surf = surf[1]
      else
	 surf = surface_group (surf)
      end
   end

   local ss = raw.Subspace (surf)

   -- Record the GC link between SS and SURF.
   --
   gc_ref (ss, surf)

   return ss
end

-- Wrap the instance constructor to record the GC link between an
-- instance and its subspace.
--
function instance (subspace, xform)
   local inst = raw.Instance (subspace, xform)
   gc_ref (inst, subspace)
   return inst
end

-- Wrap the surface_group constructor to add some method wrappers to it,
-- and support adding a table of surfaces as well.
--
function surface_group (surfs)
   local group = raw.SurfaceGroup ()

   -- Initialize wrapper functions if necessary
   --
   if not has_index_wrappers (group) then
      local wrap = index_wrappers (group)

      -- Augment raw add method to (1) record the link between a group
      -- and the surfaces in it so GC can see it, and (2) support adding
      -- a table of surfaces all at once.
      --
      function wrap:add (surf)
	 if (type (surf) == "table") then
	    for k,v in pairs (surf) do
	       self:add (v)
	    end
	 else
	    gc_ref (self, surf)
	    nowrap_meth_call (self, "add", surf)
	 end
      end
   end

   if surfs then
      group:add (surfs)
   end

   return group
end


----------------------------------------------------------------
--
-- Lights

function point_light (pos, intens)
   return raw.PointLight (pos, color (intens))
end

function sphere_light (pos, radius, intens)
   return raw.SphereLight (pos, radius, color (intens))
end

function rect_light (corner, side1, side2, intens)
   return raw.RectLight (corner, side1, side2, color (intens))
end


----------------------------------------------------------------
--
-- Images

image = raw.image


----------------------------------------------------------------
--
-- Miscellaneous texture sources and operators

-- Image textures (read from a file)
--
image_tex = raw.image_tex
mono_image_tex = raw.mono_image_tex

-- Return a "grey_tex" texture object using the floating-point texture
-- VAL as a source.  This can be used to convert a floating-point
-- texture into a color texture.
--
function grey_tex (val) return raw.grey_tex (float_tex_val (val)) end

-- Return a "intens_tex" texture object using the color texture VAL as a
-- source.  This can be used to convert a color-point texture into a
-- floating-point texture.
--
function intens_tex (val) return raw.intens_tex (color_tex_val (val)) end

-- Return a "check" texture, which evaluates to either TEX1 or TEX2 in a
-- check pattern.
--
function check_tex (tex1, tex2)
   return raw.check_tex (tex_vals (tex1, tex2))
end
function check3d_tex (tex1, tex2)
   return raw.check3d_tex (tex_vals (tex1, tex2))
end

-- Return an interpolation texture, which interpolates between two
-- textures according to the value of its control parameter.
--
function linterp_tex (control, val1, val2)
   return raw.linterp_tex (float_tex_val (control), tex_vals (val1, val2))
end
function sinterp_tex (control, val1, val2)
   return raw.sinterp_tex (float_tex_val (control), tex_vals (val1, val2))
end

plane_map_tex = raw.plane_map_tex
cylinder_map_tex = raw.cylinder_map_tex
lat_long_map_tex = raw.lat_long_map_tex

-- A cache of "singleton" texture sources, whose instances have no
-- state, and really only one shared instance is needed.
--
local singleton_tex_cache = {}
local function singleton_tex_fun (name, create)
   return function ()
	     local inst = singleton_tex_cache[name]
	     if not inst then
		inst = create ()
		singleton_tex_cache[name] = inst
	     end
	     return inst
	  end
end

sin_tex = singleton_tex_fun ('sin', raw.sin_tex)
tri_tex = singleton_tex_fun ('tri', raw.tri_tex)

perlin_tex = singleton_tex_fun ('perlin', raw.perlin_tex)


----------------------------------------------------------------
--
-- Texture transformations

-- Return a texture which transforms TEX by the transform XFORM.
--
-- Actually it's the texture coordinates which are transformed (before
-- giving them to TEX), so for instance, to make TEX get "smaller", you
-- would use a value of XFORM which scales by an amount greater than 1.
--
function xform_tex (xform, tex)
   return raw.xform_tex (xform, tex_val (tex))
end

-- Convenience functions for various sorts of texture transformations.
--
function scale_tex (amount, tex) return xform_tex (scale (amount), tex) end
function rotate_tex (amount, tex) return xform_tex (rotate (amount), tex) end
rot_tex = rotate_tex


----------------------------------------------------------------
--
-- Texture arithmetic

-- Encoding for arith_tex operations.
--
local arith_tex_ops = {
   ADD = 0, SUB = 1, MUL = 2, DIV = 3, MOD = 4, POW = 5,
   FLOOR = 6, CEIL = 7, TRUNC = 8, -- floor/ceil/trunc (X / Y) * Y
   MIN = 9, MAX = 10, AVG = 11,
   MIRROR = 12,			   -- abs (X - Y)
   SIN = 13, COS = 14, TAN = 15,   -- sin/cos/tan (X * 2 * PI / Y)
   ATAN2 = 16
}

-- Return a texture which performs operation OP on input textures ARG1
-- and ARG2.  Both color and floating-point textures are handled (a
-- mixture of both results in the floating-point texture being converted
-- to color before applying the operation).
--
function arith_tex (op, arg1, arg2)
   op = arith_tex_ops[op]
   return raw.arith_tex (op, tex_vals (arg1, arg2))
end

-- Alias for the arith_tex MUL operation.  This function treats the
-- second operand specially because it is used to overload the "*"
-- operator for textures, which we want to work for texture-xform
-- operations too.
--
function mul_tex (tex1, tex2_or_xform)
   if is_xform (tex2_or_xform) then
      return xform_tex (tex2_or_xform, tex1)
   else
      return arith_tex ('MUL', tex1, tex2_or_xform)
   end
end

-- Convenient aliases for the various other arith_tex operations.
--
function add_tex (...) return arith_tex ('ADD', ...) end
function sub_tex (...) return arith_tex ('SUB', ...) end
function div_tex (...) return arith_tex ('DIV', ...) end
function mod_tex (...) return arith_tex ('MOD', ...) end
function pow_tex (...) return arith_tex ('POW', ...) end
function floor_tex (x, y) return arith_tex ('FLOOR', x, y or 1) end
function ceil_tex (x, y) return arith_tex ('CEIL', x, y or 1) end
function trunc_tex (x, y) return arith_tex ('TRUNC', x, y or 1) end
function min_tex (...) return arith_tex ('MIN', ...) end
function max_tex (...) return arith_tex ('MAX', ...) end
function avg_tex (...) return arith_tex ('AVG', ...) end
function mirror_tex (...) return arith_tex ('MIRROR', ...) end
function abs_tex (tex) return arith_tex ('MIRROR', tex, 0) end
function neg_tex (tex) return arith_tex ('SUB', 0, tex) end
function sin_tex (x, y) return arith_tex ('SIN', x, y or 2*math.pi) end
function cos_tex (x, y) return arith_tex ('COS', x, y or 2*math.pi) end
function tan_tex (x, y) return arith_tex ('TAN', x, y or 2*math.pi) end
function atan2_tex (...) return arith_tex ('ATAN2', ...) end

-- Install operator overloads for the texture metatable MT.
--
local function setup_tex_metatable (mt)
   mt.__add = add_tex
   mt.__sub = sub_tex
   mt.__mul = mul_tex
   mt.__div = div_tex
   mt.__unm = neg_tex
   mt.__pow = pow_tex
end

-- There's a metatable for each underlying texture datatype, currently
-- Color and float.  We install the same overload functions for both.
--
setup_tex_metatable (getmetatable (sin_tex ()))	  -- float
setup_tex_metatable (getmetatable (grey_tex (5))) -- Color


----------------------------------------------------------------
--
-- File handling

include_path = { "." }


function filename_dir (filename)
   return string.match (filename, "^(.*)/[^/]*$")
end
function filename_ext (filename)
   return string.match (filename, "[.]([^./]*)$")
end

function load_include (filename)
   local loaded, loaded_filename, err_msg

   if not filename_ext (filename) then
      filename = filename .. ".lua"
   end

   if string.sub (filename, 1, 1) == "/" then
      loaded_filename = filename
      loaded, err_msg = loadfile (filename)
   else
      -- First try the same directory as cur_filename.
      --
      local cur_dir = filename_dir (cur_filename)
      if cur_dir then
	 loaded_filename = cur_dir .. "/" .. filename
	 loaded, err_msg = loadfile (loaded_filename)
      end

      -- If we didn't find anything, try searching along include_path.
      --
      if not loaded then
	 local path_pos = 1
	 while not loaded and path_pos <= #include_path do
	    loaded_filename = include_path[path_pos] .. "/" .. filename
	    loaded, err_msg = loadfile (loaded_filename)
	    path_pos = path_pos + 1
	 end
      end
   end

   return loaded, loaded_filename, err_msg
end

function eval_include (loaded, fenv, loaded_filename, err_msg)
   if loaded then
      local old_cur_filename = cur_filename
      cur_filename = loaded_filename

      setfenv (loaded, fenv)
      loaded ()

      cur_filename = old_cur_filename
   else
      error (err_msg)
   end
end

-- Load FILENAME evaluated the current environment.  FILENAME is
-- searched for using the path in the "include_path" variable; while it
-- is being evaluating, the directory FILENAME was actually loaded from
-- is prepended to "include_path", so that any recursive includes may
-- come from the same directory.
--
function include (filename)
   local loaded, loaded_filename, err_msg = load_include (filename)
   local callers_env = getfenv (2)
   eval_include (loaded, callers_env, loaded_filename, err_msg)
   return loaded_filename
end


-- Map of filenames to the environment in which they were loaded.
--
local use_envs = {}

-- A metatable for inheriting from the snogray environment
--
local inherit_snogray_metatable = { __index = snogray }

-- A table containing entries for symbol names we _don't_ want inherited
-- because of a call to use().
--
local dont_inherit_syms = {}
dont_inherit_syms["_used_files"] = true
dont_inherit_syms["_used_symbols"] = true

-- Load FILENAME in a dedicated environment, and arrange for its
-- functions and constants to be inherited by the caller's environment.
-- FILENAME is only loaded if it hasn't already been loaded (if it has,
-- then the previously loaded file is re-used).
--
-- "Inheritance" is done by copying, so that it only works properly for
-- functions and constants.
--
-- The variable "_used_files" in the caller's environment is used to
-- record which use() environments have previously been added to its
-- inheritance set.  It maps loaded filenames to the environment in
-- which they were loaded.  The variable "_used_symbols" in the caller's
-- environment is where symbols to be inherited are copied; the caller's
-- environment then inherits from this.
--
function use (filename)
   local loaded, loaded_filename, err_msg = load_include (filename)

   local callers_env = getfenv (2)
   local used_files = callers_env._used_files

   if not used_files then
      used_files = {}
      callers_env._used_files = used_files
   end

   if not used_files[loaded_filename] then
      local use_env = use_envs[loaded_filename]

      -- If this file hasn't been loaded already, load it into a new
      -- environment.
      --
      if not use_env then
	 use_env = {}
	 use_envs[loaded_filename] = use_env

	 -- Make sure the loaded file inherits the snogray interface.
	 --
	 setmetatable (use_env, inherit_snogray_metatable)

	 eval_include (loaded, use_env, loaded_filename, err_msg)
      end

      -- Arrange for the symbols defined in USE_ENV to be inherited
      -- by the caller.  Because we need "multiple inheritance" (more
      -- than one call to use()), instead of directly using Lua
      -- inheritance, we instead copy the symbols defined by each
      -- use'd file into a table, and then use Lua inheritance to
      -- inherit from it.
      --
      -- [Note that we can't copy the symbols directly into the caller's
      -- environment because we don't want them to be seen by anybody
      -- that calls use() on our caller.]
      --
      local used_syms = callers_env._used_symbols
      if not used_syms then
	 used_syms = {}
	 callers_env._used_symbols = used_syms

	 -- Replace the caller's metatable with our own metatable
	 -- that inherits from USED_SYMS; thus USED_SYMS needs to
	 -- inherit in turn from the global snogray environment.
	 --
	 setmetatable (used_syms, inherit_snogray_metatable)
	 setmetatable (callers_env, {__index = used_syms}) 
      end

      -- Copy all symbols defined by the loaded file (in USE_ENV) into
      -- the caller's "inheritance set" (USED_SYMS), where they can be
      -- seen by the caller.  We avoid copying any symbols listed in
      -- dont_inherit_syms.
      --
      for k, v in pairs (use_env) do
	 if not dont_inherit_syms[k] then
	    used_syms[k] = v
	 end
      end

      -- Remember that the caller has loaded this file.
      --
      used_files[loaded_filename] = use_env
   end

   return loaded_filename
end


----------------------------------------------------------------
--
-- Autoloading

-- Add a stub for file-extension EXT to LOADER_TABLE that will load
-- LOADER_FILE and call the function named LOADER_NAME (which
-- LOADER_FILE must define); the stub will also install that function
-- into LOADER_TABLE so that it can be called directly for subsequent
-- files of the same type.
--
local function add_autoload_stub (loader_table, ext, loader_file, loader_name)
   loader_table[ext]
      = function (...)
	   print ("* autoloading: "..loader_file)

	   local contents, err = loadfile (loader_file)

	   if contents then
	      local environ = {}
	      setmetatable (environ, inherit_snogray_metatable)
	      setfenv (contents, environ)

	      contents ()	-- Finish loading

	      local loader = environ[loader_name]
	      if loader then
		 loader_table[ext] = loader
		 return loader (...)
	      else
		 error ("loading "..loader_file.." didn't define "..loader_name)
	      end
	   else
	      error (err, 0)
	   end
	end
end


----------------------------------------------------------------
--
-- Scene loading

-- Table of scene loaders for various file extensions.
--
local scene_loaders = {}


-- Load a scene from FILENAME into RSCENE and RCAMERA (the "raw" scene
-- and camera objects).
--
-- Return true for a successful load, false if FILENAME is not
-- recognized as loadable, or an error string if an error occured during
-- loading.
--
-- Note that this only handles formats loaded using Lua, not those
-- handled by the C++ core.  To load any supported format, use the
-- scene "load" method.
--
function load_scene (filename, fmt, rscene, rcamera, ...)
   local loader = scene_loaders[fmt]

   if loader then
      -- Set up the scene object for the loader code to use.
      --
      init_scene (rscene)

      -- Let users use the raw camera directly.
      --
      camera = rcamera

      -- Call the loader.
      --
      return loader (filename, scene, camera, ...)
   else
      return false
   end
end


----------------------------------------------------------------
--
-- Mesh loading

-- Table of mesh loaders for various file extensions.
--
local mesh_loaders = {}


-- Load a mesh from FILENAME into MESH.
--
-- Return true for a successful load, false if FILENAME is not
-- recognized as loadable, or an error string if an error occured during
-- loading.
--
-- Note that this only handles formats loaded using Lua, not those
-- handled by the C++ core.  To load any supported format, use the
-- mesh "load" method.
--
function load_mesh (filename, fmt, ...)
   local loader = mesh_loaders[fmt]
   if loader then
      return loader (filename, ...)
   else
      return false
   end
end


add_autoload_stub (mesh_loaders, "obj", "load-obj.lua", "load_obj")
add_autoload_stub (mesh_loaders, "ug", "load-ug.lua", "load_ug")
add_autoload_stub (mesh_loaders, "stl", "load-stl.lua", "load_stl")


----------------------------------------------------------------
--
-- Lua scene description loader

-- Load Lua scene description from FILENAME into RSCENE and RCAMERA.
-- Return true if the scene was loaded successfully, or an error string.
--
function scene_loaders.lua (filename, rscene, rcamera)

   -- Load the user's file!  This just constructs a function from the
   -- loaded file, but doesn't actually evaluate it.
   --
   local contents, err = loadfile (filename)

   if not contents then
      error (err, 0)		-- propagate the loading error
   end

   -- Make a new environment to evaluate the file contents in; it will
   -- inherit from "snogray" for convenience.  There are no global
   -- pointers to this table so it and its contents will be garbage
   -- collected after loading.
   --
   local environ = {}
   setmetatable (environ, inherit_snogray_metatable)
   setfenv (contents, environ)

   -- Remember filename being loaded, so we can find other files in
   -- the same location.
   --
   cur_filename = filename

   -- Finally, evaluate the loaded file!
   --
   contents ()

   return true
end

-- Other types of Lua file.
--
scene_loaders.luac = scene_loaders.lua
scene_loaders.luo = scene_loaders.lua

add_autoload_stub (scene_loaders, "nff", "load-nff.lua", "load_nff")


-- arch-tag: e5dc4da4-c3f0-45e7-a4a1-a20cb4db6d6b
