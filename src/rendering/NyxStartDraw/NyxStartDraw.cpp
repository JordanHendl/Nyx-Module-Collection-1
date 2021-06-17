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
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <Iris/profiling/Timer.h>
#include <Iris/config/Parser.h>
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <glm/glm.hpp>
#include <limits>
#include <string>
#include <vector>
#include <climits>
#include <map>
#include <glm/ext/matrix_clip_space.hpp>
#include <chrono>
#include <thread>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    using          Log       = iris::log::Log             ;
    using          Framework = nyx::vkg::Vulkan           ;
    using          Token     = iris::config::json::Token  ;
    constexpr auto Format    = nyx::ImageFormat::RGBA8    ;
    
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
      
      unsigned                                         window_id    ;
      nyx::RenderPass<Framework>                       render_pass  ;
      nyx::Chain<Framework>                            render_chain ;
      unsigned                                         device       ;
      iris::Bus                                        ref_bus      ;
      iris::Bus                                        draw_bus     ;
      iris::Bus                                        wait_signal  ;
      std::string                                      name         ;
      unsigned                                         width        ;
      unsigned                                         height       ;
      unsigned                                         channels     ;
      Subpasses                                        subpasses    ;
      glm::mat4                                        proj         ;
      std::map<unsigned, const nyx::Chain<Framework>*> chain_map    ;
      bool                                             dirty        ;
      float                                            fov          ;
      bool                                             first        ;
      
      /** Default constructor.
       */
      NyxStartDrawData() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::Chain<Framework>& chain() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::RenderPass<Framework>& pass() ;
      
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
      
      /**
       * 
       */
      void initialize() ;

      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutputRefName( const char* name ) ;

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
      void setWaitName( unsigned idx, const char* name ) ;
      
      /** Set the name of the output name to signal.
       * @param name the name to signal.
       */
      void setSignalName( const char* name ) ;

      void setDrawInputNames( unsigned idx, const char* name ) ;
      
      void setDrawOutputName( const char* name ) ;
      
      void setInputChildRefName( unsigned idx, const char* name ) ;
      
      void setFOV( float fov ) ;
      
      void stepOne() ;
      
      void stepTwo() ;
      
      template<unsigned ID>
      void setInputDrawChain( const nyx::Chain<Framework>& chain ) ;
      
      template<unsigned ID>
      void setInputWaitSignal() ;
      
      template<unsigned ID>
      const nyx::Image<Framework>& framebuffer() ;

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
      this->first     = true     ;
      this->width     = 1280     ;
      this->height    = 1024     ;
      this->fov       = 80.f     ;
      this->name      = ""       ;
      this->device    = 0        ;
      this->window_id = UINT_MAX ;
    }
    
    const nyx::Chain<Framework>& NyxStartDrawData::chain()
    {
      return this->render_chain ;
    }
    
    const nyx::RenderPass<Framework>& NyxStartDrawData::pass()
    {
      return this->render_pass ;
    }

    const glm::mat4& NyxStartDrawData::projection()
    {
      return this->proj ;
    }
    
    template<unsigned ID>
    void NyxStartDrawData::setInputDrawChain( const nyx::Chain<Framework>& chain )
    {
      Log::output( "Module ", this->name.c_str(), " set input draw chain reference at index ", ID, " as \"", reinterpret_cast<const void*>( &chain ), "\"" ) ;
      this->chain_map[ ID ] = &chain ;
      this->dirty = true ;
    }

    void NyxStartDrawData::setOutputProjectionName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output projection as \"", name, "\"" ) ;
      this->ref_bus.publish( this, &NyxStartDrawData::projection, name ) ;
    }
    
    void NyxStartDrawData::setOutputRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output reference as \"", name, "\"" ) ;
      
      this->ref_bus.publish( this, &NyxStartDrawData::chain, name ) ;
      this->ref_bus.publish( this, &NyxStartDrawData::pass , name ) ;
    }
    
    template<unsigned ID>
    void NyxStartDrawData::setInputWaitSignal()
    {
//      Log::output( "Module ", this->name.c_str(), " wait signal recieved" ) ;
    }

    void NyxStartDrawData::setDrawInputNames( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set required input signal ", idx, " as \"", name, "\"" ) ;
      switch( idx )
      {
        case 0  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputWaitSignal<0>, iris::REQUIRED, name ) ; break ;
        case 1  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputWaitSignal<1>, iris::REQUIRED, name ) ; break ;
        case 2  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputWaitSignal<2>, iris::REQUIRED, name ) ; break ;
        case 3  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputWaitSignal<3>, iris::REQUIRED, name ) ; break ;
        case 4  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputWaitSignal<4>, iris::REQUIRED, name ) ; break ;
        default : break ;
      }
    }
    
    void NyxStartDrawData::setDrawOutputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set stage 2 output signal as \"", name, "\"" ) ;
      this->draw_bus.publish( this, &NyxStartDrawData::signal, name ) ;
    }
    
    void NyxStartDrawData::setFOV( float fov ) 
    {
      this->fov = fov ;
      this->proj = glm::perspective( glm::radians( this->fov ), static_cast<float>( this->width ) / static_cast<float>( this->height ), 0.1f, 5000.0f ) ;
    }

    void NyxStartDrawData::setInputChildRefName( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input draw reference ", idx, " as \"", name, "\"" ) ;
      
      switch( idx )
      {
        case 0  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputDrawChain<0>, iris::OPTIONAL, name ) ; break ;
        case 1  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputDrawChain<1>, iris::OPTIONAL, name ) ; break ;
        case 2  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputDrawChain<2>, iris::OPTIONAL, name ) ; break ;
        case 3  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputDrawChain<3>, iris::OPTIONAL, name ) ; break ;
        case 4  : this->draw_bus.enroll( this, &NyxStartDrawData::setInputDrawChain<4>, iris::OPTIONAL, name ) ; break ;
        default : break ;
      }
    }
    
    template<unsigned ID>
    const nyx::Image<Framework>& NyxStartDrawData::framebuffer()
    {
      return reinterpret_cast<const nyx::Image<Framework>&>( this->render_pass.framebuffer( ID ) ) ;
    }

    void NyxStartDrawData::setInputSubpasses( const Token& token )
    {
      nyx::Subpass    subpass           ;
      nyx::Attachment attachment        ;
      std::string     format_str        ;
      std::string     layout_str        ;
      float           clear_colors[ 4 ] ;
      bool            reinit            ;
      
      reinit = false ;
      
      if( this->render_pass.initialized() ) reinit = true ;
      if( this->render_chain.initialized() ) this->render_chain.synchronize() ;
      
      Log::output( "Module ", this->name.c_str(), " resetting." ) ;
      this->render_pass .reset() ;
//      this->render_chain.reset() ;
      
      this->subpasses.clear() ;
      Log::output( "Module ", this->name.c_str(), " parsing subpasses." ) ;

      // For each subpass.
      for( unsigned subpass_index = 0; subpass_index < token.size(); subpass_index++ )
      {
        auto subpass_token = token.token( subpass_index ) ;
        
        const auto depth_enable = subpass_token[ "depth_enable" ] ;
        const auto output       = subpass_token[ "output"       ] ;
        const auto depends      = subpass_token[ "depends"      ] ;
        const auto attachments  = subpass_token[ "attachments"  ] ;
        
        subpass = nyx::Subpass() ;
       
        if( output )
        {
          std::string out = output.string() ;
          Log::output( "Module ", this->name.c_str(), " set subpass ", subpass_index, " output as ", out.c_str() ) ;
          switch( subpass_index )
          {
            case 0: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<0>, out.c_str() ) ; break ;
            case 1: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<1>, out.c_str() ) ; break ;
            case 2: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<2>, out.c_str() ) ; break ;
            case 3: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<3>, out.c_str() ) ; break ;
            case 4: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<4>, out.c_str() ) ; break ;
            case 5: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<5>, out.c_str() ) ; break ;
            case 6: this->draw_bus.publish( this, &NyxStartDrawData::framebuffer<6>, out.c_str() ) ; break ;
          }
        }

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
                  clear_colors[ clear_index ] = clear_color.decimal( clear_index ) ;
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
        this->initialize() ;
      }
    }
    
    void NyxStartDrawData::stepOne()
    {
      this->render_chain.begin() ;
      this->wait_signal .emit() ;
      this->draw_bus    .wait() ;
    }
    
    void NyxStartDrawData::stepTwo()
    {
      const bool has_children = !this->chain_map.empty() ;
      // Begin our chain's operation and then send the signal telling the children to draw.
      if( has_children )
      {
        // Recombine command buffers.
        for( auto chain : this->chain_map )
        {
          this->render_chain.combine( *chain.second ) ;
        }
        
        this->render_chain.end() ;
        
        // Submit and present.
//        this->render_chain.submit() ;
        if( this->render_pass.present( this->render_chain ) )
        {
          Log::output( "Module", this->name.c_str(), " has had problem presenting to screen. Telling children to recreate.." ) ;
          this->ref_bus.emit() ;
          this->chain_map.clear() ;
        }
      }
      else
      {
        this->render_chain.end() ;
      }
      
      // Signal that this module has finished drawing.
      this->draw_bus.emit() ;
    }

    void NyxStartDrawData::initialize()
    {
      Log::output( "Module ", this->name.c_str(), " initializing." ) ;
      this->render_pass.setDimensions( this->width, this->height ) ;
      for( auto& subpass : this->subpasses )
      {
        this->render_pass.addSubpass( subpass ) ;
      }

      if( this->window_id == UINT_MAX )
      {
        this->render_pass .initialize( this->device                                      ) ;
        this->render_chain.initialize( this->render_pass, nyx::ChainType::Graphics, true ) ;
      }
      else
      {
        this->render_pass .initialize( this->device     , this->window_id       ) ;
        this->render_chain.initialize( this->render_pass, this->window_id, true ) ;
      }
      
      this->first = true ;
      this->ref_bus.emit() ;
    }
    
    void NyxStartDrawData::signal()
    {
    }
    
    void NyxStartDrawData::setWindowId( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set window id as ", id ) ;
      this->window_id = id ;
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
      data().proj = glm::perspective( glm::radians( data().fov ), static_cast<float>( data().width ) / static_cast<float>( data().height ), 0.1f, 5000.0f ) ;
      data().initialize() ;
    }

    void NyxStartDraw::subscribe( unsigned id )
    {
      data().ref_bus.setChannel( id ) ;
      data().name = this->name() ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setFOV                 , iris::OPTIONAL, this->name(), "::fov"           ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setWindowId            , iris::OPTIONAL, this->name(), "::window_id"     ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setWidth               , iris::OPTIONAL, this->name(), "::width"         ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setHeight              , iris::OPTIONAL, this->name(), "::height"        ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setInputSubpasses      , iris::OPTIONAL, this->name(), "::subpasses"     ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setDevice              , iris::OPTIONAL, this->name(), "::device"        ) ;
      
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setInputChildRefName   , iris::OPTIONAL, this->name(), "::children"      ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setDrawInputNames      , iris::OPTIONAL, this->name(), "::wait"          ) ;
      
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setSignalName          , iris::OPTIONAL, this->name(), "::child_signal"  ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setDrawOutputName      , iris::OPTIONAL, this->name(), "::finish"        ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setOutputRefName       , iris::OPTIONAL, this->name(), "::reference"     ) ;
      data().ref_bus.enroll( this->module_data, &NyxStartDrawData::setOutputProjectionName, iris::OPTIONAL, this->name(), "::projection"    ) ;
    }

    void NyxStartDraw::shutdown()
    {
      Framework::deviceSynchronize( data().device ) ;
    }

    void NyxStartDraw::execute()
    {
      data().stepOne() ;
      data().stepTwo() ;
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
