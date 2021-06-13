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
 * File:   NyxDrawBlit.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawBlit.h"
#include "blit.h"
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <unordered_map>
#include <queue>

static const unsigned VERSION = 1 ;
namespace nyx
{
  const static glm::vec4 vertices[] = 
  { 
               // pos      // tex
    glm::vec4( -1.0f,  1.0f, 0.0f,  0.0f ),
    glm::vec4(  1.0f, -1.0f, 1.0f, -1.0f ),
    glm::vec4( -1.0f, -1.0f, 0.0f, -1.0f ), 

    glm::vec4( -1.0f,  1.0f, 0.0f,  0.0f ),
    glm::vec4(  1.0f,  1.0f, 1.0f,  0.0f ),
    glm::vec4(  1.0f, -1.0f, 1.0f, -1.0f )
  };
    
  constexpr unsigned TRANSFORM_SIZE = 1024 ;
  using Framework = nyx::vkg::Vulkan ;

  NyxDrawBlit::NyxDrawBlit()
  {
    NyxDrawModule::setTransformKey   ( "transform"                                  ) ;
    NyxDrawModule::setTransformSize  ( TRANSFORM_SIZE                               ) ;
    NyxDrawModule::setPipeline       ( nyx::bytes::blit, sizeof( nyx::bytes::blit ) ) ;
  }
  
  NyxDrawBlit::~NyxDrawBlit()
  {
    
  }
  
  void NyxDrawBlit::initialize()
  {
    this->data.vertices  .initialize( this->gpu(), 6, false, nyx::ArrayFlags::Vertex ) ;
    this->data.copy_chain.initialize( this->gpu(), nyx::ChainType::Compute           ) ;
    
    this->data.copy_chain.copy( nyx::vertices, this->data.vertices ) ;
    this->data.copy_chain.submit     () ;
    this->data.copy_chain.synchronize() ;
  }
  
  void NyxDrawBlit::subscribe( unsigned id )
  {
    this->data.bus.setChannel( id ) ;
    NyxDrawModule::subscribe( this->data.bus ) ;
    
    this->data.bus.enroll( &this->data, &NyxDrawBlitData::setInputName, iris::OPTIONAL, this->name(), "::image" ) ;
  }
  
  void NyxDrawBlit::shutdown()
  {
    NyxDrawModule::shutdown() ;
  }
  
  void NyxDrawBlit::draw()
  {
    if( this->data.input )
    {
      Log::output( "Module ", this->name(), " redrawing!" ) ;
      Framework::deviceSynchronize( this->gpu() ) ;
      this->pipeline().bind( "tex", *this->data.input ) ;
      
      this->chain().begin() ;
      this->chain().draw( this->pipeline(), this->data.vertices ) ;
      this->chain().end() ;
      
      this->emit() ;
      this->data.input = nullptr ;
    }
  }
  
  void NyxDrawBlit::execute()
  {
    this->draw() ;
  }
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDrawBlit" ;
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
  return new nyx::NyxDrawBlit() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxDrawBlit* mod ;
  
  mod = dynamic_cast<nyx::NyxDrawBlit*>( module ) ;
  delete mod ;
}
// </editor-fold>