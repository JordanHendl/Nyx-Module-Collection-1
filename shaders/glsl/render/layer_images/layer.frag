#version 450 core
#extension GL_GOOGLE_include_directive    : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf            : enable
#include "Nyx.h"

layout( location = 0 ) out vec4 out_color ;
layout( location = 0 ) in  vec2 coords    ;

layout( binding = 0 ) uniform sampler2D textures[ 10 ] ; 

NyxBuffer TextureCount
{
  uint texture_count ;
};

NyxPushConstant push 
{
  TextureCount texture_ptr        ;
  NyxIterator  const_tex_iterator ;
};

void main()
{
  NyxIterator iterator ;
  uint        count    ;
  vec4        tmp      ;

  iterator = const_tex_iterator ;

  nyx_seek( iterator, 0 ) ;
  count = nyx_get( texture_ptr, iterator ).texture_count ;
  
  for( uint index = 0; index < count; index++ )
  {
    tmp       = texture( textures[ index ], coords )            ;
    out_color = nyxCompareFloat( tmp.a, 0.0 ) ? tmp : out_color ; 
  }
}