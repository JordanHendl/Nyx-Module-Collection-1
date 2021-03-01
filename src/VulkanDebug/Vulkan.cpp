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

#include "Vulkan.h"
#include <nyx/vkg/Instance.h>
#include <nyx/vkg/CommandBuffer.h>
#include <nyx/vkg/Queue.h>
#include <nyx/template/List.h>
#include <nyx/vkg/Vulkan.h>
#include <nyx/vkg/Device.h>
#include <iris/data/Bus.h>
#include <vector>
#include <map>
#include <mutex>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    struct VkModuleData
    {
      using ValidationLayers = std::vector<std::string> ;

      iris::Bus bus ; ///< The bus to communicate over.

      /** Default constructor.
       */
      VkModuleData() ;
      
      /** Method to add a validation layer to be added to this instance.
       * @param validation_layer The test name of the layer to add.
       */
      void addValidationLayer( unsigned idx, const char* validation_layer ) ;
    };
    
    VkModuleData::VkModuleData()
    {
      
    }

    void VkModuleData::addValidationLayer( unsigned idx, const char* validation_layer )
    {
      idx = idx ;
      Vulkan::addValidationLayer( validation_layer ) ;
    }

    VkModule::VkModule()
    {
      this->module_data = new VkModuleData() ;
    }

    VkModule::~VkModule()
    {
      delete this->module_data ;
    }

    void VkModule::initialize()
    {
      nyx::vkg::Vulkan::initialize() ;
    }

    void VkModule::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().bus.enroll( this->module_data, &VkModuleData::addValidationLayer, iris::OPTIONAL, this->name(), "::validation_layers"  ) ;
    }

    void VkModule::shutdown()
    {
    }

    void VkModule::execute()
    {
      data().bus.wait() ;
    }

    VkModuleData& VkModule::data()
    {
      return *this->module_data ;
    }

    const VkModuleData& VkModule::data() const
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
  return "Vulkan" ;
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
  return new ::nyx::vkg::VkModule() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::VkModule* mod ;
  
  mod = dynamic_cast<::nyx::vkg::VkModule*>( module ) ;
  delete mod ;
}