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
#include "shader.h"
#include <nyx/template/List.h>
#include <nyx/vkg/Vulkan.h>
#include <nyx/NyxFile.h>
#include <nyx/library/Renderer.h>
#include <nyx/library/Window.h>
#include <nyx/library/Array.h>
#include <nyx/event/Event.h>
#include <nyx/library/Image.h>
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
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
      struct Vertex
      {
        float x  ;
        float y  ;
        float tx ;
        float ty ;
      };
      
      static constexpr Vertex   vertex_data[] = { { -1.0f, -1.0f, 0.0f, 0.0f }, { 1.0f, -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 0.0f, 1.0f } } ;
      static constexpr unsigned index_data [] = { 0, 1, 3, 1, 2, 3 } ;
      nyx::Renderer<Impl, nyx::ImageFormat::RGBA8> renderer ;
      
      nyx::EventManager                manager     ;
      iris::Bus                        bus         ; ///< The bus to communicate over.
      nyx::RGBAImage<Impl>             image       ; ///< The staging image.
      Impl::Array<Vertex>              vertices    ; ///< The vertices to use for drawing.
      Impl::Array<unsigned>            indices     ; ///< The indices to use for drawing.
      Impl::Queue                      queue       ; ///< The queue to use for gpu operations.
      unsigned                         device      ; ///< The device to use for all vulkan calls.
      vk::SurfaceKHR                   vk_surface  ; ///< The surface to use for generating a device.
      nyx::Window<Impl>                window      ; ///< The implementation window object.
      std::string                      title       ; ///< The string title of this window.
      unsigned                         width       ; ///< The width of this window in pixels.
      unsigned                         height      ; ///< The height of this window in pixels.
      std::string                      module_name ; ///< The name of this module.
      std::mutex                       mutex       ; ///< Mutex to ensure images and synchronizations arent set while this module is processing.
      bool                             should_exit ;

      /** Default constructor.
       */
      VkWindowData() ;

      /** Method to set the device for this object to use.
       * @param id The id of device created.
       * @param device The const-reference to the device.
       */
      void setDevice( unsigned id ) ;
      
      /** Method to retrieve the exit flag of this obejct.
       */
      bool exit() ;

      /** Method to recieve an exit signal from the input window.
       * @param The exit signal.
       */
      void handleExit( const nyx::Event& event ) ;

      /** Method to set whether to quit the iris runtime when exitting this window.
       * @param exit Whether to exit the iris runtime when exitting this window.
       */
      void shouldExitOnExit( bool exit ) ;

      /** Method to initialize the GPU buffers of this object.
       */
      void initBuffers() ;
      
      /** Method to input an image to draw to this window's current vulkan swapchain image.
       * @param The image to draw to this object.
       */
      void draw( const nyx::RGBAImage<Impl>& image ) ;
      
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
      void setInputName( unsigned idx, const char* name ) ;
      
      /** Method to set the output name of this object.
       * The name of the output of this module.
       */
      void setOutputName( unsigned idx, const char* name ) ;
    };
    
    VkWindowData::VkWindowData()
    {
      this->should_exit = false        ;
      this->width       = 1024         ;
      this->height      = 720          ;
      this->title       = "OGM_Window" ;
      this->device      = 0            ;
    }
    
    bool VkWindowData::exit()
    {
      return this->should_exit ;
    }

    void VkWindowData::handleExit( const nyx::Event& event )
    {
      event.type() ;
      this->should_exit = true ;
    }

    void VkWindowData::shouldExitOnExit( bool exit )
    {
      if( exit )
      {
        this->bus.publish( this, &VkWindowData::exit, "Iris::Exit::Flag" ) ;
      }
    }

    void VkWindowData::initBuffers()
    {
      Impl::Array<Vertex  > staging_1 ;
      Impl::Array<unsigned> staging_2 ;
      
      staging_1.initialize( device, 4, true, nyx::ArrayFlags::TransferSrc ) ;
      staging_2.initialize( device, 6, true, nyx::ArrayFlags::TransferSrc ) ;
      this->vertices.initialize( device, 4, false, nyx::ArrayFlags::Vertex | nyx::ArrayFlags::TransferDst ) ;
      this->indices .initialize( device, 6, false, nyx::ArrayFlags::Index  | nyx::ArrayFlags::TransferDst ) ;
      
      staging_1.copyToDevice( this->vertex_data, 4 ) ;
      staging_2.copyToDevice( this->index_data , 6 ) ;

      this->vertices.copy( staging_1, 4, this->queue ) ;
      this->indices .copy( staging_2, 6, this->queue ) ;
      this->image.transition( nyx::ImageLayout::ShaderRead, this->queue ) ;
      Impl::deviceSynchronize( this->device ) ;
    }

    void VkWindowData::setInputName( unsigned idx, const char* name )
    {
      switch( idx )
      {
        default: this->bus.enroll( this, &VkWindowData::draw, iris::REQUIRED, name ) ;
      }
    }
    
    void VkWindowData::setDevice( unsigned id )
    {
      this->device = id ;
    }
    
    void VkWindowData::draw( const nyx::RGBAImage<Impl>& image )
    {
      this->mutex.lock() ;
      
      if( this->image.resize( image.width(), image.height() ) )
      {
        this->renderer.bind( "framebuffer", this->image ) ;
      }

      this->image.copy( image, this->queue ) ;
      this->mutex.unlock() ;
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
      this->window.setTitle( title ) ;
    }
    
    VkWindow::VkWindow()
    {
      this->module_data = new VkWindowData() ;
      
      Impl::addDeviceExtension  ( "VK_KHR_swapchain"                        ) ;
      Impl::addInstanceExtension( "VK_KHR_surface"                          ) ;
      Impl::addInstanceExtension( Impl::platformSurfaceInstanceExtensions() ) ;
    }

    VkWindow::~VkWindow()
    {
      delete this->module_data ;
    }

    void VkWindow::initialize()
    {
      data().window.initialize( data().title.c_str(), data().width, data().height ) ;
      data().vk_surface = data().window.context() ;
      data().queue      = Impl::graphicsQueue( data().device ) ;
      data().renderer .initialize( data().device, nyx::bytes::window, sizeof( nyx::bytes::window ), data().vk_surface ) ;
      data().image    .initialize( data().device, data().window.width(), data().window.height()                       ) ;
      
      data().initBuffers() ;
      
      data().renderer.bind( "framebuffer", data().image ) ;

      Impl::deviceSynchronize( data().device ) ;
    }

    void VkWindow::subscribe( unsigned id )
    {
      using IMPL = nyx::vkg::Vulkan ;
      
      data().module_name = this->name() ;
      data().bus.setChannel( id ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setBorderless, iris::OPTIONAL, this->name(), "::borderless" ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setFullscreen, iris::OPTIONAL, this->name(), "::fullscreen" ) ;
      data().bus.enroll( &data().window   , &nyx::Window<IMPL>::setResizable , iris::OPTIONAL, this->name(), "::resizable"  ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setInputName      , iris::OPTIONAL, this->name(), "::inputs"     ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setWidth          , iris::OPTIONAL, this->name(), "::width"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setDevice         , iris::OPTIONAL, this->name(), "::device"     ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setHeight         , iris::OPTIONAL, this->name(), "::height"     ) ;
      data().bus.enroll( this->module_data, &VkWindowData::setTitle          , iris::OPTIONAL, this->name(), "::title"      ) ;
      data().bus.enroll( this->module_data, &VkWindowData::shouldExitOnExit  , iris::OPTIONAL, this->name(), "::quit_iris"  ) ;

      data().manager.enroll( this->module_data, &VkWindowData::handleExit, nyx::Event::Type::WindowExit, this->name() ) ;
    }

    void VkWindow::shutdown()
    {
      data().window  .reset() ;
//      data().renderer.reset() ;
    }

    void VkWindow::execute()
    {
      data().bus.wait() ;
      
      data().mutex.lock() ;
      iris::log::Log::output( this->name(), " copying image to window." ) ;
      data().window.handleEvents() ;
      
      data().renderer.drawIndexed( data().indices, data().vertices ) ;
      data().renderer.finalize() ;
      data().mutex.unlock() ;

      data().bus.emit() ;
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