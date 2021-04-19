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
 * File:   NyxStartDraw.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxStartDraw.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/profiling/Timer.h>
#include <iris/config/Parser.h>
#include <nyxgpu/library/Array.h>
#include <nyxgpu/library/Image.h>
#include <nyxgpu/library/Chain.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <glm/glm.hpp>
#include <limits>
#include <string>
#include <vector>
#include <climits>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    using          Log    = iris::log::Log          ;
    using          Impl   = nyx::vkg::Vulkan        ;
    constexpr auto Format = nyx::ImageFormat::RGBA8 ;
    
    static nyx::ImageLayout stringToLayout( std::string& str ) ;
    static nyx::ImageFormat stringToFormat( std::string& str ) ;
    
    
    nyx::ImageFormat stringToFormat( std::string& str )
    {
      if( str == "RGBA32F" ) return nyx::ImageFormat::RGBA32F ;
      if( str == "RGBA8"   ) return nyx::ImageFormat::RGBA8   ;
      if( str == "RGB8"    ) return nyx::ImageFormat::RGB8    ;
      return nyx::ImageFormat::RGBA8 ;
    }
    
    nyx::ImageLayout stringToLayout( std::string& str )
    {
      if( str == "Color" ) return nyx::ImageLayout::ColorAttachment ;
      return nyx::ImageLayout::ColorAttachment ;
    }

    template<unsigned ID>
    unsigned baseGetId()
    {
      return ID ;
    }

    struct NyxStartDrawData
    {
      using Subpasses = std::vector<nyx::Subpass> ;
      
      unsigned              window_id    ;
      nyx::RenderPass<Impl> render_pass  ;
      nyx::Chain<Impl>      render_chain ;
      unsigned              device       ;
      iris::Bus             bus          ;
      iris::Bus             wait_signal  ;
      std::string           name         ;
      unsigned              width        ;
      unsigned              height       ;
      unsigned              channels     ;
      Subpasses             subpasses    ;
      glm::mat4             proj         ;
      
      /** Default constructor.
       */
      NyxStartDrawData() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::Chain<Impl>& chain() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::RenderPass<Impl>& pass() ;
      
      /** Method to input the required subpasses of this object.
       * @param token The token to parse for the subpass.
       */
      void setInputSubpasses( const iris::config::json::Token& token ) ;
      
      /** Method to retrieve the projection of this object.
       * @return The reference to the projection of this object.
       */
      const glm::mat4& projection() ;
      
      /** Method to wait on an input.
       */
      void wait() ;
      
      /** Method to signal an output.
       */
      void signal() ;
      
      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutputName( unsigned idx, const char* name ) ;

      /** Method to set the name to associate with this object's projection output.
       * @param name The name to associate with this object's output projection.
       */
      void setOutputProjectionName( const char* name ) ;
      
      /** Method to set the name of the input width parameter.
       * @param name The name to associate with the input.
       */
      void setInputName( unsigned idx, const char* name ) ;
      
      /** Method to set the window id for this render pass to draw to.
       * @param id The id to use for drawing the window.
       */
      void setWindowId( unsigned id ) ;
      
      /** Set the name of the input signal to wait on.
       * @param name the signal name to wait on.
       */
      void setWaitName( const char* name ) ;
      
      /** Set the name of the output name to signal.
       * @param name the name to signal.
       */
      void setSignalName( const char* name ) ;

      /** Method to set the device to use for this module's operations.
       * @param device The device to set.
       */
      void setDevice( unsigned id ) ;
      
      /** Method to set the width of the render pass
       * @param width The width of the incoming image.
       */
      void setWidth( unsigned width ) ;
      
      /** Method to set the height of the render pass
       * @param height The height in pixels of the incoming image.
       */
      void setHeight( unsigned height ) ;
    };
    
    NyxStartDrawData::NyxStartDrawData()
    {
      this->width     = 0        ;
      this->height    = 0        ;
      this->name      = ""       ;
      this->device    = 0        ;
      this->window_id = UINT_MAX ;
    }
    
    const nyx::Chain<Impl>& NyxStartDrawData::chain()
    {
      return this->render_chain ;
    }
    
    const nyx::RenderPass<Impl>& NyxStartDrawData::pass()
    {
      return this->render_pass ;
    }
    const glm::mat4& NyxStartDrawData::projection()
    {
      return this->proj ;
    }
    
    void NyxStartDrawData::setOutputProjectionName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output projection as \"", name, "\"" ) ;
      this->bus.publish( this, &NyxStartDrawData::projection, name ) ;
    }
    
    void NyxStartDrawData::setOutputName( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output ", idx, " as \"", name, "\"" ) ;
      
      this->bus.publish( this, &NyxStartDrawData::chain, name ) ;
      this->bus.publish( this, &NyxStartDrawData::pass , name ) ;
      switch( idx )
      {
        case 0  : this->bus.publish( &vkg::baseGetId<0>, name ) ; break ;
        case 1  : this->bus.publish( &vkg::baseGetId<1>, name ) ; break ;
        case 2  : this->bus.publish( &vkg::baseGetId<2>, name ) ; break ;
        case 3  : this->bus.publish( &vkg::baseGetId<3>, name ) ; break ;
        case 4  : this->bus.publish( &vkg::baseGetId<4>, name ) ; break ;
        default : break ;
      }
      
      this->bus.emit() ;
    }
    
    void NyxStartDrawData::setInputSubpasses( const iris::config::json::Token& token )
    {
      nyx::Subpass    subpass           ;
      nyx::Attachment attachment        ;
      std::string     format_str        ;
      std::string     layout_str        ;
      float           clear_colors[ 4 ] ;
      bool            reinit            ;
      
      reinit = false ;
      if( this->render_pass.initialized() ) reinit = true ;
      
      this->render_pass .reset() ;
      this->render_chain.reset() ;
      
      // For each subpass.
      for( unsigned subpass_index = 0; subpass_index < token.size(); subpass_index++ )
      {
        auto subpass_token = token.token( subpass_index ) ;
        
        const auto depth_enable = subpass_token[ "depth_enable" ] ;
        const auto depends      = subpass_token[ "depends"      ] ;
        const auto attachments  = subpass_token[ "attachments"  ] ;
        
        subpass = nyx::Subpass() ;
       
        if( depth_enable ) subpass.setDepthStencilEnable( depth_enable.boolean() ) ;
        
        if( depends )
        {
          for( unsigned index = 0; index < depends.size(); index++ ) subpass.addSubpassDependancy( depends.number( index ) ) ;
        }
        
        if( attachments )
        {
          // For each attachment.
          for( unsigned index = 0; index < attachments.size(); index++ )
          {
            attachment = nyx::Attachment() ;
            
            auto curr_attach = attachments.token( index ) ;
            
            auto format        = curr_attach[ "format"        ] ;
            auto stencil_clear = curr_attach[ "stencil_clear" ] ;
            auto layout        = curr_attach[ "layout"        ] ;
            auto clear_color   = curr_attach[ "clear_color"   ] ;
            
            if( format        ) format_str = format.string()                          ;
            if( layout        ) layout_str = layout.string()                          ;
            if( stencil_clear ) attachment.setStencilClear( stencil_clear.boolean() ) ;
            if( clear_color   )
            {
              for( unsigned clear_index = 0; clear_index < clear_color.size(); clear_index++ )
              {
                if( clear_index < 4 )
                {
                  clear_colors[ clear_index ] = clear_color.decimal( index ) ;
                }
              }
            }
            
            attachment.setFormat    ( stringToFormat( format_str ) ) ;
            attachment.setLayout    ( stringToLayout( layout_str ) ) ;
            attachment.setClearColor( clear_colors[ 0 ],
                                      clear_colors[ 1 ],
                                      clear_colors[ 2 ],
                                      clear_colors[ 3 ] ) ;
            
            Log::output( "\n Module ", this->name.c_str(), " subpass ", subpass_index, " attachment ", index, ": \n",
                         "---- format      : ", format_str.c_str()                                                                       , "\n",
                         "---- layout      : ", layout_str.c_str()                                                                       , "\n",
                         "---- stencil     : ", attachment.clearStencil()                                                                , "\n",
                         "---- clear_colors: ", clear_colors[ 0 ], " ", clear_colors[ 1 ], " ", clear_colors[ 2 ], " ", clear_colors[ 3 ], "\n" ) ;
            
          }
          subpass.addAttachment( attachment ) ;
        }
        
        this->subpasses.push_back( subpass ) ;
      }
      
      if( reinit )
      {
        if( this->window_id != UINT32_MAX )
        {
          this->render_pass .initialize( this->device ) ;
        }
        else
        {
          this->render_pass .initialize( this->device, this->window_id ) ;
        }
        
        this->bus.emit() ;
      }
    }
    
    void NyxStartDrawData::wait()
    {
      
    }
    
    void NyxStartDrawData::signal()
    {
      
    }
    
    void NyxStartDrawData::setWindowId( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set window id as ", id ) ;
      this->window_id = id ;
    }
    
    void NyxStartDrawData::setWaitName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set to WAIT ON signal ", name, " to perform it's operation." ) ;
      this->wait_signal.enroll( this, &NyxStartDrawData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxStartDrawData::setSignalName( const char* name ) 
    {
      Log::output( "Module ", this->name.c_str(), " set to SIGNAL ", name, " whenever it's finished." ) ;
      this->wait_signal.publish( this, &NyxStartDrawData::signal, name ) ;
    }
    
    void NyxStartDrawData::setDevice( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set device as ", id ) ;
      this->device = id ;
    }
    
    void NyxStartDrawData::setWidth( unsigned width )
    {
      Log::output( "Module ", this->name.c_str(), " set width as ", width ) ;
      this->width = width ;
    }
    
    void NyxStartDrawData::setHeight( unsigned height )
    {
      Log::output( "Module ", this->name.c_str(), " set height as ", height ) ;
      this->height = height ;
    }
    
    NyxStartDraw::NyxStartDraw()
    {
      this->module_data = new NyxStartDrawData() ;
    }

    NyxStartDraw::~NyxStartDraw()
    {
      delete this->module_data ;
    }

    void NyxStartDraw::initialize()
    {

    }

    void NyxStartDraw::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setWaitName            , iris::OPTIONAL, this->name(), "::wait_on"       ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setSignalName          , iris::OPTIONAL, this->name(), "::finish_signal" ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setWindowId            , iris::OPTIONAL, this->name(), "::window_id"     ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setWidth               , iris::OPTIONAL, this->name(), "::width"         ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setHeight              , iris::OPTIONAL, this->name(), "::height"        ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setOutputName          , iris::OPTIONAL, this->name(), "::outputs"       ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setInputSubpasses      , iris::OPTIONAL, this->name(), "::subpasses"     ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setOutputProjectionName, iris::OPTIONAL, this->name(), "::projection"    ) ;
      data().bus.enroll( this->module_data, &NyxStartDrawData::setDevice              , iris::OPTIONAL, this->name(), "::device"        ) ;
    }

    void NyxStartDraw::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
    }

    void NyxStartDraw::execute()
    {
      if( !data().render_pass.initialized() )
      {
        data().render_pass.setDimensions( data().width, data().height ) ;
      
        for( auto& subpass : data().subpasses )
        {
          data().render_pass.addSubpass( subpass ) ;
        }
  
        if( data().window_id == UINT_MAX )
        {
          data().render_pass .initialize( data().device                                ) ;
          data().render_chain.initialize( data().render_pass, nyx::ChainType::Graphics ) ;
        }
        else
        {
          data().render_pass .initialize( data().device     , data().window_id ) ;
          data().render_chain.initialize( data().render_pass, data().window_id ) ;
        }
        
        data().bus.emit() ;
      }
      else
      {
        data().wait_signal.wait() ;
      }
      
      data().wait_signal.emit() ;
    }

    NyxStartDrawData& NyxStartDraw::data()
    {
      return *this->module_data ;
    }

    const NyxStartDrawData& NyxStartDraw::data() const
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
  return "NyxStartDraw" ;
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
  return new nyx::vkg::NyxStartDraw() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxStartDraw* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxStartDraw*>( module ) ;
  delete mod ;
}
