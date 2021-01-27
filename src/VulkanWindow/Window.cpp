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

#include "Window.h"
#include <template/List.h>
#include <vkg/Vulkan.h>
#include <library/Window.h>
#include <data/Bus.h>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <string>
#include <mutex>

static const unsigned VERSION = 1 ;

namespace nyx
{
  namespace vkg
  {
    using Impl = nyx::vkg::Vulkan ;
    
    struct VkWindowData
    {
      nyx::List<Impl::Synchronization> syncs       ; ///< The synchronization objects to use for this object's synchronization.
      nyx::List<Impl::CommandRecord  > cmds        ; ///< The command buffers to record commands into.
      iris::Bus                        bus         ; ///< The bus to communicate over.
      Impl::Device                     device      ; ///< The device to use for all vulkan calls.
      vk::SurfaceKHR                   vk_surface  ; ///< The surface to use for generating a device.
      nyx::Window<Impl>                window      ; ///< The implementation window object.
      Impl::Queue                      queue       ; ///< The present queue to use for swapchain creation.
      Impl::Swapchain                  swapchain   ; ///< The swapchain object to manage rendering to this window.
      Impl::RenderPass                 pass        ; ///< The render pass object to use for handle drawing to the window.
      std::string                      title       ; ///< The string title of this window.
      unsigned                         device_id   ; ///< The id of the device to use for this object's GPU.
      unsigned                         width       ; ///< The width of this window in pixels.
      unsigned                         height      ; ///< The height of this window in pixels.
      std::string                      module_name ; ///< The name of this module.
      std::mutex                       mutex       ; ///< Mutex to ensure images and synchronizations arent set while this module is processing.

      /** Default constructor.
       */
      VkWindowData() ;
      
      /** Method to set the device id to use for this object.
       * @param id The id of the vulkan device to use.
       */
      void setDeviceId( unsigned id ) ;

      /** Method to set the device for this object to use.
       * @param id The id of device created.
       * @param device The const-reference to the device.
       */
      void setDevice( unsigned id, const nyx::vkg::Device& device ) ;

      /** Method to input an image to draw to this window's current vulkan swapchain image.
       * @param The image to draw to this object.
       */
      void draw( const nyx::vkg::Image& image ) ;
      
      /** Method to input an sync to wait on for this window's current vulkan swapchain image.
       * @param The synchronization object to wait on.
       */
      void wait( const nyx::vkg::Synchronization& sync ) ;

      /** Method to set the width of the window in pixels.
       * @param width The width in pixels of the window.
       */
      void setWidth( unsigned width ) ;
      
      /** Method to set the height of the window in pixels.
       * @param height The height of the window in pixels
       */
      void setHeight( unsigned height ) ;
      
      /** Method to set the title of the window.
       * @param title The string title of this window.
       */
      void setTitle( const char* title ) ;
      
      /** Method to set the name of the input image to copy into this object's image buffer.
       * @return The name to associate with this object's input image.
       */
      void setInputName( const char* name ) ;
      
      /** Method to set the name of the input synchronization object.
       * @param name The name to associate with the input synchronization of this object.
       */
      void setInputSyncName( const char* name ) ;

      /** Method to set the output name of this object.
       * The name of the output of this module.
       */
      void setOutputName( const char* name ) ;

      /** Method to retrieve const reference to the surface of this object;
       * @return Const reference to a kgl device from this object.
       */
      const vk::SurfaceKHR& surface() ;
    };
    
    VkWindowData::VkWindowData()
    {
      this->device_id = 0            ;
      this->width     = 1024         ;
      this->height    = 720          ;
      this->title     = "OGM_Window" ;
    }
    
    const vk::SurfaceKHR& VkWindowData::surface()
    {
      return this->vk_surface ;
    }
    
    void VkWindowData::setDeviceId( unsigned id )
    {
      this->device_id = id ;
    }
    
    void VkWindowData::setInputName( const char* name )
    {
      this->bus.enroll( this, &VkWindowData::draw, iris::REQUIRED, name ) ;
    }
    
    void VkWindowData::setInputSyncName( const char* name )
    {
      this->bus.enroll( this, &VkWindowData::wait, iris::REQUIRED, name ) ;
    }
    
    void VkWindowData::setDevice( unsigned id, const nyx::vkg::Device& device )
    {
      if( id == this->device_id )
      {
        this->device = device ;
        
        this->queue = this->device.presentQueue() ;
        this->swapchain.initialize( this->queue, this->surface()                       ) ;
        this->pass     .initialize( this->swapchain                                    ) ;
        this->cmds     .initialize( this->pass.numFramebuffers() + 1, this->queue, 1   ) ;
        this->syncs    .initialize( this->pass.numFramebuffers() + 1, this->device, 0  ) ;
        
        this->syncs.current().waitOn( this->swapchain.acquire() ) ;
      }
    }
    
    void VkWindowData::draw( const nyx::vkg::Image& image )
    {
      this->mutex.lock() ;
      auto img = this->swapchain.image( this->swapchain.current() ) ;
      img.copy( image, this->cmds ) ;
      // maybe have to wait on fence here?

      this->cmds.advance() ;
      this->mutex.unlock() ;
    }
    
    void VkWindowData::wait( const nyx::vkg::Synchronization& sync )
    {
      this->mutex.lock() ;
      this->syncs.current().waitOn( sync ) ;
      this->mutex.unlock() ;
    }
    
    void VkWindowData::setOutputName( const char* name )
    {
      this->bus.publish( this, &VkWindowData::surface, this->module_name.c_str(), "::", name ) ;
    }
    
    void VkWindowData::setWidth( unsigned width )
    {
      this->width = width ;
      this->window.setWidth( width ) ;
    }
    
    void VkWindowData::setHeight( unsigned height )
    {
      this->height = height ;
      this->window.setHeight( height ) ;
    }
    
    void VkWindowData::setTitle( const char* title )
    {
      this->title = title ;
    }
    
    VkWindow::VkWindow()
    {
      this->module_data = new VkWindowData() ;
    }

    VkWindow::~VkWindow()
    {
      delete this->module_data ;
    }

    void VkWindow::initialize()
    {
      data().window.initialize( data().title.c_str(), data().width, data().height ) ;
      
      data().vk_surface = data().window.context() ;
      
      data().bus.emit() ;
    }

    void VkWindow::subscribe( unsigned id )
    {
      using IMPL = nyx::vkg::Vulkan ;
      
      data().module_name = this->name() ;
      data().bus.setChannel( id ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setBorderless, iris::OPTIONAL, this->name(), "::borderless" ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setFullscreen, iris::OPTIONAL, this->name(), "::fullscreen" ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setResizable , iris::OPTIONAL, this->name(), "::resizable"  ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setOutputName     , iris::OPTIONAL, this->name(), "::output"     ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setInputName      , iris::OPTIONAL, this->name(), "::input"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setInputSyncName  , iris::OPTIONAL, this->name(), "::sync_input" ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setWidth          , iris::OPTIONAL, this->name(), "::width"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setDeviceId       , iris::OPTIONAL, this->name(), "::device_id"  ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setHeight         , iris::OPTIONAL, this->name(), "::height"     ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setTitle          , iris::OPTIONAL, this->name(), "::title"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setTitle          , iris::OPTIONAL, this->name(), "::title"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setDevice         , iris::OPTIONAL,               "vkg_device"   ) ;
    }

    void VkWindow::shutdown()
    {
      data().cmds  .reset() ;
      data().syncs .reset() ;
      data().window.reset() ;
    }

    void VkWindow::execute()
    {
      data().bus.wait() ;
      
      data().mutex.lock() ;
      
      data().queue.submit( data().cmds, data().syncs ) ;
      data().swapchain.submit( data().syncs ) ;
      data().syncs.current().clear() ;
      data().syncs.advance() ;

      data().syncs.current().waitOn( data().swapchain.acquire() ) ;
      
      data().mutex.unlock() ;
    }

    VkWindowData& VkWindow::data()
    {
      return *this->module_data ;
    }

    const VkWindowData& VkWindow::data() const
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
  return "Vulkan Window" ;
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
  return new nyx::vkg::VkWindow() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::VkWindow* mod ;
  
  mod = dynamic_cast<nyx::vkg::VkWindow*>( module ) ;
  delete mod ;
}