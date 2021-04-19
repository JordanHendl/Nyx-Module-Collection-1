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
 * File:   NyxStartDraw.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxEndDraw.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/profiling/Timer.h>
#include <iris/config/Parser.h>
#include <nyxgpu/library/Array.h>
#include <nyxgpu/library/Image.h>
#include <nyxgpu/library/Chain.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <glm/glm.hpp>
#include <limits>
#include <string>
#include <vector>
#include <climits>
#include <map>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    using          Log       = iris::log::Log   ;
    using          Framework = nyx::vkg::Vulkan ;

    template<unsigned ID>
    unsigned baseGetId()
    {
      return ID ;
    }

    struct NyxEndDrawData
    {
      iris::Bus                                        bus          ;
      std::string                                      name         ;
      std::map<unsigned, const nyx::Chain<Framework>*> chain_map    ;
      const nyx::Chain<Framework>*                     parent       ;
      const nyx::RenderPass<Framework>*                pass         ;
      unsigned                                         num_expected ;
      bool                                             dirty        ;

      /** Default constructor.
       */
      NyxEndDrawData() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      void signal() ;
      
      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setInputName( unsigned idx, const char* name ) ;
      
      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setInputWaitNames( unsigned idx, const char* name ) ;
      
      template<unsigned ID>
      void setInputDrawChain( const nyx::Chain<Framework>& chain ) ;
      
      void setRenderPassRef( const nyx::RenderPass<Framework>& render_pass ) ;
      
      void setRenderPassRefName( const char* name ) ;
      
      void setParentChainRef( const nyx::Chain<Framework>& chain ) ;
      
      template<unsigned ID>
      void setInputWaitSignal() ;
      
      void setFinishSignalName( const char* name ) ;
      
      void finish() ;
    };
    
    template<unsigned ID>
    void NyxEndDrawData::setInputDrawChain( const nyx::Chain<Framework>& chain )
    {
      this->dirty = true ;
      this->chain_map[ ID ] = &chain ;
    }
    
    template<unsigned ID>
    void NyxEndDrawData::setInputWaitSignal()
    {
      
    }
    
    void NyxEndDrawData::finish()
    {
      
    }
    
    void NyxEndDrawData::setRenderPassRef( const nyx::RenderPass<Framework>& pass )
    {
      this->pass = &pass ;
    }
    
    void NyxEndDrawData::setParentChainRef( const nyx::Chain<Framework>& chain )
    {
      this->parent = &chain ;
    }

    void NyxEndDrawData::setRenderPassRefName( const char* name )
    {
      this->bus.enroll( this, &NyxEndDrawData::setRenderPassRef, iris::OPTIONAL, name ) ;
    }
    
    void NyxEndDrawData::setFinishSignalName( const char* name )
    {
      this->bus.publish( this, &NyxEndDrawData::finish, name ) ;
    }
    
    void NyxEndDrawData::setInputName( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input ", idx, " as \"", name, "\"" ) ;
      
      switch( idx )
      {
        case 0  : this->bus.enroll( this, &NyxEndDrawData::setInputDrawChain<0>, iris::OPTIONAL, name ) ; break ;
        case 1  : this->bus.enroll( this, &NyxEndDrawData::setInputDrawChain<1>, iris::OPTIONAL, name ) ; break ;
        case 2  : this->bus.enroll( this, &NyxEndDrawData::setInputDrawChain<2>, iris::OPTIONAL, name ) ; break ;
        case 3  : this->bus.enroll( this, &NyxEndDrawData::setInputDrawChain<3>, iris::OPTIONAL, name ) ; break ;
        case 4  : this->bus.enroll( this, &NyxEndDrawData::setInputDrawChain<4>, iris::OPTIONAL, name ) ; break ;
        default : break ;
      }
    }
    
    void NyxEndDrawData::setInputWaitNames( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set required input ", idx, " as \"", name, "\"" ) ;
      switch( idx )
      {
        case 0  : this->bus.enroll( this, &NyxEndDrawData::setInputWaitSignal<0>, iris::REQUIRED, name ) ; break ;
        case 1  : this->bus.enroll( this, &NyxEndDrawData::setInputWaitSignal<1>, iris::REQUIRED, name ) ; break ;
        case 2  : this->bus.enroll( this, &NyxEndDrawData::setInputWaitSignal<2>, iris::REQUIRED, name ) ; break ;
        case 3  : this->bus.enroll( this, &NyxEndDrawData::setInputWaitSignal<3>, iris::REQUIRED, name ) ; break ;
        case 4  : this->bus.enroll( this, &NyxEndDrawData::setInputWaitSignal<4>, iris::REQUIRED, name ) ; break ;
        default : break ;
      }
    }
    NyxEndDrawData::NyxEndDrawData()
    {
      this->num_expected = 0       ;
      this->pass         = nullptr ;
      this->parent       = nullptr ;
    }
    
    NyxEndDraw::NyxEndDraw()
    {
      this->module_data = new NyxEndDrawData() ;
    }

    NyxEndDraw::~NyxEndDraw()
    {
      delete this->module_data ;
    }

    void NyxEndDraw::initialize()
    {

    }

    void NyxEndDraw::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      data().bus.enroll( this->module_data, &NyxEndDrawData::setInputName        , iris::OPTIONAL, this->name(), "::draw_inputs"    ) ;
      data().bus.enroll( this->module_data, &NyxEndDrawData::setInputWaitNames   , iris::OPTIONAL, this->name(), "::wait_on"        ) ;
      data().bus.enroll( this->module_data, &NyxEndDrawData::setRenderPassRefName, iris::OPTIONAL, this->name(), "::render_pass"    ) ;
      data().bus.enroll( this->module_data, &NyxEndDrawData::setFinishSignalName , iris::OPTIONAL, this->name(), "::finish_signal"  ) ;
    }

    void NyxEndDraw::shutdown()
    {
    }

    void NyxEndDraw::execute()
    {
      data().bus.wait() ;
      
      auto parent = const_cast<nyx::Chain<Framework>*     >( data().parent ) ;
      auto pass   = const_cast<nyx::RenderPass<Framework>*>( data().pass   ) ;
      
      if( data().dirty && data().num_expected == data().chain_map.size() && data().pass != nullptr )
      {
        for( auto chain : data().chain_map )
        {
          parent->combine( *chain.second ) ;
        }
        
        data().dirty = false ;
      }
      if( data().parent != nullptr ) parent->submit() ;
      if( data().pass   != nullptr ) pass->present () ;
      
      data().bus.emit() ;
    }

    NyxEndDrawData& NyxEndDraw::data()
    {
      return *this->module_data ;
    }

    const NyxEndDrawData& NyxEndDraw::data() const
    {
      return *this->module_data ;
    }
  }
}

/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxEndDraw" ;
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
  return new nyx::vkg::NyxEndDraw() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxEndDraw* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxEndDraw*>( module ) ;
  delete mod ;
}
