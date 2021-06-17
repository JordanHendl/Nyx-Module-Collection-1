#version 450 core
#extension GL_GOOGLE_include_directive    : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf            : enable
#include "Nyx.h"

layout( location = 0 ) out vec4 out_color ;
layout( location = 0 ) in  vec2 coords    ;

layout( binding = 0 ) uniform sampler2D tex ; 

void main()
{
  vec4 tmp ;

  tmp       = texture( tex, coords )                          ;
  out_color = nyxCompareFloat( tmp.a, 0.0 ) ? tmp : out_color ; 
}