#version 450 core
#extension GL_GOOGLE_include_directive    : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf            : enable
#include "Nyx.h"

// Standard Ngg model vertex layout.
layout ( location = 0 ) in vec4 vertex ;


     layout( location = 0 ) out vec2 frag_coords    ;
flat layout( location = 1 ) out uint texture_index  ;

NyxBuffer PositionData 
{
  mat4 model ; 
};

struct Sprite
{
  uint sprite_index  ;
  uint tex_index     ;
  uint sprite_width  ;
  uint sprite_height ;
  uint image_width   ;
  uint image_height  ;
};

NyxPushConstant push 
{
  PositionData transform_device_ptr     ;
  NyxIterator  const_transform_iterator ;
};

layout( binding = 1 ) uniform projection
{
  mat4 proj ; 
};

layout( binding = 2 ) uniform sprite
{
  Sprite sprites[ 1024 ] ;
} ; 

vec2 calculateCoordinates( uint sprite_width, uint sprite_height, uint image_width, uint image_height, uint sprite_index, uint coord_index )
{
  uint idx                = uint( coord_index )                              ;
  uint sprite             = sprite_index                                     ;
  uint num_sprites_in_row = image_width  / sprite_width                      ; 
  uint sprite_y_index     = sprite / num_sprites_in_row                      ;
  uint sprite_x_index     = sprite - ( sprite_y_index * num_sprites_in_row ) ;
  uint sprite_ypixel      = sprite_y_index * sprite_height                   ;
  uint sprite_xpixel      = sprite_x_index * sprite_width                    ;
  
  vec2 texarray[ 4 ] ;

  texarray[ 0 ].x = float( sprite_xpixel                 ) / float( image_width  ) ;
  texarray[ 0 ].y = float( sprite_ypixel + sprite_height ) / float( image_height ) ;
  texarray[ 1 ].x = float( sprite_xpixel + sprite_width  ) / float( image_width  ) ;
  texarray[ 1 ].y = float( sprite_ypixel                 ) / float( image_height ) ;
  texarray[ 2 ].x = float( sprite_xpixel                 ) / float( image_width  ) ;
  texarray[ 2 ].y = float( sprite_ypixel                 ) / float( image_height ) ;
  texarray[ 3 ].x = float( sprite_xpixel + sprite_width  ) / float( image_width  ) ;
  texarray[ 3 ].y = float( sprite_ypixel + sprite_height ) / float( image_height ) ;

  return texarray[ idx ] ;
}

void main()
{
  NyxIterator model_iterator  ;
  mat4        model           ;
  mat4        projection      ;
  vec4        position        ;
  vec2        tex             ;
  uint        sprite_index    ;
  uint        sprite_width    ;
  uint        sprite_height   ;
  uint        image_width     ;
  uint        image_height    ;

  model_iterator  = const_transform_iterator ;
  position = vec4( vertex.x, vertex.y, 0.0, 1.0 ) ;

  nyx_seek( model_iterator , gl_InstanceIndex ) ;

  model         = nyx_get( transform_device_ptr, model_iterator  ).model ;
  texture_index = sprites[ gl_InstanceIndex ].tex_index                  ;
  sprite_index  = sprites[ gl_InstanceIndex ].sprite_index               ;
  sprite_width  = sprites[ gl_InstanceIndex ].sprite_width               ;
  sprite_height = sprites[ gl_InstanceIndex ].sprite_height              ;
  image_width   = sprites[ gl_InstanceIndex ].image_width                ;
  image_height  = sprites[ gl_InstanceIndex ].image_height               ;
  projection    = proj                                                   ;

  frag_coords = calculateCoordinates( sprite_width, sprite_height, image_width, image_height, sprite_index, uint( vertex.z ) ) ;

  gl_Position = projection * model * position ;  
}