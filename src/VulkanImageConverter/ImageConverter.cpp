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
#include <data/Bus.h>
#include <library/Array.h>
#include <vkg/Device.h>
#include <vkg/Buffer.h>
#include <vkg/Synchronization.h>
#include <vkg/Vulkan.h>
#include <vkg/Queue.h>
#include <vkg/Image.h>
#include <vkg/CommandBuffer.h>
#include <template/List.h>
#include <string>
#include <vulkan/vulkan.hpp>

static const unsigned VERSION = 1 ;

namespace nyx
{
  namespace vkg
  {
    struct ImageConverterData
    {
      nyx::List<nyx::vkg::CommandBuffer>   cmds           ;
      nyx::List<nyx::vkg::Synchronization> syncs          ;
      nyx::vkg::Queue                      queue          ;
      nyx::vkg::Image                      converted_img  ;
      nyx::vkg::VkArray<unsigned char>     staging_buffer ;
      nyx::vkg::Synchronization            current        ;
      iris::Bus                            bus            ;
      nyx::vkg::Device                     device         ;
      std::string                          name           ;
      const unsigned char*                 host_bytes     ;
      unsigned                             width          ;
      unsigned                             height         ;
      unsigned                             channels       ;
      unsigned                             staging_size   ;

      /** Default constructor.
       */
      ImageConverterData() ;
      
      /** Method to retrieve the latest secondary command buffer used by this module.
       * @return The latest secondary command buffer used by this module.
       */
      const nyx::vkg::Synchronization& sync() ;
      
      /** Method to retrieve a const image from this object.
       * @return The const reference to this object's image object.
       */
      const nyx::vkg::Image& image() ;
      
      /** Method to set the output name to associate with this module's command buffers.
       * @param name The name to associate with this module's output.
       */
      void setOutputSynchronizationName( const char* name ) ;

      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutputName( const char* name ) ;

      /** Method to set the size of the staging buffer of this object in bytes.
       * @param byte_size The size in bytes to make this object's staging buffer.
       */
      void setStagingBufferSize( unsigned byte_size ) ;

      /** Method to set the name of the input width parameter.
       * @param name The name to associate with the input.
       */
      void setInputWidthName( const char* name ) ;
      
      /** Method to set the name of the input height parameter.
       * @param name The name to associate with the input.
       */
      void setInputHeightName( const char* name ) ;

      /** Method to set the name of the input channel parameter.
       * @param name The name to associate with the input.
       */
      void setInputChannelName( const char* name ) ;
      
      /** Method to set the name of the input image bytes parameter.
       * @param name The name to associate with the input.
       */
      void setInputBytesName( const char* name ) ;
      
      /** Method to set the device to use for this module's operations.
       * @param device The device to set.
       */
      void setDevice( unsigned id, const nyx::vkg::Device& device ) ;
      
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
    
    ImageConverterData::ImageConverterData()
    {
      this->width      = 0       ;
      this->height     = 0       ;
      this->host_bytes = nullptr ;
      this->name       = ""      ;
    }
    
    const nyx::vkg::Image& ImageConverterData::image()
    {
      return this->converted_img ;
    }

    const nyx::vkg::Synchronization& ImageConverterData::sync()
    {
      this->current = this->syncs.current() ;
      this->syncs.advance() ;
      return this->current ;
    }
    
    void ImageConverterData::setOutputSynchronizationName( const char* name )
    {
      this->bus.publish( this, &ImageConverterData::sync, name ) ;
    }
    
    void ImageConverterData::setOutputName( const char* name )
    {
      this->bus.publish( this, &ImageConverterData::image, name ) ;
    }
    
    void ImageConverterData::setStagingBufferSize( unsigned byte_size )
    {
      this->staging_size = byte_size ;

      if( this->device.initialized() )
      {
        this->staging_buffer.reset() ;
        this->staging_buffer.initialize( this->device, byte_size, true, vk::BufferUsageFlagBits::eTransferSrc ) ;
      }
    }
    
    void ImageConverterData::setInputWidthName( const char* name )
    {
      this->bus.enroll( this, &ImageConverterData::setWidth, iris::REQUIRED, name ) ;
    }
    
    void ImageConverterData::setInputHeightName( const char* name )
    {
      this->bus.enroll( this, &ImageConverterData::setHeight, iris::REQUIRED, name ) ;
    }
    
    void ImageConverterData::setInputChannelName( const char* name )
    {
      this->bus.enroll( this, &ImageConverterData::setNumChannels, iris::REQUIRED, name ) ;
    }
    
    void ImageConverterData::setInputBytesName( const char* name )
    {
      this->bus.enroll( this, &ImageConverterData::setHostBytes, iris::REQUIRED, name ) ;
    }

    void ImageConverterData::setDevice( unsigned id, const nyx::vkg::Device& device )
    {
      if( !this->device.initialized() && id == 0 )
      {
        this->device = device ;
        this->staging_buffer.reset() ;
        this->staging_buffer.initialize( this->device, this->staging_size, true, vk::BufferUsageFlagBits::eTransferSrc ) ;
        this->queue = this->device.transferQueue()   ;
        this->cmds .initialize( 20, this->queue, 1 ) ;
        this->syncs.initialize( 20, device, 0      ) ;
      }
    }
    
    void ImageConverterData::setWidth( unsigned width )
    {
      this->width = width ;
    }
    
    void ImageConverterData::setHeight( unsigned height )
    {
      this->height = height ;
    }
    
    void ImageConverterData::setNumChannels( unsigned num_channels )
    {
      this->channels = num_channels ;
    }
    
    void ImageConverterData::setHostBytes( const unsigned char* data )
    {
      this->host_bytes = data ;
    }
    
    ImageConverter::ImageConverter()
    {
      this->module_data = new ImageConverterData() ;
    }

    ImageConverter::~ImageConverter()
    {
      delete this->module_data ;
    }

    void ImageConverter::initialize()
    {
      
    }

    void ImageConverter::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setDevice                   , iris::OPTIONAL,               "vkg_device"       ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setInputWidthName           , iris::OPTIONAL, this->name(), "::input_width"    ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setInputHeightName          , iris::OPTIONAL, this->name(), "::input_height"   ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setInputChannelName         , iris::OPTIONAL, this->name(), "::input_channels" ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setInputBytesName           , iris::OPTIONAL, this->name(), "::input_bytes"    ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setOutputName               , iris::OPTIONAL, this->name(), "::output"         ) ;
      data().bus.enroll( this->module_data, &ImageConverterData::setOutputSynchronizationName, iris::OPTIONAL, this->name(), "::sync_output"    ) ;
    }

    void ImageConverter::shutdown()
    {
      data().device.wait() ;
      data().converted_img .reset() ;
      data().staging_buffer.reset() ;
      data().syncs         .reset() ;
      data().cmds          .reset() ;
    }

    void ImageConverter::execute()
    {
      unsigned img_size ;
      
      data().bus.wait() ;
      
      img_size = data().width * data().height * data().channels ;
      
      // Resize image to desired dimensions.
      data().converted_img.resize( data().width, data().height ) ;
      
      // Record the needed command.
      data().cmds.current().record() ;
      if( data().staging_buffer.size() > img_size )
      {
        data().staging_buffer.copyToDevice( data().host_bytes, img_size ) ; 
        data().converted_img.copy( data().staging_buffer.buffer(), data().cmds.current().buffer( 0 ) ) ;
      }
      data().cmds.current().stop() ;
      
      // Submit and advance to the next command buffer.
      data().queue.submit( data().cmds, data().syncs ) ;
      data().cmds.advance() ;
      
      // Send synchronization along the way.
      data().bus.emit() ;
    }

    ImageConverterData& ImageConverter::data()
    {
      return *this->module_data ;
    }

    const ImageConverterData& ImageConverter::data() const
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
  return "Vulkan Image Converter" ;
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
