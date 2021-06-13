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
 * File:   ImageChooser.cpp
 * Author: jhendl
 * 
 * Created on February 14, 2021, 5:34 PM
 */

#include "ImageCombiner.h"
#include "layer_images.h"
#include <NyxGPU/library/RenderPass.h>
#include <NyxGPU/library/Pipeline.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <array>
#include <stdint.h>

static const unsigned VERSION = 1 ;
namespace nyx
{
  struct ImageCombinerData
  {
    static constexpr unsigned NUM_POSSIBLE_IMAGES = 10 ;
    using Impl       = vkg::Vulkan                                              ;
    using ImageArray = std::array<const nyx::Image<Impl>*, NUM_POSSIBLE_IMAGES> ;
    
    nyx::RenderPass<Impl> render_pass ;
    nyx::Pipeline<Impl>   pipeline    ;
    nyx::Chain<Impl>      chain       ;
    iris::Bus             bus         ;
    ImageArray            images      ;
    unsigned              image_count ;
    unsigned              device      ;
    unsigned              window_id   ;
    unsigned              width       ;
    unsigned              height      ;
    
    ImageCombinerData() ;
    
    void setInputName( unsigned idx, const char* name ) ;
    
    void setOutputName( const char* name ) ;
    
    template<unsigned Index>
    void setImage( const nyx::Image<Impl>& input ) ;
    
    void setDevice( unsigned id ) ;
    
    const nyx::Image<Impl>& output() ;
    
    void setWidth( unsigned width ) ;
    
    void setHeight( unsigned height ) ;
  };

  ImageCombinerData::ImageCombinerData()
  {
    this->device    = 0          ;
    this->window_id = UINT32_MAX ;
    this->width     = 1280       ;
    this->height    = 1024       ;
  }
  
  void ImageCombinerData::setDevice( unsigned id )
  {
    this->device = id ;
  }
  
  void ImageCombinerData::setWidth( unsigned width )
  {
    this->width = width ;
  }
  
  void ImageCombinerData::setHeight( unsigned height )
  {
    this->height = height ;
  }
  
  template<unsigned Index>
  void ImageCombinerData::setImage( const nyx::Image<Impl>& input )
  {
    this->images[ Index ] = &input ;
  }

  const nyx::Image<vkg::Vulkan>& ImageCombinerData::output()
  {
    return this->render_pass.framebuffer( 0 ) ;
  }
  
  void ImageCombinerData::setOutputName( const char* name )
  {
    this->bus.publish( this, &ImageCombinerData::output, name ) ;
  }

  void ImageCombinerData::setInputName( unsigned idx, const char* name )
  {
    switch( idx )
    {
      case 0 : this->bus.enroll( this, &ImageCombinerData::setImage<0 >, iris::REQUIRED, name ) ; break ;
      case 1 : this->bus.enroll( this, &ImageCombinerData::setImage<1 >, iris::REQUIRED, name ) ; break ;
      case 2 : this->bus.enroll( this, &ImageCombinerData::setImage<2 >, iris::REQUIRED, name ) ; break ;
      case 3 : this->bus.enroll( this, &ImageCombinerData::setImage<3 >, iris::REQUIRED, name ) ; break ;
      case 4 : this->bus.enroll( this, &ImageCombinerData::setImage<4 >, iris::REQUIRED, name ) ; break ;
      case 5 : this->bus.enroll( this, &ImageCombinerData::setImage<5 >, iris::REQUIRED, name ) ; break ;
      case 6 : this->bus.enroll( this, &ImageCombinerData::setImage<6 >, iris::REQUIRED, name ) ; break ;
      case 7 : this->bus.enroll( this, &ImageCombinerData::setImage<7 >, iris::REQUIRED, name ) ; break ;
      case 8 : this->bus.enroll( this, &ImageCombinerData::setImage<8 >, iris::REQUIRED, name ) ; break ;
      case 9 : this->bus.enroll( this, &ImageCombinerData::setImage<9 >, iris::REQUIRED, name ) ; break ;
      case 10: this->bus.enroll( this, &ImageCombinerData::setImage<10>, iris::REQUIRED, name ) ; break ;
      default : break ;
    }
  }

  ImageCombiner::ImageCombiner()
  {
    this->module_data = new ImageCombinerData() ;
  }

  ImageCombiner::~ImageCombiner()
  {
    delete this->module_data ;
  }

  void ImageCombiner::initialize()
  {
    if( data().window_id != UINT32_MAX )
    {
      data().render_pass.initialize( data().device, data().window_id ) ;
    }
    else
    {
      data().render_pass.initialize( data().device ) ;
    }
    
    data().pipeline.initialize( data().render_pass, nyx::bytes::layer_images, sizeof( nyx::bytes::layer_images ) ) ;
  }

  void ImageCombiner::subscribe( unsigned id )
  {
    data().bus.setChannel( id ) ;
    
    data().bus.enroll( this->module_data, &ImageCombinerData::setInputName , iris::OPTIONAL, this->name(), "::inputs"  ) ;
    data().bus.enroll( this->module_data, &ImageCombinerData::setOutputName, iris::OPTIONAL, this->name(), "::outputs" ) ;
    data().bus.enroll( this->module_data, &ImageCombinerData::setWidth     , iris::OPTIONAL, this->name(), "::width"   ) ;
    data().bus.enroll( this->module_data, &ImageCombinerData::setHeight    , iris::OPTIONAL, this->name(), "::height"  ) ;
    data().bus.enroll( this->module_data, &ImageCombinerData::setDevice    , iris::OPTIONAL, this->name(), "::device"  ) ;
  }

  void ImageCombiner::shutdown()
  {
  
  }

  void ImageCombiner::execute()
  {
    data().bus.wait() ;
    data().bus.emit() ;
  }

  ImageCombinerData& ImageCombiner::data()
  {
    return *this->module_data ;
  }

  const ImageCombinerData& ImageCombiner::data() const
  {
    return *this->module_data ;
  }
}

/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxImageCombiner" ;
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
  return new nyx::ImageCombiner() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::ImageCombiner* mod ;
  
  mod = dynamic_cast<nyx::ImageCombiner*>( module ) ;
  delete mod ;
}