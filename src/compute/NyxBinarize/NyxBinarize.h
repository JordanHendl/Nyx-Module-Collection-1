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
  
  struct NyxBinarizeData
  {
    const nyx::Image<Framework>* input         ;
    nyx::Image<Framework>        binarized_map ;
    float                        threshold     ;
    iris::Bus                    bus           ;
    unsigned                     gpu           ;
    std::mutex                   mutex         ;
    bool                         dirty         ;
    
    NyxBinarizeData()
    {
      this->gpu       = 0       ;
      this->threshold = 0.5f    ;
      this->input     = nullptr ;
      this->dirty     = false   ;
    }
    
    void setInputName ( const char* name )
    {
      Log::output( "NyxBinarize Module set input signal to ", name ) ;
      this->bus.enroll ( this, &NyxBinarizeData::setInput, iris::OPTIONAL, name ) ; 
    };
    
    void setThreshold( float val ) 
    {
      Log::output( "NyxBinarize Module set threshold to ", val ) ;
      this->threshold = val ;
    }

    void setOutputName( const char* name  )
    {
      Log::output( "NyxBinarize Module set output signal to ", name ) ;
      this->bus.publish( this, &NyxBinarizeData::output, name ) ; 
    } ;
    
    const nyx::Image<Framework>& output()   { return this->binarized_map ; } ;
    
    void setInput( const nyx::Image<Framework>& image )
    {
      Log::output( "NyxBinarize received image ", static_cast<const void*>( &image ), ". Reinitializing binary map and binding input..." ) ;
      this->mutex.lock() ;
      Framework::deviceSynchronize( this->gpu ) ;
      
      if( image.width() != this->binarized_map.width() || image.height() != this->binarized_map.height() )
      {
        this->binarized_map.setUsage( nyx::ImageUsage::Storage ) ;
        this->binarized_map.reset() ;
        this->binarized_map.initialize( nyx::ImageFormat::RGBA32F, this->gpu, image.width(), image.height(), image.layers() ) ;
      }
      
      this->input = &image ;
      this->dirty = true   ;
      this->mutex.unlock() ;
      Log::output( "NyxBinarize done!" ) ;
      
      Framework::deviceSynchronize( this->gpu ) ;
    }
    
  };

  /** A module for managing converting images on the host to Vulkan images on the GPU.
   */
  class NyxBinarize : public nyx::NyxImageProcessingModule
  {
    public:

      /** Default Constructor.
       */
      NyxBinarize() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      ~NyxBinarize() ;

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
      NyxBinarizeData data ;
  };
}