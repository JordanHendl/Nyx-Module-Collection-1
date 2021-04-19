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
 * File:   ImageConverter.cpp
 * Author: Jordan Hendl
 * 
 * Created on January 23, 2021, 3:58 PM
 */

#include "ImageConverter.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/profiling/Timer.h>
#include <nyxgpu/library/Array.h>
#include <nyxgpu/library/Image.h>
#include <nyxgpu/library/Chain.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <string>

static const unsigned VERSION = 1 ;

namespace nyx
{
  namespace vkg
  {
    using          Impl   = nyx::vkg::Vulkan        ;
    constexpr auto Format = nyx::ImageFormat::RGBA8 ;
    
    struct NyxStartDraw
    {
      nyx::Image<Impl>                converted_img  ;
      nyx::Array<Impl, unsigned char> staging_buffer ;
      nyx::Chain<Impl>                chain          ;
      unsigned                        device         ;
      iris::Bus                       bus            ;
      std::string                     name           ;
      const unsigned char*            host_bytes     ;
      unsigned                        width          ;
      unsigned                        height         ;
      unsigned                        channels       ;
      unsigned                        staging_size   ;

      /** Default constructor.
       */
      NyxStartDraw() ;
      
      /** Method to retrieve a const image from this object.
       * @return The const reference to this object's image object.
       */
      const nyx::Image<Impl>& image() ;
      
      /** Method to set the output name to associate with this module's command buffers.
       * @param name The name to associate with this module's output.
       */
      void setOutputSynchronizationName( const char* name ) ;

      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutputName( unsigned idx, const char* name ) ;

      /** Method to set the size of the staging buffer of this object in bytes.
       * @param byte_size The size in bytes to make this object's staging buffer.
       */
      void setStagingBufferSize( unsigned byte_size ) ;

      /** Method to set the name of the input width parameter.
       * @param name The name to associate with the input.
       */
      void setInputName( unsigned idx, const char* name ) ;
      
      /** Method to set the device to use for this module's operations.
       * @param device The device to set.
       */
      void setDevice( unsigned id ) ;
      
      /** Method to set the width of the incoming image.
       * @param width The width of the incoming image.
       */
      void setWidth( unsigned width ) ;
      
      /** Method to set the height of the incoming image.
       * @param height The height in pixels of the incoming image.
       */
      void setHeight( unsigned height ) ;
      
      /** Method to set the number of channels of the incoming image.
       * @param num_channels
       */
      void setNumChannels( unsigned num_channels ) ;
      
      /** Method to set the bytes of the incoming image.
       * @param data
       */
      void setHostBytes( const unsigned char* data ) ;
    };
    
    NyxStartDraw::NyxStartDraw()
    {
      this->width        = 0                                         ;
      this->height       = 0                                         ;
      this->host_bytes   = nullptr                                   ;
      this->name         = ""                                        ;
      this->staging_size = sizeof( unsigned char ) * 1920 * 1080 * 4 ;
      this->device       = 0                                         ;
    }
    
    const nyx::Image<Impl>& NyxStartDraw::image()
    {
      return this->converted_img ;
    }

    void NyxStartDraw::setOutputName( unsigned idx, const char* name )
    {
      switch( idx )
      {
        case 0  : this->bus.publish( this, &NyxStartDraw::image, name ) ;
        default : break ;
      }
    }
    
    void NyxStartDraw::setStagingBufferSize( unsigned byte_size )
    {
      this->staging_size = byte_size ;

      if( Impl::hasDevice( this->device ) )
      {
        this->staging_buffer.reset() ;
        this->staging_buffer.initialize( this->device, byte_size, true, nyx::ArrayFlags::TransferSrc ) ;
      }
    }
    
    void NyxStartDraw::setInputName( unsigned idx, const char* name )
    {
      switch( idx )
      {
        case 0 : this->bus.enroll( this, &NyxStartDraw::setHostBytes  , iris::REQUIRED, name ) ; break ;
        case 1 : this->bus.enroll( this, &NyxStartDraw::setWidth      , iris::REQUIRED, name ) ; break ;
        case 2 : this->bus.enroll( this, &NyxStartDraw::setHeight     , iris::REQUIRED, name ) ; break ;
        case 3 : this->bus.enroll( this, &NyxStartDraw::setNumChannels, iris::REQUIRED, name ) ; break ;
        default : break ;
      }
    }
    
    void NyxStartDraw::setDevice( unsigned id )
    {
      this->device = id ;
    }
    
    void NyxStartDraw::setWidth( unsigned width )
    {
      this->width = width ;
    }
    
    void NyxStartDraw::setHeight( unsigned height )
    {
      this->height = height ;
    }
    
    void NyxStartDraw::setNumChannels( unsigned num_channels )
    {
      this->channels = num_channels ;
    }
    
    void NyxStartDraw::setHostBytes( const unsigned char* data )
    {
      this->host_bytes = data ;
    }
    
    ImageConverter::ImageConverter()
    {
      this->module_data = new NyxStartDraw() ;
    }

    ImageConverter::~ImageConverter()
    {
      delete this->module_data ;
    }

    void ImageConverter::initialize()
    {
      data().staging_buffer.reset() ;
      data().staging_buffer.initialize( data().device, data().staging_size, true, nyx::ArrayFlags::TransferSrc | nyx::ArrayFlags::TransferDst ) ;
      data().chain.initialize( data().device, nyx::ChainType::Compute ) ;
    }

    void ImageConverter::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().bus.enroll( this->module_data, &NyxStartDraw::setInputName , iris::OPTIONAL, this->name(), "::inputs"  ) ;
      data().bus.enroll( this->module_data, &NyxStartDraw::setOutputName, iris::OPTIONAL, this->name(), "::outputs" ) ;
      data().bus.enroll( this->module_data, &NyxStartDraw::setDevice    , iris::OPTIONAL, this->name(), "::device"  ) ;
    }

    void ImageConverter::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
      data().converted_img .reset() ;
      data().staging_buffer.reset() ;
    }

    void ImageConverter::execute()
    {
      iris::Timer timer ;
      unsigned img_size ;
      
      data().bus.wait() ;
      
      timer.start() ;
       if( data().width != 0 && data().height != 0 && data().channels != 0 )
      {
        img_size = data().width * data().height * data().channels ;
        
        if( !data().converted_img.initialized() )
        {
          data().converted_img.initialize( nyx::ImageFormat::RGBA8, data().device, data().width, data().height, 1 ) ;
        }
        
        // Resize image to desired dimensions.
        data().converted_img.resize( data().width, data().height ) ;
        
        if( data().staging_buffer.size() <= img_size )
        {
          data().staging_buffer.reset() ;
          data().staging_buffer.initialize( data().device, img_size + 10000, true, nyx::ArrayFlags::TransferSrc | nyx::ArrayFlags::TransferDst ) ;
        }

        data().chain.transition( data().converted_img, nyx::ImageLayout::TransferSrc ) ;
        data().chain.copy( data().host_bytes, data().staging_buffer, img_size ) ;
        data().chain.copy( data().staging_buffer, data().converted_img        ) ;
        data().chain.transition( data().converted_img, nyx::ImageLayout::ShaderRead ) ;
        data().chain.submit() ;
        data().chain.synchronize() ;
        timer.stop() ;
        data().bus.emit() ;
      }
    }

    NyxStartDraw& ImageConverter::data()
    {
      return *this->module_data ;
    }

    const NyxStartDraw& ImageConverter::data() const
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
  return "NyxImageConverter" ;
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
  return new nyx::vkg::ImageConverter() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::ImageConverter* mod ;
  
  mod = dynamic_cast<nyx::vkg::ImageConverter*>( module ) ;
  delete mod ;
}
