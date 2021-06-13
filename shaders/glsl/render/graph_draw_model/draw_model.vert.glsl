#version 450 core
#extension GL_GOOGLE_include_directive    : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf            : enable
#include "Nyx.h"

// Standard Ngg model vertex layout.
layout ( location = 0 ) in vec4  vertex     ; 
layout ( location = 1 ) in vec4  normals    ; 
layout ( location = 2 ) in vec4  weights    ;
layout ( location = 3 ) in uvec4 ids        ;
layout ( location = 4 ) in vec2  tex_coords ;

     layout( location = 0 ) out vec2  frag_coords    ;
flat layout( location = 1 ) out uvec2 texture_index  ;

NyxPushConstant push
{
  uint index     ;
  uint tex_index ;
};

layout( binding = 1 ) uniform projection
{
  mat4 viewproj ;
};

layout( binding = 2 ) buffer transform
{
  mat4 transforms[] ;
}; 

void main()
{
  mat4  model     ;
  vec4  position  ;
  vec4  normal    ;
  float weight    ;
  uint  id        ;

  normal         = normals      ;
  weight         = weights[ 0 ] ;
  id             = ids    [ 0 ] ;
  position       = vertex       ;

  model           = transforms[ index ] ;
  frag_coords     = tex_coords          ;
  texture_index.x = tex_index           ;

  gl_Position = viewproj * model * position ;
}