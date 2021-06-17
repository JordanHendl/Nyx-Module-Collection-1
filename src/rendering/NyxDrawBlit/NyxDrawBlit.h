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

#pragma once 

#include <templates/NyxDrawModule.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Array.h>
#include <Iris/data/Bus.h>
#include <atomic>

namespace nyx
{
  
  struct NyxDrawBlitData
  {
    iris::Bus                                 bus        ;
    std::atomic<const nyx::Image<Framework>*> input      ;
    nyx::Array<Framework, glm::vec4>          vertices   ;
    nyx::Chain<Framework>                     copy_chain ;

    NyxDrawBlitData()
    {
      this->input = nullptr ;
    }
    
    void setInputName( const char* signal )
    {
      Log::output( "NyxDrawBlit module setting input image signal to ", signal ) ;
      this->bus.enroll( this, &NyxDrawBlitData::setInput, iris::OPTIONAL, signal ) ; 
    } ;
    
    void setInput( const nyx::Image<Framework>& image )
    {
      this->input = &image ; 
      
      Log::output( "NyxDrawBlit module setting input image to ", static_cast<const void*>( this->input ) ) ;
    };
  };
  
  /** A module for managing converting images on the host to Vulkan images on the GPU.
   */
  class NyxDrawBlit : public nyx::NyxDrawModule<nyx::Image<nyx::vkg::Vulkan>>
  {
    public:

      /** Default Constructor.
       */
      NyxDrawBlit() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      ~NyxDrawBlit() ;

      void draw() ;
      
      /** Method to initialize this module after being configured.
       */
      void initialize() ;

      /** Method to subscribe this module's configuration to the bus.
       * @param id The id to use for this graph.
       */
      void subscribe( unsigned id ) ;

      void updateTextures() ;
      
      /** Method to shut down this object's operation.
       */
      void shutdown() ;

      /** Method to execute a single instance of this module's operation.
       */
      void execute() ;

    private:
      NyxDrawBlitData data ;
  };
}