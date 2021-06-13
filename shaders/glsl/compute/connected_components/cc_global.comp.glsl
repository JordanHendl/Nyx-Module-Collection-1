#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable
#include "Nyx.h"

#define BLOCK_SIZE_X 32 
#define BLOCK_SIZE_Y 32 
#define BLOCK_SIZE_Z 1 
#define NUM_COLORS   10
#define BLOCK_THREAD_COUNT BLOCK_SIZE_X*BLOCK_SIZE_Y*BLOCK_SIZE_Z

layout( local_size_x = BLOCK_SIZE_X, local_size_y = BLOCK_SIZE_Y, local_size_z = BLOCK_SIZE_Z ) in ; 

layout( binding = 0, rgba32f ) coherent restrict writeonly uniform image2D output_tex ;

layout( binding = 1 ) buffer index_map
{
  uint indices[] ;
};

NyxPushConstant push
{
  uint width ;
};

void write( uint ix, uint iy, vec4 color )
{
  imageStore( output_tex, ivec2( ix, iy ), color ) ;
}

void main()
{
  const uint  ix  = gl_GlobalInvocationID.x   ;
  const uint  iy  = gl_GlobalInvocationID.y   ;
  const uint  gid = ix + ( iy * width )       ;
  
  vec4 colors[ 10 ] ;
  uint position     ;
  uint root         ;
  
  colors[ 0 ] = vec4( 0.0, 0.0, 1.0, 1.0 ) ;
  colors[ 1 ] = vec4( 0.0, 1.0, 0.0, 1.0 ) ;
  colors[ 2 ] = vec4( 1.0, 0.0, 0.0, 1.0 ) ;
  colors[ 3 ] = vec4( 0.0, 0.5, 0.5, 1.0 ) ;
  colors[ 4 ] = vec4( 0.5, 0.0, 0.5, 1.0 ) ;
  colors[ 5 ] = vec4( 0.5, 0.5, 0.0, 1.0 ) ;
  colors[ 6 ] = vec4( 0.1, 0.3, 0.3, 1.0 ) ;
  colors[ 7 ] = vec4( 0.3, 0.1, 0.7, 1.0 ) ;
  colors[ 8 ] = vec4( 0.0, 0.0, 0.0, 1.0 ) ;
  colors[ 9 ] = vec4( 1.0, 1.0, 1.0, 1.0 ) ;
  
  position = gid                 ;
  root     = indices[ position ] ;
  
  while( position != indices[ root ] )
  {
    position = root            ;
    root     = indices[ root ] ;
  }
  
  indices[ gid ] = root ;
  
  write( ix, iy, colors[ root % NUM_COLORS ] ) ;
}
