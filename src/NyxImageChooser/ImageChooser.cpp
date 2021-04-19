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

#include "ImageChooser.h"
#include <nyxgpu/library/Image.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <iris/data/Bus.h>
#include <iris/log/Log.h>

static const unsigned VERSION = 1 ;
namespace nyx
{
  struct ImageChooserData
  {
    using Impl = vkg::Vulkan ;
    iris::Bus bus    ;
    unsigned  choice ;

    const nyx::Image<Impl>* rgba8_image ;
    
    ImageChooserData() ;
    void setInputName( unsigned idx, const char* name ) ;
    void setOutputName( const char* name ) ;
    
    const nyx::Image<Impl>& output() ;

    template<unsigned index>
    void input( const nyx::Image<Impl>& image ) ;
    void setChoice( unsigned index ) ;
  };

  ImageChooserData::ImageChooserData()
  {
    this->choice      = 0       ;
    this->rgba8_image = nullptr ;
  }
  
  const nyx::Image<vkg::Vulkan>& ImageChooserData::output()
  {
    return *this->rgba8_image ;
  }
  
  void ImageChooserData::setChoice( unsigned index )
  {
    this->choice = index ;
  }

  void ImageChooserData::setOutputName( const char* name )
  {
    this->bus.publish( this, &ImageChooserData::output, name ) ;
  }

  void ImageChooserData::setInputName( unsigned idx, const char* name )
  {
    switch( idx )
    {
      case 0 : this->bus.enroll( this, &ImageChooserData::input<0 >, iris::REQUIRED, name ) ; break ;
      case 1 : this->bus.enroll( this, &ImageChooserData::input<1 >, iris::REQUIRED, name ) ; break ;
      case 2 : this->bus.enroll( this, &ImageChooserData::input<2 >, iris::REQUIRED, name ) ; break ;
      case 3 : this->bus.enroll( this, &ImageChooserData::input<3 >, iris::REQUIRED, name ) ; break ;
      case 4 : this->bus.enroll( this, &ImageChooserData::input<4 >, iris::REQUIRED, name ) ; break ;
      case 5 : this->bus.enroll( this, &ImageChooserData::input<5 >, iris::REQUIRED, name ) ; break ;
      case 6 : this->bus.enroll( this, &ImageChooserData::input<6 >, iris::REQUIRED, name ) ; break ;
      case 7 : this->bus.enroll( this, &ImageChooserData::input<7 >, iris::REQUIRED, name ) ; break ;
      case 8 : this->bus.enroll( this, &ImageChooserData::input<8 >, iris::REQUIRED, name ) ; break ;
      case 9 : this->bus.enroll( this, &ImageChooserData::input<9 >, iris::REQUIRED, name ) ; break ;
      case 10: this->bus.enroll( this, &ImageChooserData::input<10>, iris::REQUIRED, name ) ; break ;
      case 11: this->bus.enroll( this, &ImageChooserData::input<11>, iris::REQUIRED, name ) ; break ;
      case 12: this->bus.enroll( this, &ImageChooserData::input<12>, iris::REQUIRED, name ) ; break ;
      default : break ;
    }
  }

  template<unsigned index>
  void ImageChooserData::input( const nyx::Image<Impl>& image )
  {
    if( this->choice == index )
    {
      this->rgba8_image = &image ;
    }
  }

  ImageChooser::ImageChooser()
  {
    this->module_data = new ImageChooserData() ;
  }

  ImageChooser::~ImageChooser()
  {
    delete this->module_data ;
  }

  void ImageChooser::initialize()
  {
  
  }

  void ImageChooser::subscribe( unsigned id )
  {
    data().bus.setChannel( id ) ;
    
    data().bus.enroll( this->module_data, &ImageChooserData::setInputName , iris::OPTIONAL, this->name(), "::inputs"  ) ;
    data().bus.enroll( this->module_data, &ImageChooserData::setOutputName, iris::OPTIONAL, this->name(), "::outputs" ) ;
    data().bus.enroll( this->module_data, &ImageChooserData::setChoice    , iris::OPTIONAL, this->name(), "::choice"  ) ;
  }

  void ImageChooser::shutdown()
  {
  
  }

  void ImageChooser::execute()
  {
    data().bus.wait() ;
    data().bus.emit() ;
  }

  ImageChooserData& ImageChooser::data()
  {
    return *this->module_data ;
  }

  const ImageChooserData& ImageChooser::data() const
  {
    return *this->module_data ;
  }
}

/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxImageChooser" ;
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
  return new nyx::ImageChooser() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::ImageChooser* mod ;
  
  mod = dynamic_cast<nyx::ImageChooser*>( module ) ;
  delete mod ;
}