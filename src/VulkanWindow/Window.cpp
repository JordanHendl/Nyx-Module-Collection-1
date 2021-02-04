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
#include <nyx/template/List.h>
#include <nyx/vkg/Vulkan.h>
#include <nyx/NyxFile.h>
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
    
    const uint32_t vert_spirv[] = 
    {
      0x07230203,0x00010000,0x00080008,0x00000020,0x00000000,0x00020011,0x00000001,0x0006000b,
      0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
      0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000c,0x00000014,
      0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,0x72617065,0x5f657461,
      0x64616873,0x6f5f7265,0x63656a62,0x00007374,0x00040005,0x00000004,0x6e69616d,0x00000000,
      0x00050005,0x00000009,0x5f786574,0x726f6f63,0x00007364,0x00040005,0x0000000c,0x74726576,
      0x00007865,0x00060005,0x00000012,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,
      0x00000012,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x00000012,0x00000001,
      0x505f6c67,0x746e696f,0x657a6953,0x00000000,0x00070006,0x00000012,0x00000002,0x435f6c67,
      0x4470696c,0x61747369,0x0065636e,0x00070006,0x00000012,0x00000003,0x435f6c67,0x446c6c75,
      0x61747369,0x0065636e,0x00030005,0x00000014,0x00000000,0x00040047,0x00000009,0x0000001e,
      0x00000000,0x00040047,0x0000000c,0x0000001e,0x00000000,0x00050048,0x00000012,0x00000000,
      0x0000000b,0x00000000,0x00050048,0x00000012,0x00000001,0x0000000b,0x00000001,0x00050048,
      0x00000012,0x00000002,0x0000000b,0x00000003,0x00050048,0x00000012,0x00000003,0x0000000b,
      0x00000004,0x00030047,0x00000012,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,
      0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,
      0x00040020,0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,
      0x00040017,0x0000000a,0x00000006,0x00000004,0x00040020,0x0000000b,0x00000001,0x0000000a,
      0x0004003b,0x0000000b,0x0000000c,0x00000001,0x00040015,0x0000000f,0x00000020,0x00000000,
      0x0004002b,0x0000000f,0x00000010,0x00000001,0x0004001c,0x00000011,0x00000006,0x00000010,
      0x0006001e,0x00000012,0x0000000a,0x00000006,0x00000011,0x00000011,0x00040020,0x00000013,
      0x00000003,0x00000012,0x0004003b,0x00000013,0x00000014,0x00000003,0x00040015,0x00000015,
      0x00000020,0x00000001,0x0004002b,0x00000015,0x00000016,0x00000000,0x0004002b,0x00000006,
      0x00000019,0x00000000,0x0004002b,0x00000006,0x0000001a,0x3f800000,0x00040020,0x0000001e,
      0x00000003,0x0000000a,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
      0x00000005,0x0004003d,0x0000000a,0x0000000d,0x0000000c,0x0007004f,0x00000007,0x0000000e,
      0x0000000d,0x0000000d,0x00000002,0x00000003,0x0003003e,0x00000009,0x0000000e,0x0004003d,
      0x0000000a,0x00000017,0x0000000c,0x0007004f,0x00000007,0x00000018,0x00000017,0x00000017,
      0x00000000,0x00000001,0x00050051,0x00000006,0x0000001b,0x00000018,0x00000000,0x00050051,
      0x00000006,0x0000001c,0x00000018,0x00000001,0x00070050,0x0000000a,0x0000001d,0x0000001b,
      0x0000001c,0x00000019,0x0000001a,0x00050041,0x0000001e,0x0000001f,0x00000014,0x00000016,
      0x0003003e,0x0000001f,0x0000001d,0x000100fd,0x00010038
    };
    
    const uint32_t frag_spirv[] = 
    {
      0x07230203,0x00010000,0x00080008,0x0000001c,0x00000000,0x00020011,0x00000001,0x0006000b,
      0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
      0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000011,0x00000015,0x00030010,
      0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,
      0x72617065,0x5f657461,0x64616873,0x6f5f7265,0x63656a62,0x00007374,0x00040005,0x00000004,
      0x6e69616d,0x00000000,0x00040005,0x00000009,0x6f6c6f63,0x00000072,0x00050005,0x0000000d,
      0x6d617266,0x66756265,0x00726566,0x00050005,0x00000011,0x5f786574,0x726f6f63,0x00007364,
      0x00050005,0x00000015,0x5f74756f,0x6f6c6f63,0x00000072,0x00040047,0x0000000d,0x00000022,
      0x00000000,0x00040047,0x0000000d,0x00000021,0x00000000,0x00040047,0x00000011,0x0000001e,
      0x00000000,0x00040047,0x00000015,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,
      0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,
      0x00000004,0x00040020,0x00000008,0x00000007,0x00000007,0x00090019,0x0000000a,0x00000006,
      0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000b,
      0x0000000a,0x00040020,0x0000000c,0x00000000,0x0000000b,0x0004003b,0x0000000c,0x0000000d,
      0x00000000,0x00040017,0x0000000f,0x00000006,0x00000002,0x00040020,0x00000010,0x00000001,
      0x0000000f,0x0004003b,0x00000010,0x00000011,0x00000001,0x00040020,0x00000014,0x00000003,
      0x00000007,0x0004003b,0x00000014,0x00000015,0x00000003,0x00050036,0x00000002,0x00000004,
      0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000008,0x00000009,0x00000007,
      0x0004003d,0x0000000b,0x0000000e,0x0000000d,0x0004003d,0x0000000f,0x00000012,0x00000011,
      0x00050057,0x00000007,0x00000013,0x0000000e,0x00000012,0x0003003e,0x00000009,0x00000013,
      0x0004003d,0x00000007,0x00000016,0x00000009,0x00050051,0x00000006,0x00000017,0x00000016,
      0x00000000,0x00050051,0x00000006,0x00000018,0x00000016,0x00000001,0x00050051,0x00000006,
      0x00000019,0x00000016,0x00000002,0x00050051,0x00000006,0x0000001a,0x00000016,0x00000003,
      0x00070050,0x00000007,0x0000001b,0x00000017,0x00000018,0x00000019,0x0000001a,0x0003003e,
      0x00000015,0x0000001b,0x000100fd,0x00010038
    };

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
      nyx::EventManager                manager     ;
      nyx::List<Impl::Synchronization> syncs       ; ///< The synchronization objects to use for this object's synchronization.
      nyx::List<Impl::CommandRecord  > cmds        ; ///< The command buffers to record commands into.
      nyx::List<Impl::Descriptor     > descriptors ; ///< The descriptors to use for rendering.
      iris::Bus                        bus         ; ///< The bus to communicate over.
      nyx::RGBAImage<Impl>             image       ; ///< The staging image.
      Impl::Array<Vertex>              vertices    ; ///< The vertices to use for drawing.
      Impl::Array<unsigned>            indices     ; ///< The indices to use for drawing.
      unsigned                         device      ; ///< The device to use for all vulkan calls.
      vk::SurfaceKHR                   vk_surface  ; ///< The surface to use for generating a device.
      nyx::Window<Impl>                window      ; ///< The implementation window object.
      Impl::DescriptorPool             pool        ; ///< The pool to grab descriptors from.
      Impl::Pipeline                   pipeline    ; ///< The pipeline to use for rendering.
      Impl::Shader                     shader      ; ///< The object to contain all GPU Shader data.
      Impl::Queue                      queue       ; ///< The present queue to use for swapchain creation.
      Impl::Swapchain                  swapchain   ; ///< The swapchain object to manage rendering to this window.
      Impl::RenderPass                 pass        ; ///< The render pass object to use for handle drawing to the window.
      std::string                      title       ; ///< The string title of this window.
      unsigned                         width       ; ///< The width of this window in pixels.
      unsigned                         height      ; ///< The height of this window in pixels.
      std::string                      module_name ; ///< The name of this module.
      std::mutex                       mutex       ; ///< Mutex to ensure images and synchronizations arent set while this module is processing.
      unsigned                             should_exit ;

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
      unsigned exit() ;

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
      
      /** Method to remake the object needed for a recreated swapchain.
       */
      void remakeObjects() ;
      

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
      this->should_exit = 0            ;
      this->width       = 1024         ;
      this->height      = 720          ;
      this->title       = "OGM_Window" ;
      this->device      = 0            ;
    }
    
    unsigned VkWindowData::exit()
    {
      return this->should_exit ;
    }

    void VkWindowData::handleExit( const nyx::Event& event )
    {
      event.type() ;
      if( this->should_exit == 1 )
      {
        ::exit( -1 ) ;
      }
    }

    void VkWindowData::shouldExitOnExit( bool exit )
    {
      if( exit )
      {
        this->should_exit = 1 ;
        this->bus.publish( this, &VkWindowData::exit, "Iris::Exit::Flag" ) ;
      }
    }

    void VkWindowData::remakeObjects()
    {
      this->pass    .reset() ;
      this->pipeline.reset() ;
      this->cmds    .reset() ;

      this->pass    .initialize( this->swapchain                                  ) ;
      this->pipeline.initialize( this->pass, this->shader                         ) ;
      this->cmds    .initialize( this->pass.numFramebuffers() + 1, this->queue, 1 ) ;
    }

    void VkWindowData::initBuffers()
    {
      Impl::Array<Vertex  > staging_1 ;
      Impl::Array<unsigned> staging_2 ;
      Impl::CommandRecord   cmd_buff  ;
      
      cmd_buff.initialize( this->queue, 1 ) ;
      
      staging_1.initialize( device, 4, true, nyx::ArrayFlags::TransferSrc ) ;
      staging_2.initialize( device, 6, true, nyx::ArrayFlags::TransferSrc ) ;
      this->vertices.initialize( device, 4, false, nyx::ArrayFlags::Vertex | nyx::ArrayFlags::TransferDst ) ;
      this->indices .initialize( device, 6, false, nyx::ArrayFlags::Index  | nyx::ArrayFlags::TransferDst ) ;
      
      staging_1.copyToDevice( this->vertex_data, 4 ) ;
      staging_2.copyToDevice( this->index_data , 6 ) ;

      cmd_buff.record() ;
      this->vertices.copy( staging_1, cmd_buff, 4 ) ;
      this->indices .copy( staging_2, cmd_buff, 6 ) ;
      cmd_buff.stop() ;
      
      this->queue.submit( cmd_buff ) ;
      this->image.transition( nyx::ImageLayout::ShaderRead, this->queue ) ;
      Impl::deviceSynchronize( this->device ) ;
      staging_1.reset() ;
      staging_2.reset() ;
      cmd_buff .reset() ;
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
    }
    
    VkWindow::VkWindow()
    {
      this->module_data = new VkWindowData() ;
      
      Impl::addDeviceExtension( "VK_KHR_swapchain"                          ) ;
      Impl::addInstanceExtension( Impl::platformSurfaceInstanceExtensions() ) ;
      Impl::addInstanceExtension( "VK_KHR_surface"                          ) ;
    }

    VkWindow::~VkWindow()
    {
      delete this->module_data ;
    }

    void VkWindow::initialize()
    {
      data().window.initialize( data().title.c_str(), data().width, data().height ) ;
      
      data().vk_surface = data().window.context() ;
      
      data().syncs.initialize( data().pass.numFramebuffers() + 1, data().device, 0  ) ;
      
      data().queue = Impl::presentQueue( data().vk_surface, data().device ) ;
      data().pass.setAttachmentStoreOp( vk::AttachmentStoreOp::eStore ) ;
      data().pass.setFinalLayout      ( nyx::ImageLayout::PresentSrc  ) ;
      data().pass.setScissorExtentX   ( data().width                   ) ;
      data().pass.setScissorExtentY   ( data().height                  ) ;
      
      data().shader.addAttribute   ( 0, 0, Impl::Shader::Format::vec4                             ) ;
      data().shader.addInputBinding( 0, sizeof( float ) * 4, Impl::Shader::InputRate::Vertex      ) ;
      data().shader.addDescriptor  ( 0, nyx::ImageUsage::Sampled, 1, nyx::ShaderStage::Fragment   ) ;
      data().shader.addShaderModule( nyx::ShaderStage::Vertex  , vert_spirv, sizeof( vert_spirv ) ) ;
      data().shader.addShaderModule( nyx::ShaderStage::Fragment, frag_spirv, sizeof( frag_spirv ) ) ;
      data().pool  .addImageInput  ( "framebuffer", 0, nyx::ImageUsage::Sampled                   ) ;
      
      data().shader     .initialize( data().device                                                      ) ;
      data().pool       .initialize( data().shader, 50                                                  ) ;
      data().swapchain  .initialize( data().queue, data().vk_surface                                    ) ;
      data().pass       .initialize( data().swapchain                                                   ) ;
      data().pipeline   .initialize( data().pass, data().shader                                         ) ;
      data().cmds       .initialize( data().pass.numFramebuffers() + 1, data().queue, 1                 ) ;
      data().descriptors.initialize( 50, data().pool                                                    ) ;
      data().image      .initialize( data().device, data().swapchain.width(), data().swapchain.height() ) ;
      
      data().initBuffers() ;
      
      for( unsigned i = 0; i < 50; i++ )
      { 
        data().descriptors.current().set( "framebuffer", data().image ) ;
        data().descriptors.advance() ;
      }

      if( data().swapchain.acquire() == Impl::Error::RecreateSwapchain )
      {
        data().remakeObjects() ;
      }

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
      data().cmds  .reset() ;
      data().syncs .reset() ;
      data().window.reset() ;
    }

    void VkWindow::execute()
    {
      data().bus.wait() ;
      
      data().mutex.lock() ;
      iris::log::Log::output( this->name(), " copying image to window." ) ;
      
      data().window.handleEvents() ;
      data().cmds.current().record( data().pass ) ;
      data().cmds.current().bind( data().pipeline    ) ;
      data().cmds.current().bind( data().descriptors ) ;
      data().cmds.current().drawIndexed( data().indices, data().vertices ) ;
      data().cmds.current().stop() ;
      
      data().queue.submit( data().cmds  ) ;

      if( data().swapchain.submit() == Impl::Error::RecreateSwapchain )
      {
        data().remakeObjects() ;
      }
      
      data().syncs.current().clear() ;
      
      data().syncs      .advance() ;
      data().cmds       .advance() ;
      data().descriptors.advance() ;
      
      if( data().swapchain.acquire() == Impl::Error::RecreateSwapchain)
      {
        data().remakeObjects() ;
      }
      
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