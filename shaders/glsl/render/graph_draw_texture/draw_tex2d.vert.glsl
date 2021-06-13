#version 450
#extension GL_GOOGLE_include_directive    : enable
#extension GL_ARB_separate_shader_objects : enable
#include "Nyx.h"

// Standard Ngg model vertex layout.
layout ( location = 0 ) in vec4  vertex ;

     layout( location = 0 ) out vec2 frag_coords    ;
flat layout( location = 1 ) out uint texture_index  ;

layout( binding = 1 ) uniform projection
{
  mat4 proj ; 
};

layout( binding = 2 ) buffer transform
{
  mat4 transforms[] ;
};

layout( binding = 3 ) buffer texture_id
{
  uint texture_ids[] ;
};

void main()
{
  mat4 model          ;
  mat4 projection     ;
  vec4 position       ;
  vec2 tex            ;

  position      = vec4( vertex.x, vertex.y, 0.0, 1.0 ) ;
  model         = transforms [ gl_InstanceIndex ]      ;
  texture_index = texture_ids[ gl_InstanceIndex ]      ;
  projection    = proj                                 ;
  frag_coords   = vec2( vertex.z, vertex.w )           ;

  gl_Position = projection * model * position ;  
}