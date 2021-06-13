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
 * File:   NyxDrawModel.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawModel.h"
#include "draw_model.h"
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
  struct NyxDrawModelData
  {
    struct Iterators
    {
      unsigned index       ;
      unsigned diffuse_tex ;
    };

    nyx::Chain<Framework>                      copy_chain ;
    nyx::Array<Framework, glm::mat4>           d_viewproj ;
    std::unordered_map<std::string, Iterators> mesh_map   ;
    iris::Bus                                  bus        ;
    bool                                       dirty      ;
    const glm::mat4*                           projection ;
    const glm::mat4*                           camera     ;
    
    NyxDrawModelData()                              { this->dirty = false ; this->projection = nullptr ; this->camera = nullptr ;         } ;
    void setProjectionInput( const char* input    ) { this->bus.enroll( this, &NyxDrawModelData::setProjection, iris::OPTIONAL, input ) ; } ;
    void setCameraInput    ( const char* input    ) { this->bus.enroll( this, &NyxDrawModelData::setCamera    , iris::OPTIONAL, input ) ; } ;
    void setProjection     ( const glm::mat4& val ) { this->projection = &val ; this->dirty = true ;                                      } ;
    void setCamera         ( const glm::mat4& val ) { this->camera     = &val ; this->dirty = true ;                                      } ;
    
    void updateViewProj()
    {
      glm::mat4 viewproj ;
      if( this->dirty && this->projection != nullptr && this->camera != nullptr )
      {
        viewproj = ( *this->projection * *this->camera ) ; 
        
        this->copy_chain.copy( &viewproj, this->d_viewproj ) ;
        this->copy_chain.submit     () ;
        this->copy_chain.synchronize() ;
      }
    }
  };
  
  static NyxDrawModelData model_data ;
  
  NyxDrawModel::NyxDrawModel()
  {
    auto function = [=] ( unsigned id, mars::Reference<mars::Model<Framework>>& model, nyx::Chain<Framework>& draw_chain, nyx::Pipeline<Framework>& pipeline )
    {
          auto& meshes = model->meshes() ;

          for( auto mesh : meshes )
          {
            auto& iter = model_data.mesh_map[ mesh->name ] ;
            auto mesh_iter = mesh->textures.find( "diffuse" ) ;
            if( mesh_iter != mesh->textures.end() ) iter.diffuse_tex = mesh->textures[ "diffuse" ] ;
            else                                    iter.diffuse_tex = 0                           ;
            
            iter.index = id ;
            draw_chain.push       ( pipeline, iter                          ) ;
            draw_chain.drawIndexed( pipeline, mesh->indices, mesh->vertices ) ;
          }
    };
    
    NyxDrawModule::setTransformFlag  ( nyx::ArrayFlags::StorageBuffer                           ) ;
    NyxDrawModule::setTransformKey   ( "transform"                                              ) ;
    NyxDrawModule::setTransformSize  ( TRANSFORM_SIZE                                           ) ;
    NyxDrawModule::setPipeline       ( nyx::bytes::draw_model, sizeof( nyx::bytes::draw_model ) ) ;
    NyxDrawModule::setPerDrawCallback( function                                                 ) ;
  }
  
  NyxDrawModel::~NyxDrawModel()
  {
    
  }
  
  void NyxDrawModel::updateTextures()
  {
    Log::output( "Module ", this->name(), " updating texture array." ) ;
    Framework::deviceSynchronize( this->gpu() ) ;
    this->pipeline().bind( "textures", mars::TextureArray<Framework>::images(), mars::TextureArray<Framework>::count() ) ;
    Framework::deviceSynchronize( this->gpu() ) ;
  }
  
  void NyxDrawModel::initialize()
  {
    model_data.copy_chain.initialize( this->gpu(), nyx::ChainType::Compute                  ) ;
    model_data.d_viewproj.initialize( this->gpu(), 1, false, nyx::ArrayFlags::UniformBuffer ) ;
    
    NyxDrawModule::pipeline().bind( "projection", model_data.d_viewproj ) ;
    
    mars::TextureArray<Framework>::addCallback( this, &NyxDrawModel::updateTextures, this->name() ) ;
  }
  
  void NyxDrawModel::subscribe( unsigned id )
  {
    this->bus.setChannel( id ) ;
    NyxDrawModule::subscribe( this->bus ) ;
    
    this->bus.enroll( &model_data, &NyxDrawModelData::setCameraInput    , iris::OPTIONAL, this->name(), "::camera"     ) ;
    this->bus.enroll( &model_data, &NyxDrawModelData::setProjectionInput, iris::OPTIONAL, this->name(), "::projection" ) ;
  }
  
  void NyxDrawModel::shutdown()
  {
    NyxDrawModule::shutdown() ;
    model_data.d_viewproj.reset() ;
  }
  
  void NyxDrawModel::execute()
  {
    model_data.updateViewProj() ;
    this->draw() ;
    this->bus.emit() ;
  }
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDrawModel" ;
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
  return new nyx::NyxDrawModel() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxDrawModel* mod ;
  
  mod = dynamic_cast<nyx::NyxDrawModel*>( module ) ;
  delete mod ;
}
// </editor-fold>