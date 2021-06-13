/*
 * Copyright (C) 2020 Jordan Hendl
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
 * File:   NyxDrawTex2D.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawTex2D.h"
#include "draw_tex2d.h"
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <Mars/TextureArray.h>
#include <unordered_map>
#include <queue>

static const unsigned VERSION = 1 ;
namespace nyx
{
  constexpr unsigned TRANSFORM_SIZE = 1024 ;
  using Framework = nyx::vkg::Vulkan ;
  
  const static glm::vec4 vertices[] = 
  { 
               // pos      // tex
    glm::vec4( 0.0f,  0.0f, 0.0f, 1.0f ),
    glm::vec4( 1.0f, -1.0f, 1.0f, 0.0f ),
    glm::vec4( 0.0f, -1.0f, 0.0f, 0.0f ), 

    glm::vec4( 0.0f,  0.0f, 0.0f, 1.0f ),
    glm::vec4( 1.0f,  0.0f, 1.0f, 1.0f ),
    glm::vec4( 1.0f, -1.0f, 1.0f, 0.0f )
  };
      
  static bool updated_textures = false ;
  
  struct NyxDrawTex2DData
  {
    nyx::Chain<Framework>             copy_chain ;
    nyx::Array<Framework, glm::mat4 > d_viewproj ;
    nyx::Array<Framework, unsigned  > d_indices  ;
    nyx::Array<Framework, glm::vec4 > d_vertices ;
    std::vector<unsigned>             indices    ;
    iris::Bus                         bus        ;
    bool                              dirty      ;
    const glm::mat4*                  projection ;
    const glm::mat4*                  camera     ;
    unsigned                          count      ;
    
    NyxDrawTex2DData()                                      { this->dirty = false ; this->projection = nullptr ; this->camera = nullptr ;         } ;
    void setProjectionInput( const char* input            ) { this->bus.enroll( this, &NyxDrawTex2DData::setProjection, iris::OPTIONAL, input ) ; } ;
    void setCameraInput    ( const char* input            ) { this->bus.enroll( this, &NyxDrawTex2DData::setCamera    , iris::OPTIONAL, input ) ; } ;
    void setProjection     ( const glm::mat4& val         ) { this->projection = &val ; this->dirty = true ;                                      } ;
    void setCamera         ( const glm::mat4& val         ) { this->camera     = &val ; this->dirty = true ;                                      } ;
    
    void draw( nyx::Chain<Framework>& chain, nyx::Pipeline<Framework>& pipeline )
    {
      chain.begin() ;
      chain.draw( pipeline, this->d_vertices ) ; 
      chain.end() ;
    };
    
    void updateViewProj()
    {
      glm::mat4 viewproj ;
      if( this->dirty && this->projection != nullptr )
      {
        viewproj = ( glm::mat4( 1.0f ) ) ; 
        
        this->copy_chain.copy( &viewproj, this->d_viewproj ) ;
        this->copy_chain.submit     () ;
        this->copy_chain.synchronize() ;
      }
    }
    
    void updateTextureIds()
    {
      this->copy_chain.copy( this->indices.data(), this->d_indices ) ;
      this->copy_chain.submit     () ;
      this->copy_chain.synchronize() ;
    }
  };
  
  static NyxDrawTex2DData data_2d ;
  
  NyxDrawTex2D::NyxDrawTex2D()
  {
    auto function = [=] ( unsigned id, unsigned tex_id, nyx::Chain<Framework>& chain, nyx::Pipeline<Framework>& pipeline )
    {
      data_2d.indices[ id ] = tex_id ;
      data_2d.count++ ;
      data_2d.draw( chain, pipeline ) ;
    };
    
    data_2d.indices.resize( TRANSFORM_SIZE ) ;
    NyxDrawModule::setTransformFlag  ( nyx::ArrayFlags::StorageBuffer                           ) ;
    NyxDrawModule::setTransformKey   ( "transform"                                              ) ;
    NyxDrawModule::setTransformSize  ( TRANSFORM_SIZE                                           ) ;
    NyxDrawModule::setPipeline       ( nyx::bytes::draw_tex2d, sizeof( nyx::bytes::draw_tex2d ) ) ;
    NyxDrawModule::setPerDrawCallback( function                                                 ) ;
  }
  
  NyxDrawTex2D::~NyxDrawTex2D()
  {
    
  }
  
  void NyxDrawTex2D::updateTextures()
  {
    Log::output( "Module ", this->name(), " updating texture array." ) ;
    Framework::deviceSynchronize( this->gpu() ) ;
    this->pipeline().bind( "textures", mars::TextureArray<Framework>::images(), mars::TextureArray<Framework>::count() ) ;
    Framework::deviceSynchronize( this->gpu() ) ;
    nyx::updated_textures = true ;
  }
  
  void NyxDrawTex2D::initialize()
  {
    data_2d.copy_chain.initialize( this->gpu(), nyx::ChainType::Compute                                       ) ;
    data_2d.d_viewproj.initialize( this->gpu(), 1                     , false, nyx::ArrayFlags::UniformBuffer ) ;
    data_2d.d_indices .initialize( this->gpu(), data_2d.indices.size(), false, nyx::ArrayFlags::StorageBuffer ) ;
    data_2d.d_vertices.initialize( this->gpu(), 6                     , false, nyx::ArrayFlags::Vertex        ) ;
    
    NyxDrawModule::pipeline().bind( "projection", data_2d.d_viewproj ) ;
    NyxDrawModule::pipeline().bind( "texture_id", data_2d.d_indices  ) ;
    
    data_2d.copy_chain.copy( nyx::vertices, data_2d.d_vertices ) ;
    data_2d.copy_chain.submit     () ;
    data_2d.copy_chain.synchronize() ;

    mars::TextureArray<Framework>::addCallback( this, &NyxDrawTex2D::updateTextures, this->name() ) ;
  }
  
  void NyxDrawTex2D::subscribe( unsigned id )
  {
    this->bus.setChannel( id ) ;
    NyxDrawModule::subscribe( this->bus ) ;
    
    this->bus.enroll( &data_2d, &NyxDrawTex2DData::setCameraInput    , iris::OPTIONAL, this->name(), "::camera"     ) ;
    this->bus.enroll( &data_2d, &NyxDrawTex2DData::setProjectionInput, iris::OPTIONAL, this->name(), "::projection" ) ;
  }
  
  void NyxDrawTex2D::shutdown()
  {
    NyxDrawModule::shutdown() ;
    data_2d.d_viewproj.reset() ;
  }
  
  void NyxDrawTex2D::execute()
  {
    if( nyx::updated_textures )
    {
      data_2d.updateViewProj() ;
      data_2d.count = 0 ;
      this->draw() ;
      data_2d.updateTextureIds() ;
    }
    this->bus.emit() ;
  }
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDrawTex2D" ;
}

/** Exported function to retrieve the version of this module.
 * @return The version of this module.
 */
exported_function unsigned version()
{
  return VERSION ;
}

/** Exported function to make one instance of this module.
 * @return A single instance of this module.
 */
exported_function ::iris::Module* make()
{
  return new nyx::NyxDrawTex2D() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxDrawTex2D* mod ;
  
  mod = dynamic_cast<nyx::NyxDrawTex2D*>( module ) ;
  delete mod ;
}
// </editor-fold>