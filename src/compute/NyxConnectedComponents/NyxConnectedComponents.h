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

#include <templates/NyxImageProcessingModule.h>
#include <Iris/data/Bus.h>
#include <mutex>

namespace nyx
{
  using Framework = nyx::vkg::Vulkan ;
  using Log       = iris::log::Log   ;
  
  struct NyxConnectedComponentsData
  {

  };

  /** A module for managing converting images on the host to Vulkan images on the GPU.
   */
  class NyxConnectedComponents : public nyx::NyxImageProcessingModule
  {
    public:

      /** Default Constructor.
       */
      NyxConnectedComponents() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      ~NyxConnectedComponents() ;

      /** Method to initialize this module after being configured.
       */
      void initialize() ;

      /** Method to subscribe this module's configuration to the bus.
       * @param id The id to use for this graph.
       */
      void subscribe( unsigned id ) ;

      /** Method to shut down this object's operation.
       */
      void shutdown() ;

      /** Method to execute a single instance of this module's operation.
       */
      void execute() ;
      
      void setThreshold( float val ) ;
    private:
      const nyx::Image<Framework>*    input             ;
      nyx::Pipeline<Framework>        boundary_analysis ;
      nyx::Pipeline<Framework>        global_uf         ;
      nyx::Image<Framework>           indexed_map       ;
      nyx::Array<Framework, unsigned> d_indices         ;
      iris::Bus                       bus               ;
      std::mutex                      mutex             ;
      bool                            dirty             ;
      
      void setInputName ( const char* name )
      {
        Log::output( "NyxConnectedComponents Module set input signal to ", name ) ;
        this->bus.enroll ( this, &NyxConnectedComponents::setInput, iris::OPTIONAL, name ) ; 
      };
      
      void setOutputName( const char* name  )
      {
        Log::output( "NyxConnectedComponents Module set output signal to ", name ) ;
        this->bus.publish( this, &NyxConnectedComponents::output, name ) ; 
      } ;
      
      const nyx::Image<Framework>& output()   { return this->indexed_map ; } ;
      
      void setInput( const nyx::Image<Framework>& image )
      {
        Log::output( "NyxConnectedComponents received image ", static_cast<const void*>( &image ), ". Reinitializing binary map and binding input..." ) ;
        this->mutex.lock() ;
        Framework::deviceSynchronize( this->gpu() ) ;
        
        if( image.width() != this->indexed_map.width() || image.height() != this->indexed_map.height() )
        {
          this->indexed_map.setUsage( nyx::ImageUsage::Storage ) ;
          this->indexed_map.reset() ;
          this->d_indices  .reset() ;
          this->indexed_map.initialize( nyx::ImageFormat::RGBA32F, this->gpu(), image.width(), image.height(), image.layers()               ) ;
          this->d_indices  .initialize( this->gpu(), image.width() * image.height() * image.layers(), false, nyx::ArrayFlags::StorageBuffer ) ;
        }
        this->input = &image ;
        this->dirty = true   ;
        
        this->mutex.unlock() ;
        Log::output( "NyxConnectedComponents done!" ) ;
        
        Framework::deviceSynchronize( this->gpu() ) ;
      }
  };
}