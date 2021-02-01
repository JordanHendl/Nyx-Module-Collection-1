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
 * File:   Test.cpp
 * Author: Jordan Hendl
 *
 * Created on January 2, 2021, 5:43 PM
 */

#include "Vulkan.h"
#include <nyx/vkg/Device.h>
#include <nyx/vkg/Instance.h>
#include <iris/data/Bus.h>
#include <vulkan/vulkan.hpp>
#include <iostream>

static nyx::vkg::Instance kgl_instance ;
static nyx::vkg::Device   kgl_device   ;

void setInstance( const nyx::vkg::Instance& instance )
{
  kgl_instance = instance ;
}

void setDevice( unsigned index, const nyx::vkg::Device& device )
{
  index = index ; // suppress warnings... for now
  if( device.device() ) 
  {
    kgl_device = device ;
  }
}

int main() 
{
  nyx::vkg::VkModule module ;
  iris::Bus          bus    ;
  
  bus.enroll( &setInstance, "vkg_instance" ) ;
  bus.enroll( &setDevice  , "vkg_device"   ) ;
  
  module.subscribe( 0 ) ;
  
  module.initialize() ;
  module.execute()    ;

  if( static_cast<VkInstance>( kgl_instance.instance() ) != nullptr && static_cast<VkDevice>( kgl_device.device() ) != nullptr )
  {
    std::cout << "Instance and device successfully created & Sent successfully." << std::endl ;
    return 0 ;
  }
  
  std::cout << "Module failed to create or send instance & device data." << std::endl ;
  return 1 ;
}

