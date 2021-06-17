/*
 * Copyright (C) 2021 Jordan Hendl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   Nyx.h
 * Author: Jordan Hendl
 *
 * Created on January 28, 2021, 7:23 AM
 */

#ifndef NYXGLSL_H
#define NYXGLSL_H

#extension GL_EXT_scalar_block_layout                    : enable 
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable 

#define NyxPushConstant layout( push_constant ) uniform
#define nyx_seek( iter, pos  ) ( iter.position = pos < iter.position ? pos : iter.position ) ;
#define nyx_get(  buff, iter ) buff[ uint( iter.position ) ]
#define nyx_set(  buff, iter, value ) buff[ uint( iter.position ) ] = value

vec3 nyxGetPosition( mat4 transform )
{
  float x = transform[3].x ;
  float y = transform[3].y ;
  float z = transform[3].z ;
  
  return vec3( x, y, z ) ;
}

bool nyxCompareFloat( float a, float b )
{
  const float epsilon = 0.00001 ;
  
  return ( abs( a - b ) < epsilon ) ;
}

#endif

