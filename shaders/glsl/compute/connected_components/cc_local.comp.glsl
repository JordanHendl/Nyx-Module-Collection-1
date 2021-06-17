#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive    : enable
#include "Nyx.h"

#define BLOCK_SIZE_X 32 
#define BLOCK_SIZE_Y 32 
#define BLOCK_SIZE_Z 1 
#define BLOCK_THREAD_COUNT BLOCK_SIZE_X*BLOCK_SIZE_Y*BLOCK_SIZE_Z

layout( local_size_x = BLOCK_SIZE_X, local_size_y = BLOCK_SIZE_Y, local_size_z = BLOCK_SIZE_Z ) in ; 

layout( binding = 0, rgba32f ) coherent restrict readonly uniform image2D input_tex ;

layout( binding = 1 ) buffer index_map
{
  uint indices[] ;
};

NyxPushConstant push
{
  uint width ;
};

shared uint slabel[ BLOCK_THREAD_COUNT ] ;
shared uint simg  [ BLOCK_THREAD_COUNT ] ;

uint find( uint id )
{
  while( id != slabel[ id ] )
  {
    id = slabel[ id ] ;
  }
  
  return id ;
}

void findAndUnion( uint id1, uint id2 )
{
  bool done ;
  uint p    ;
  uint q    ;
  
  done = false ;
  
  while( !done )
  {
    p = find( id1 ) ;
    q = find( id2 ) ;
    
    if( p < q )
    {
      atomicMin( slabel[ q ] , p ) ;
    }
    else if( q < p )
    {
      atomicMin( slabel[ p ] , q ) ;
    }
    else
    {
      done = true ;
    }
  }
}

void main()
{
  const uint  ix         = gl_GlobalInvocationID.x                                                                           ;
  const uint  iy         = gl_GlobalInvocationID.y                                                                           ;
  const uint  tx         = gl_LocalInvocationID.x                                                                            ;
  const uint  ty         = gl_LocalInvocationID.y                                                                            ;
  const ivec2 tex_coords = ivec2( ix, iy )                                                                                   ;
  const uint  pixel      = uint( imageLoad( input_tex, tex_coords ).x  )                                                     ;
  const uint  tid        = gl_LocalInvocationIndex                                                                           ;
  const uint  gid        = ix + ( iy * width )                                                                               ;
  const uint  offset     = ( gl_WorkGroupSize.x * gl_WorkGroupID.x ) + ( gl_WorkGroupID.y * ( width * gl_WorkGroupSize.y ) ) ;
  uint temp ;

  slabel[ tid ] = tid   ;
  simg  [ tid ] = pixel ;
  
  memoryBarrierShared() ;
  barrier() ;
 
  // Scan left member to see if labels match, and connect.
  if( tx > 0 )
  {
    if( simg[ tid ] == simg[ tid - 1 ] )
    {
      slabel[ tid ] = slabel[ tid - 1 ] ;
    }
  }

  memoryBarrierShared() ;
  barrier() ;

  // Scan top member to see if labels match, and connect.
  if( ty > 0 )
  {
    if( simg[ tid ] == simg[ tid - gl_WorkGroupSize.x ] )
    {
      slabel[ tid ] = slabel[ tid - gl_WorkGroupSize.x ] ;
    }
  }

  memoryBarrierShared() ;
  barrier() ;
  
  temp = tid ;
  
  // Find root
  while( temp != slabel[ temp ] )
  {
    temp = slabel[ temp ] ;
    slabel[ tid ] = temp ;
  }
  
  memoryBarrierShared() ;
  barrier() ;
  
  // Now, union-find the left neighbor.
  if( tx > 0 )
  {
    if( simg[ tid ] == simg[ tid - 1 ] )
    {
      findAndUnion( tid, tid - 1 ) ;
    }
  }
  
  memoryBarrierShared() ;
  barrier() ;
  
  // Now, union-find the top neighbor.
  if( ty > 0 )
  {
    if( simg[ tid ] == simg[ tid - gl_WorkGroupSize.y ] )
    {
      findAndUnion( tid, tid - gl_WorkGroupSize.y ) ;
    }
  }
  
  memoryBarrierShared() ;
  barrier() ;
  
  temp = tid ;
  while( temp != slabel[ temp ] )
  {
    temp = slabel[ temp ] ;
    slabel[ tid ] = temp ;
  }
  
  memoryBarrierShared() ;
  barrier() ;
  
  uint tmp = ( ( slabel[ tid ] >> 5 ) * width ) + ( slabel[ tid ] & 31 ) + offset ;
  indices[ gid ] = tmp ;
}
