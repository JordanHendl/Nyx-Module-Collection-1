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
#include <nyxgpu/vkg/Vulkan.h>
#include <nyxgpu/event/Event.h>
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <limits>

static const unsigned VERSION = 1 ;

namespace nyx
{
  namespace vkg
  {
    using Impl = nyx::vkg::Vulkan ;
    using Log  = iris::log::Log   ;
    
    /** Structure to contain a window module's data.
     */
    struct NyxWindowData
    {
      nyx::EventManager manager       ; ///< The object to manage events produced from this window.
      iris::Bus         bus           ; ///< The bus to communicate over.
      std::string       title         ; ///< The string title of this window.
      unsigned          width         ; ///< The width of this window in pixels.
      unsigned          height        ; ///< The height of this window in pixels.
      std::string       module_name   ; ///< The name of this module.
      bool              should_exit   ; ///< Whether or not this object should quit the iris engine when it's exit button it pressed.
      unsigned          id            ; ///< The window id.
      bool              capture_mouse ; ///< Whether or not the mouse should be captures by this window.
      bool              initialized   ; ///< Whether or not this object is initialized.

      /** Default constructor.
       */
      NyxWindowData() ;
      
      /** Method to set the id of this object.
       * @param id The id to associate with this object's input.
       */
      void setId( unsigned id ) ;
      
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

      /** Method to set the width of the window in pixels.
       * @param width The width in pixels of the window.
       */
      void setWidth( unsigned width ) ;
      
      /** Method to set whether or not the mouse should be captured by this object.
       * @param capture Whether or not the mouse is to be captures.
       */
      void setMouseCapture( bool capture ) ;

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
      
      /** Method to set the output name of this object.
       * The name of the output of this module.
       */
      void setOutputName( const char* name ) ;
      
      bool signal() ;
      void recieve() ;
    };
    
    NyxWindowData::NyxWindowData()
    {
      this->should_exit   = false      ;
      this->width         = 1024       ;
      this->height        = 720        ;
      this->title         = "Window"   ;
      this->id            = UINT32_MAX ;
      this->initialized   = false      ;
      this->capture_mouse = false      ;
    }
    
    void NyxWindowData::setMouseCapture( bool capture )
    {
      Log::output( "Module ", this->module_name.c_str(), " set capture mouse input to ", capture ) ;
      this->capture_mouse = capture ;
      if( this->initialized )
      {
        Impl::setWindowMouseCapture( this->id, true ) ;
      }
    }
    
    bool NyxWindowData::exit()
    {
      return this->should_exit ;
    }

    void NyxWindowData::handleExit( const nyx::Event& event )
    {
      event.type() ;
      this->should_exit = true ;
    }

    bool NyxWindowData::signal()
    {
      return true ;
    }
    
    void NyxWindowData::recieve()
    {
      
    }
    
    void NyxWindowData::setId( unsigned id )
    {
      this->id = id ;
    }

    void NyxWindowData::shouldExitOnExit( bool exit )
    {
      Log::output( "Module ", this->module_name.c_str(), " set whether it should exit Iris to ", exit ) ;
      if( exit )
      {
        this->bus.publish( this, &NyxWindowData::exit, "Iris::Exit::Flag" ) ;
      }
    }

    void NyxWindowData::setInputName( const char* name )
    {
      Log::output( "Module ", this->module_name.c_str(), " set input signal to ", name ) ;
      this->bus.enroll( this, &NyxWindowData::recieve, iris::REQUIRED, name ) ;
    }
    
    void NyxWindowData::setOutputName( const char* name )
    {
      Log::output( "Module ", this->module_name.c_str(), " set output signal to ", name ) ;
      this->bus.publish( this, &NyxWindowData::signal, name ) ;
    }
    
    void NyxWindowData::setWidth( unsigned width )
    {
      Log::output( "Module ", this->module_name.c_str(), " set width to ", width ) ;
      this->width = width ;
      Impl::setWindowWidth( id, width ) ;
    }
    
    void NyxWindowData::setHeight( unsigned height )
    {
      Log::output( "Module ", this->module_name.c_str(), " set height to ", height ) ;
      this->height = height ;
      Impl::setWindowWidth( id, height ) ;
    }
    
    void NyxWindowData::setTitle( const char* title )
    {
      Log::output( "Module ", this->module_name.c_str(), " set title to ", title ) ;
      this->title = title ;
      Impl::setWindowTitle( id, title ) ;
    }
    
    Window::Window()
    {
      this->module_data = new NyxWindowData() ;
      
      Impl::addDeviceExtension  ( "VK_KHR_swapchain"                        ) ;
      Impl::addInstanceExtension( "VK_KHR_surface"                          ) ;
      Impl::addInstanceExtension( Impl::platformSurfaceInstanceExtensions() ) ;
    }

    Window::~Window()
    {
      delete this->module_data ;
    }

    void Window::initialize()
    {
      if( data().id != UINT32_MAX )
      {
        Impl::addWindow( data().id, data().title.c_str(), data().width, data().height ) ;
        if( data().capture_mouse ) Impl::setWindowMouseCapture( data().id, true ) ;
      }

      data().initialized = true ;
    }

    void Window::subscribe( unsigned id )
    {
      data().module_name = this->name() ;
      data().bus.setChannel( id ) ;
      data().bus.enroll( &Impl::setWindowBorderless, iris::OPTIONAL, this->name(), "::borderless" ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::setWidth          , iris::OPTIONAL, this->name(), "::width"         ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::setHeight         , iris::OPTIONAL, this->name(), "::height"        ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::setTitle          , iris::OPTIONAL, this->name(), "::title"         ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::setMouseCapture   , iris::OPTIONAL, this->name(), "::capture_mouse" ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::shouldExitOnExit  , iris::OPTIONAL, this->name(), "::quit_iris"     ) ;
      data().bus.enroll( this->module_data, &NyxWindowData::setId             , iris::OPTIONAL, this->name(), "::id"            ) ;
      
      data().manager.enroll( this->module_data, &NyxWindowData::handleExit, nyx::Event::Type::WindowExit, this->name() ) ;
    }

    void Window::shutdown()
    {
      
    }

    void Window::execute()
    {
      data().bus.wait() ;
      
      Impl::handleWindowEvents( data().id ) ;

      data().bus.emit() ;
    }

    NyxWindowData& Window::data()
    {
      return *this->module_data ;
    }

    const NyxWindowData& Window::data() const
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
  return "NyxWindow" ;
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
  return new nyx::vkg::Window() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::Window* mod ;
  
  mod = dynamic_cast<nyx::vkg::Window*>( module ) ;
  delete mod ;
}