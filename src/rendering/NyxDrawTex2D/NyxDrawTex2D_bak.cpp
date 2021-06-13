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
 * File:   NyxDrawTexture.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawTex2D_bak.h"
#include "draw_tex2d.h"
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <Iris/profiling/Timer.h>
#include <Iris/config/Parser.h>
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <Mars/Texture.h>
#include <Mars/TextureArray.h>
#include <Mars/Manager.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <mutex>
#include <map>
#include <unordered_map>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    glm::vec4 vertices[] = 
    { 
      // pos      // tex
      glm::vec4( 0.0f,  0.0f, 0.0f, 1.0f ),
      glm::vec4( 1.0f, -1.0f, 1.0f, 0.0f ),
      glm::vec4( 0.0f, -1.0f, 0.0f, 0.0f ), 
  
      glm::vec4( 0.0f,  0.0f, 0.0f, 1.0f ),
      glm::vec4( 1.0f,  0.0f, 1.0f, 1.0f ),
      glm::vec4( 1.0f, -1.0f, 1.0f, 0.0f )
    };
    
    // <editor-fold defaultstate="collapsed" desc="Aliases">
    using Log            = iris::log::Log               ;
    using Impl           = nyx::vkg::Vulkan             ;
    using Matrix         = glm::mat4                    ;
    using Textures       = std::map<unsigned, unsigned> ;
    using IdVec          = std::vector<unsigned>        ;
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawTex2DData Class & Function Declerations">
    struct NyxDrawTex2DData
    {
      struct Proj
      {
        Matrix proj ;
      };
      struct Sprite
      {
        float tx            ;
        float ty            ;
        float image_width   ;
        float image_height  ;
        float sprite_width  ;
        float sprite_height ;
        int   image_index   ;
        int   padding       ;
      };
      
      struct Iterators
      {
        nyx::Iterator<Impl, unsigned > textures  ;
        nyx::Iterator<Impl, glm::mat4> positions ;
      };

      Iterators                    iterator         ;
      IdVec                        textures         ;
      std::vector<glm::mat4>       tmp              ;
      nyx::Viewport                viewport         ;
      Proj                         mvp              ;
      nyx::Array<Impl, unsigned>   d_texture        ;
      nyx::Array<Impl, glm::vec4>  d_vertices       ;
      nyx::Array<Impl, glm::mat4>  d_model_trans    ;
      nyx::Array<Impl, glm::mat4>  d_viewproj       ;
      Textures                     drawables        ;
      const Matrix*                camera           ;
      Matrix                       projection       ;
      unsigned                     subpass          ;
      nyx::Renderer<Impl>          pipeline         ;
      const nyx::Chain<Impl>*      parent           ;
      const nyx::RenderPass<Impl>* parent_pass      ;
      nyx::Chain<Impl>             draw_chain       ;
      nyx::Chain<Impl>             copy_chain       ;
      unsigned                     device           ;
      iris::Bus                    bus              ;
      iris::Bus                    wait_and_publish ;
      std::string                  name             ;
      bool                         dirty_flag       ;
      bool                         vp_dirty_flag    ;
      std::mutex                   lock             ;
      
      /** Default constructor.
       */
      NyxDrawTex2DData() ;
      
      /** Helper method to synchronize the VP matrix from the host to the device.
       */
      void syncVPMatrix() ;
      
      /** Method to help update textures when the database is updated.
       */
      void updateTextures() ;

      /** Helper method to build a buffer of the draw operations of all models.
       */
      void redrawTextures() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::Chain<Impl>& chain() ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param texture The id associated with the texture in the database.
       */
      void setTexture( unsigned index, unsigned texture ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void setTextureTransform( unsigned index, const glm::mat4& position ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void removeTexture( unsigned index ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setParentRef( const nyx::Chain<Impl>& parent ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setParentPassRef( const nyx::RenderPass<Impl>& parent ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setSubpass( unsigned index ) ;
      
      /** Method to set the reference to the view matrix to use when rendering models.
       * @param view The reference to the matrix used to represent a camera.
       */
      void setViewRef( const Matrix& view ) ;
      
      /** Method to set the reference to the projection matrix to use when rendering models.
       * @param proj The reference to the matrix used to represent the view frustum.
       */
      void setProjRef( const Matrix& proj ) ;
      
      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutRefName( const char* name ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setParentRefName( const char* name ) ;
      
      /** The function to use to signal when this module has finished.
       */
      void wait() ;
      
      /** The function to use to signal when this module is to begin operation.
       */
      void signal() ;
      
      /** Method to set then name of the input signal to signal when operation is finished.
       * @param name The name of the signal to signal.
       */
      void setOutputName( const char* name ) ;

      /** Method to set the name of the database's request signal.
       * @param name The name to send the signal for a requested model to.
       */
      void setDatabaseRequest( const char* name ) ;

      /** Method to set the name of the input width parameter.
       * @param name The name to associate with the input.
       */
      void setInputNames( unsigned idx, const char* name ) ;
      
      /** Method to set the name to associate with the model input.
       * @param name The name to associate with the model input.
       */
      void setTextureInputName( const char* name ) ;
      
      /** Method to set the name to associate with the model clear.
       * @param name The name to associate with the model clear.
       */
      void setTextureClearName( const char* name ) ;
      
      /** Method to set the name to associate with the camera input.
       * @param name The name to associate with the camera input.
       */
      void setViewRefName( const char* name ) ;
      
      /** Method to set the name to associate with the projection input .
       * @param name the name to associate with the projection input.
       */
      void setProjRefName( const char* name ) ;
      
      /** Method to set the device to use for this module's operations.
       * @param device The device to set.
       */
      void setDevice( unsigned id ) ;
    };
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawTex2D Class & Function Definitions">

    void NyxDrawTex2DData::updateTextures()
    {
      this->lock.lock() ;
      Impl::deviceSynchronize( this->device ) ;
      auto images = mars::TextureArray<Impl>::images() ;
      images = images ;
      this->pipeline.bind( "textures", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
      this->dirty_flag = true ;
      Impl::deviceSynchronize( this->device ) ;
      this->lock.unlock() ;
    }

    void NyxDrawTex2DData::syncVPMatrix()
    {
      if( this->copy_chain.initialized() ) 
      {
        this->lock.lock() ;
        this->copy_chain.copy( &this->projection, this->d_viewproj )  ;
        this->copy_chain.submit() ;
        this->copy_chain.synchronize() ;
        this->vp_dirty_flag = false ;
        this->lock.unlock() ;
      }
    }
    
    void NyxDrawTex2DData::redrawTextures()
    {
        if( this->dirty_flag && this->draw_chain.initialized() && this->drawables.size() != 0 )
      {
        this->iterator.textures  = this->d_texture    .iterator() ;
        this->iterator.positions = this->d_model_trans.iterator() ;
        
        this->lock.lock() ;
        this->draw_chain.push( this->pipeline, this->iterator ) ;
        this->draw_chain.drawInstanced( this->drawables.size(), this->pipeline, this->d_vertices ) ;
        
        this->draw_chain.end() ;
        this->dirty_flag = false ;
        this->draw_chain.end() ;
        this->lock.unlock() ;
        this->bus.emit() ;
      }
      else
      {
        this->draw_chain.advance() ;
      }
    }
    
    void NyxDrawTex2DData::wait()
    {
    }
    
    void NyxDrawTex2DData::signal()
    {
    }
    
    void NyxDrawTex2DData::setTexture( unsigned index, unsigned texture )
    {
      this->lock.lock() ;
      if( index < this->textures.size() ) 
      {
        this->textures[ index ] = texture ;
        this->copy_chain.copy( this->textures.data(), this->d_texture ) ;
        this->copy_chain.submit() ;
      }

      this->drawables.insert( { index, texture } ) ;
      this->dirty_flag = true ;
      this->lock.unlock() ;
    }
    
    void NyxDrawTex2DData::setTextureTransform( unsigned index, const glm::mat4& position )
    {
      if( this->d_model_trans.size() < index )
      {
        this->d_model_trans.reset() ;
        this->d_model_trans.initialize( this->device, index + 1024 ) ;
      }
      
      this->tmp[ index ] = position ;
      this->lock.lock() ;
      this->copy_chain.copy( this->tmp.data(), this->d_model_trans ) ;
      this->copy_chain.submit() ;
      this->copy_chain.synchronize() ;
      this->lock.unlock() ;
    }
    
    void NyxDrawTex2DData::removeTexture( unsigned index )
    {
      this->lock.lock() ;
      auto iter = this->drawables.find( index ) ;
      
      if( iter != this->drawables.end() )
      {
        this->drawables.erase( iter ) ;
      }
      
      this->dirty_flag = true ;
      this->lock.unlock() ;
    }
    
    void NyxDrawTex2DData::setParentRef( const nyx::Chain<Impl>& parent )
    {
      this->lock.lock() ;
      Log::output( "Module ", this->name.c_str(), " set input parent chain reference as ", reinterpret_cast<const void*>( &parent ) ) ;
      this->parent = &parent ;
      this->lock.unlock() ;
    }
    
    void NyxDrawTex2DData::setParentPassRef( const nyx::RenderPass<Impl>& parent )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference as ", reinterpret_cast<const void*>(  &parent ) ) ;
      this->parent_pass = &parent ;
    }
    
    void NyxDrawTex2DData::setSubpass( unsigned index )
    {
      Log::output( "Module ", this->name.c_str(), " set input subpass as ", index ) ;
      this->subpass = index ;
    }

    void NyxDrawTex2DData::setViewRef( const Matrix& view )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference as ", reinterpret_cast<const void*>(  &view ) ) ;
      this->camera = &view ;
      this->vp_dirty_flag = true ;
    }
    
    const nyx::Chain<Impl>& NyxDrawTex2DData::chain()
    {
      return this->draw_chain ;
    }
    
    void NyxDrawTex2DData::setDevice( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set input device as ", id ) ;
      this->device = id ;
    }

    void NyxDrawTex2DData::setParentRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setParentRef    , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setParentPassRef, iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setSubpass      , iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawTex2DData::setTextureInputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input add texture signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setTexture         , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setTextureTransform, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawTex2DData::setTextureClearName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input remove model signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::removeTexture, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawTex2DData::setViewRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawTex2DData::setViewRef, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawTex2DData::setOutRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output reference as \"", name, "\"" ) ;
      
      this->bus.publish( this, &NyxDrawTex2DData::chain, name ) ;
    }
    
    void NyxDrawTex2DData::setInputNames( unsigned idx, const char* name )
    {
      idx = idx ;
      this->wait_and_publish.enroll( this, &NyxDrawTex2DData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxDrawTex2DData::setOutputName( const char* name )
    {
      this->wait_and_publish.publish( this, &NyxDrawTex2DData::signal, name ) ;
    }

    NyxDrawTex2DData::NyxDrawTex2DData()
    {
      this->projection = glm::mat4( 1.0f ) ;
      
      this->viewport.setWidth ( 1280 ) ;
      this->viewport.setHeight( 1024 ) ;
      
      this->name        = ""      ;
      this->device      = 0       ;
      this->parent_pass = nullptr ;
      this->parent      = nullptr ;
      this->camera      = nullptr ;
    }
    
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawTex2D Function Definitions">
    NyxDrawTex2D::NyxDrawTex2D()
    {
      this->module_data = new NyxDrawTex2DData() ;
    }

    NyxDrawTex2D::~NyxDrawTex2D()
    {
      delete this->module_data ;
    }

    void NyxDrawTex2D::initialize()
    {
      data().copy_chain   .initialize( data().device, nyx::ChainType::Compute ) ;
      data().d_vertices   .initialize( data().device, 6, false, nyx::ArrayFlags::Vertex ) ;
      data().d_viewproj   .initialize( data().device, 1, false, nyx::ArrayFlags::UniformBuffer ) ;
      data().d_model_trans.initialize( data().device, 1024 ) ;
      data().d_texture    .initialize( data().device, 1024 ) ;
      data().tmp          .resize( 1024 )                    ;
      data().textures     .resize( 1024 )                    ;
      
      std::fill( data().tmp.begin(), data().tmp.end(), glm::mat4( 1.0f ) ) ;
      
      data().lock.lock() ;
      data().copy_chain.copy( data().tmp.data(), data().d_model_trans ) ;
      data().copy_chain.submit() ;
      data().copy_chain.synchronize() ;
      
      data().copy_chain.copy( vkg::vertices, data().d_vertices ) ;
      data().copy_chain.submit() ;
      data().copy_chain.synchronize() ;
      data().lock.unlock() ;
      
      data().draw_chain.setMode( nyx::ChainMode::All ) ;
      
      if( data().parent_pass != nullptr )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.setTestDepth( true ) ;
        data().pipeline.initialize( data().device, *data().parent_pass, nyx::bytes::draw_tex2d, sizeof( nyx::bytes::draw_tex2d ) ) ;
        data().pipeline.bind( "projection", data().d_viewproj ) ;
      }
      mars::TextureArray<Impl>::addCallback( this->module_data, &NyxDrawTex2DData::updateTextures, this->name() ) ;
    }

    void NyxDrawTex2D::subscribe( unsigned id )
    {
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setInputNames       , iris::OPTIONAL, this->name(), "::input"          ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setOutputName       , iris::OPTIONAL, this->name(), "::output"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setParentRefName    , iris::OPTIONAL, this->name(), "::parent"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setTextureInputName , iris::OPTIONAL, this->name(), "::texture_input"  ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setTextureClearName , iris::OPTIONAL, this->name(), "::texture_remove" ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setOutRefName       , iris::OPTIONAL, this->name(), "::reference"      ) ;
      data().bus.enroll( this->module_data, &NyxDrawTex2DData::setDevice           , iris::OPTIONAL, this->name(), "::device"         ) ;
    }

    void NyxDrawTex2D::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
    }

    void NyxDrawTex2D::execute()
    {
      data().wait_and_publish.wait() ;
      
      data().syncVPMatrix() ;
      
      if( !data().draw_chain.initialized() && data().parent != nullptr )
      {
        data().draw_chain.initialize( *data().parent, data().subpass ) ;
      }
      if( !data().pipeline.initialized() && data().parent_pass )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.setTestDepth( true ) ;
        data().pipeline.initialize( data().device, *data().parent_pass, nyx::bytes::draw_tex2d, sizeof( nyx::bytes::draw_tex2d ) ) ;
        data().lock.lock() ;
        Impl::deviceSynchronize( data().device ) ;
        data().pipeline.bind( "projection", data().d_viewproj ) ;
        data().pipeline.bind( "textures", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
        Impl::deviceSynchronize( data().device ) ;
        data().lock.unlock() ;
      }
      data().redrawTextures() ;
      
      data().wait_and_publish.emit() ;
    }

    NyxDrawTex2DData& NyxDrawTex2D::data()
    {
      return *this->module_data ;
    }

    const NyxDrawTex2DData& NyxDrawTex2D::data() const
    {
      return *this->module_data ;
    }
  }
  // </editor-fold>
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDrawTex2D" ;
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
  return new nyx::vkg::NyxDrawTex2D() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxDrawTex2D* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxDrawTex2D*>( module ) ;
  delete mod ;
  // </editor-fold>
}