#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) out vec4  out_color ;
layout( location = 0 ) in  vec2 coords ;
void main()
{
  vec2 tmp = coords ;
  out_color = vec4( 1.0, 0.0, 0.0, 1.0 ) ;
}