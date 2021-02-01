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
#include <vulkan/vulkan.hpp>
#include <map>
#include <mutex>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    struct VkModuleData
    {
      using Devices          = std::vector<nyx::vkg::Device>                          ;
      using CmdMap           = std::map<unsigned, nyx::List<nyx::vkg::CommandBuffer>> ;
      using ValidationLayers = std::vector<std::string>                               ;

      nyx::List<nyx::vkg::CommandBuffer> buffers         ; ///< The command buffers to use for submitting to the graphics queue.
      CmdMap                             buffer_map      ; ///< The command buffer map to ensure we submit each buffer onto it's rightful queue.
      iris::Bus                          bus             ; ///< The bus to communicate over.
      nyx::vkg::Instance                 iris_instance    ; ///< The iris instance to sent.
      Devices                            devices         ; ///< The vector of devices generated & found.
      ::vk::SurfaceKHR                   surface         ; ///< The surface to use for generating a device.
      std::string                        name            ; ///< The name of the module.
      bool                               expects_surface ; ///< Whether or not this module is expecting a vulkan surface, and should not build virtual devices without it.
      std::mutex                         mutex           ; ///< Mutex to help make sure command buffers arent being set while this object is processing them.
      ValidationLayers                   layers          ; ///< The list of validation layers to use when creating the instance.

      /** Default constructor.
       */
      VkModuleData() ;
      
      /** Method to add a command buffer to this module for processing.
       * @param cmd The secondary command buffer to add to this object.
       */
      void addCmdBuffer( const nyx::vkg::CommandBuffer& cmd ) ;

      /** Method to set the name of the expected secondary command buffer input.
       * @param name The name to associate with the secondary command buffer input.
       */
      void setCmdBufferInputName( const char* name ) ;
      
      /** Method to add a validation layer to be added to this instance.
       * @param validation_layer The test name of the layer to add.
       */
      void addValidationLayer( unsigned idx, const char* validation_layer ) ;

      /** Method to set the surface input name.
       * @param input The name of input to recieve for a vulkan surface.
       */
      void setSurfaceInput( const char* input ) ;

      /** Method to input a surface to use for generating vulkan devices.
       * @param surface The surface to use for generating vulkan devices.
       */
      void setSurface( const ::vk::SurfaceKHR& surface ) ;
      
      /** Method to retrieve the instance of this object.
       * @return Const reference to the instance of this object.
       */
      const nyx::vkg::Instance& instance() ;
      
      /** Method to retrieve the instance of this object.
       * @return Const reference to the instance of this object.
       */
      const vk::Instance& vulkan_instance() ;
      
      /** Method to retrieve const reference to the device of this object;
       * @return Const referecnce to a iris device from this object.
       */
      const nyx::vkg::Device& device( unsigned index ) ;
      
      /** Method to retrieve const reference to the device of this object;
       * @return Const referecnce to a iris device from this object.
       */
      const vk::Device& vulkan_device( unsigned index ) ;
    };
    
    VkModuleData::VkModuleData()
    {
      
    }

    void VkModuleData::setCmdBufferInputName( const char* name )
    {
      name = name ;
    }
    
    void VkModuleData::addValidationLayer( unsigned idx, const char* validation_layer )
    {
      idx = idx ;
      this->layers.push_back( validation_layer ) ;
    }

    void VkModuleData::setSurfaceInput( const char* input )
    {
      this->expects_surface = true ;
      this->bus.enroll( this, &VkModuleData::setSurface, iris::REQUIRED, input ) ;
    }
    
    void VkModuleData::setSurface( const vk::SurfaceKHR& surface )
    {
      unsigned index ;
      
      this->surface = surface ;

      if( this->devices.empty() )
      {
        index = 0 ;
        this->devices.resize( this->iris_instance.numDevices() ) ;
        for( auto& device : this->devices )
        {
          device.addExtension( "VK_KHR_swapchain" ) ;
          device.initialize( this->iris_instance.device( index ), surface ) ;

          this->bus.emit( index++ ) ;
        }
      }
    }

    const nyx::vkg::Instance& VkModuleData::instance()
    {
      return this->iris_instance ;
    }
    
    const vk::Instance& VkModuleData::vulkan_instance()
    {
      return this->iris_instance.instance() ;
    }
    
    const nyx::vkg::Device& VkModuleData::device( unsigned index )
    {
      static nyx::vkg::Device dummy ;
      if( index < this->devices.size() ) return this->devices[ index ] ;
      
      return dummy ;
    }
    
    const vk::Device& VkModuleData::vulkan_device( unsigned index )
    {
      static vk::Device dummy ;
      if( index < this->devices.size() ) return this->devices[ index ].device() ;
      
      return dummy ;
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
      unsigned index ;

      if( data().expects_surface )
      {
        data().iris_instance.addExtension( "VK_KHR_surface"                                 ) ;
        data().iris_instance.addExtension( vkg::Vulkan::platformSurfaceInstanceExtensions() ) ;
      }

      for( auto& layer : data().layers )
      {
        data().iris_instance.addValidationLayer( layer.c_str() ) ;
      }
      
      data().iris_instance.initialize() ;
      nyx::vkg::Vulkan::initialize( data().iris_instance ) ;
      
      // If we dont expect an instance.
      if( data().devices.empty() && !data().expects_surface )
      {
        index = 0 ;
        data().devices.resize( data().iris_instance.numDevices() ) ;
        for( auto& device : data().devices )
        {
          device.initialize( data().iris_instance.device( index ) ) ;

          data().bus.emit( index++ ) ;
        }
      }

      data().bus.emit() ;
    }

    void VkModule::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      data().bus.publish( this->module_data, &VkModuleData::instance         , "vkg_instance" ) ;
      data().bus.publish( this->module_data, &VkModuleData::device           , "vkg_device"   ) ;
      data().bus.publish( this->module_data, &VkModuleData::vulkan_instance  , "raw_instance" ) ;
      data().bus.publish( this->module_data, &VkModuleData::vulkan_device    , "raw_device"   ) ;
      data().bus.enroll( this->module_data, &VkModuleData::setSurfaceInput   , iris::OPTIONAL, this->name(), "::surface_input_name" ) ;
      data().bus.enroll( this->module_data, &VkModuleData::addValidationLayer, iris::OPTIONAL, this->name(), "::validation_layers"  ) ;
      
      data().bus.enroll( this->module_data, &VkModuleData::setCmdBufferInputName, iris::REQUIRED, this->name(), "::asldkjalskdkasjd" ) ;
    }

    void VkModule::shutdown()
    {
      // Potentially a problem. what happens during shutdown when this happens first? Bad things...
//      for( auto& device : data().devices )
//      {
//        device.wait () ;
//        device.reset() ;
//      }
//      
//      data().iris_instance.reset() ;
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