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
 * File:   NyxDrawText2D.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawText2D.h"
#include "draw_text2D.h"
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <queue>

static const unsigned VERSION = 1 ;
namespace nyx
{
  constexpr unsigned TRANSFORM_SIZE = 2048 ;
  
  NyxDrawText2D::NyxDrawText2D()
  {
    auto function = [=] ( unsigned , std::string& string, nyx::Chain<Framework>& , nyx::Pipeline<Framework>&  )
    {
      string.size() ;
    };
    
    NyxDrawModule::setTransformKey   ( "transforms"                                               ) ;
    NyxDrawModule::setTransformSize  ( TRANSFORM_SIZE                                             ) ;
    NyxDrawModule::setPipeline       ( nyx::bytes::draw_text2D, sizeof( nyx::bytes::draw_text2D ) ) ;
    NyxDrawModule::setPerDrawCallback( function                                                   ) ;
  }
  
  NyxDrawText2D::~NyxDrawText2D()
  {
    
  }
  
  void NyxDrawText2D::initialize()
  {
    
  }
  
  void NyxDrawText2D::setFontName( const char* font_name )
  {
    std::string name = font_name ;
  }
  
  void NyxDrawText2D::subscribe( unsigned id )
  {
    this->bus.setChannel( id ) ;
    NyxDrawModule::subscribe( this->bus ) ;
    
    this->bus.enroll( this, &NyxDrawText2D::setFontName, iris::OPTIONAL, this->name(), "::font" ) ;
  }
  
  void NyxDrawText2D::shutdown()
  {
    NyxDrawModule::shutdown() ;
    
  }
  
  void NyxDrawText2D::execute()
  {
    this->draw() ;
    this->bus.emit() ;
  }
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDrawText2D" ;
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
  return new nyx::NyxDrawText2D() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxDrawText2D* mod ;
  
  mod = dynamic_cast<nyx::NyxDrawText2D*>( module ) ;
  delete mod ;
  // </editor-fold>
}