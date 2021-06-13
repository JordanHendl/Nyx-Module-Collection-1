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

#include "NyxGPUBarrier.h"
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <Iris/profiling/Timer.h>
#include <NyxGPU/vkg/Vulkan.h>
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
    using Log       = iris::log::Log   ;
    using Framework = nyx::vkg::Vulkan ;

    struct NyxGPUBarrierData
    {
      std::string name   ;
      unsigned    device ;
      iris::Bus   bus    ;
      
      /** Default constructor.
       */
      NyxGPUBarrierData() ;
      
      /** Method to wait on an input.
       */
      void wait() ;
      
      /** Method to signal an output.
       */
      void signal() ;
      
      /** Method to signal an output.
       */
      void drawSignal() ;
      
      /** Set the name of the input signal to wait on.
       * @param name the signal name to wait on.
       */
      void setWaitName( unsigned idx, const char* name ) ;
      
      /** Set the name of the output name to signal.
       * @param name the name to signal.
       */
      void setSignalName( unsigned idx, const char* name ) ;

      /** Method to set the device to use for this module's operations.
       * @param device The device to set.
       */
      void setDevice( unsigned id ) ;
    };
    
    NyxGPUBarrierData::NyxGPUBarrierData()
    {
      this->device = 0 ;
    }
    
    void NyxGPUBarrierData::wait()
    {
    }
    
    void NyxGPUBarrierData::signal()
    {
      
    }
    
    void NyxGPUBarrierData::setWaitName( unsigned idx, const char* name )
    {
      idx = idx ;
      Log::output( "Module ", this->name.c_str(), " set to WAIT ON signal ", name, " to perform it's operation." ) ;
      this->bus.enroll( this, &NyxGPUBarrierData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxGPUBarrierData::setSignalName( unsigned idx, const char* name ) 
    {
      idx = idx ;
      Log::output( "Module ", this->name.c_str(), " set to SIGNAL ", name, " whenever it's finished." ) ;
      this->bus.publish( this, &NyxGPUBarrierData::signal, name ) ;
    }
    
    void NyxGPUBarrierData::setDevice( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set device as ", id ) ;
      this->device = id ;
    }
    
    NyxGPUBarrier::NyxGPUBarrier()
    {
      this->module_data = new NyxGPUBarrierData() ;
    }

    NyxGPUBarrier::~NyxGPUBarrier()
    {
      delete this->module_data ;
    }

    void NyxGPUBarrier::initialize()
    {
    }

    void NyxGPUBarrier::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      data().bus.enroll( this->module_data, &NyxGPUBarrierData::setWaitName  , iris::OPTIONAL, this->name(), "::input"  ) ;
      data().bus.enroll( this->module_data, &NyxGPUBarrierData::setSignalName, iris::OPTIONAL, this->name(), "::output" ) ;
      data().bus.enroll( this->module_data, &NyxGPUBarrierData::setDevice    , iris::OPTIONAL, this->name(), "::device" ) ;
    }

    void NyxGPUBarrier::shutdown()
    {
      Framework::deviceSynchronize( data().device ) ;
    }

    void NyxGPUBarrier::execute()
    {
      data().bus.wait() ;
      Framework::deviceSynchronize( data().device ) ;
      data().bus.emit() ;
    }

    NyxGPUBarrierData& NyxGPUBarrier::data()
    {
      return *this->module_data ;
    }

    const NyxGPUBarrierData& NyxGPUBarrier::data() const
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
  return "NyxGPUBarrier" ;
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
  return new nyx::vkg::NyxGPUBarrier() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxGPUBarrier* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxGPUBarrier*>( module ) ;
  delete mod ;
}
