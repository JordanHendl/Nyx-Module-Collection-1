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
 * File:   NyxDrawSprite.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDrawSprite.h"
#include "draw_sprite.h"
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
    struct Sprite
    {
      unsigned sprite_index  = 0    ;
      unsigned tex_index     = 0    ;
      unsigned sprite_width  = 64   ;
      unsigned sprite_height = 64   ;
      unsigned image_width   = 1240 ;
      unsigned image_height  = 1080 ;
    };

      glm::vec4 sprite_vertices[] = 
      { 
        // pos    
        glm::vec4( 0.0f, 1.0f, 0.f, 0.0f ),
        glm::vec4( 1.0f, 0.0f, 1.f, 0.0f ),
        glm::vec4( 0.0f, 0.0f, 2.f, 0.0f ),
        
        glm::vec4( 0.0f, 1.0f, 0.f, 0.0f ),
        glm::vec4( 1.0f, 1.0f, 3.f, 0.0f ), 
        glm::vec4( 1.0f, 0.0f, 1.f, 0.0f )
      };
    
    // <editor-fold defaultstate="collapsed" desc="Aliases">
    using Log       = iris::log::Log               ;
    using Impl      = nyx::vkg::Vulkan             ;
    using Matrix    = glm::mat4                    ;
    using IdVec     = std::vector<unsigned>        ;
    using IdMap     = std::map<unsigned, unsigned> ;
    using SpriteMap = std::map<unsigned, Sprite>   ;
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawSpriteData Class & Function Declerations">
    struct NyxDrawSpriteData
    {
      struct Proj
      {
        Matrix proj ;
      };
      
      struct Iterators
      {
        nyx::Iterator<Impl, glm::mat4> positions ;
      };
      
      IdMap                        drawables        ;
      SpriteMap                    sprite_map       ;
      nyx::Array<Impl, Sprite   >  d_sprites        ;
      nyx::Array<Impl, glm::vec4>  d_vertices       ;
      nyx::Array<Impl, glm::mat4>  d_transforms     ;
      nyx::Array<Impl, glm::mat4>  d_viewproj       ;
      std::vector<glm::mat4>       h_transforms     ;
      std::vector<Sprite>          h_sprites        ;
      Iterators                    h_iterator       ;
      nyx::Viewport                viewport         ;
      Proj                         mvp              ;
      const Matrix*                camera           ;
      Matrix                       projection       ;
      unsigned                     subpass          ;
      nyx::Pipeline<Impl>          pipeline         ;
      const nyx::Chain<Impl>*      parent           ;
      const nyx::RenderPass<Impl>* parent_pass      ;
      bool                         rebuild_chain    ;
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
      NyxDrawSpriteData() ;
      
      /** Helper method to synchronize the VP matrix from the host to the device.
       */
      void syncVPMatrix() ;
      
      /** Method to help update textures when the database is updated.
       */
      void updateTextures() ;

      /** Helper method to build a buffer of the draw operations of all models.
       */
      void redrawSprites() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::Chain<Impl>& chain() ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param texture The id associated with the texture in the database.
       */
      void setSprite( unsigned index, unsigned sprite ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param texture The id associated with the texture in the database.
       */
      void setSpriteTexture( unsigned index, unsigned texture ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void setSpriteTransform( unsigned index, const glm::mat4& position ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void removeSprite( unsigned index ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setParentRef( const nyx::Chain<Impl>& parent ) ;
      
      /** Method to parse the token of sprites for this object to be able to draw.
       * @param token The token to parse. 
       */
      void parseSprites( const iris::config::json::Token& token ) ;

      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setParentPassRef( const nyx::RenderPass<Impl>& parent ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setSubpass( unsigned index ) ;
      
      /** Method to set the parent chain of this object.
       * @param parent The chain to inherit from and to use for appending draw calls to.
       */
      void setSubpassName( const char* name ) ;
      
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
      void setSpriteInputName( const char* name ) ;
      
      /** Method to set the name to associate with the model input.
       * @param name The name to associate with the model input.
       */
      void setSpriteTextureInputName( const char* name ) ;
      
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
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawSprite Class & Function Definitions">

    void NyxDrawSpriteData::updateTextures()
    {
      this->lock.lock() ;
      
      for( auto& sprite : this->h_sprites )
      {
        sprite.image_width  = mars::TextureArray<Impl>::images()[ sprite.tex_index ]->width () ;
        sprite.image_height = mars::TextureArray<Impl>::images()[ sprite.tex_index ]->height() ;
      }
      
      Impl::deviceSynchronize( this->device ) ;
      auto images = mars::TextureArray<Impl>::images() ;
      images = images ;
      this->pipeline.bind( "textures", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
      this->copy_chain.copy( this->h_sprites.data(), this->d_sprites ) ;
      this->copy_chain.submit() ;
      this->dirty_flag = true ;
      Impl::deviceSynchronize( this->device ) ;
      this->lock.unlock() ;
    }

    void NyxDrawSpriteData::syncVPMatrix()
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
    
    void NyxDrawSpriteData::redrawSprites()
    {
        if( this->dirty_flag && this->draw_chain.initialized() && this->drawables.size() != 0 )
      {
        this->h_iterator.positions = this->d_transforms.iterator() ;
        
        this->draw_chain.push( this->pipeline, this->h_iterator ) ;
        this->draw_chain.drawInstanced( this->drawables.size(), this->pipeline, this->d_vertices ) ;
        
        this->draw_chain.end() ;
        this->dirty_flag = false ;
        this->bus.emit() ;
      }
      else
      {
        this->draw_chain.advance() ;
      }
    }
    
    void NyxDrawSpriteData::wait()
    {
    }
    
    void NyxDrawSpriteData::signal()
    {
    }
    
    void NyxDrawSpriteData::setSprite( unsigned index, unsigned sprite_index )
    {
      if( index < this->d_sprites.size() ) 
      {
        this->lock.lock() ;
        this->h_sprites[ index ].sprite_index = sprite_index ;
        this->copy_chain.copy( this->h_sprites.data(), this->d_sprites ) ;
        this->copy_chain.submit() ;
        this->lock.unlock() ;
      }

      this->drawables[ index ] = sprite_index ; 
      this->dirty_flag = true ;
    }
    
    void NyxDrawSpriteData::setSpriteTexture( unsigned index, unsigned texture )
    {
      if( index < this->d_sprites.size() ) 
      {
        lock.lock() ;
        this->h_sprites[ index ].tex_index = texture ;
        this->copy_chain.copy( this->h_sprites.data(), this->d_sprites ) ;
        this->copy_chain.submit() ;
        lock.unlock() ;
      }
    }
    
    void NyxDrawSpriteData::setSpriteTransform( unsigned index, const glm::mat4& position )
    {
      if( this->d_transforms.size() < index )
      {
        this->d_transforms.reset() ;
        this->d_transforms.initialize( this->device, index + 1024 ) ;
      }
      
      this->h_transforms[ index ] = position ;
      this->lock.lock() ;
      this->copy_chain.synchronize() ;
      this->copy_chain.copy( this->h_transforms.data(), this->d_transforms ) ;
      this->copy_chain.submit() ;
      this->copy_chain.synchronize() ;
      this->lock.unlock() ;
    }
    
    void NyxDrawSpriteData::removeSprite( unsigned index )
    {
      auto iter = this->drawables.find( index ) ;
      
      if( iter != this->drawables.end() )
      {
        this->drawables.erase( iter ) ;
      }
      
      this->dirty_flag = true ;
    }
    
    void NyxDrawSpriteData::parseSprites( const iris::config::json::Token& token )
    {
      unsigned id     ;
      unsigned width  ;
      unsigned height ;
     
      id     = UINT32_MAX ;
      width  = UINT32_MAX ;
      height = UINT32_MAX ;
      
      for( unsigned index = 0; index < token.size(); index++ )
      {
        auto sprite_token = token.token( index )            ;
        auto id_token     = sprite_token[ "ID" ]            ;
        auto width_token  = sprite_token[ "sprite_width"  ] ;
        auto height_token = sprite_token[ "sprite_height" ] ;
        
        if( id_token     ) id     = id_token    .number() ;
        if( width_token  ) width  = width_token .number() ;
        if( height_token ) height = height_token.number() ;
        
        if( id != UINT32_MAX && width != UINT32_MAX && height != UINT32_MAX ) 
        {
          Log::output( "Module ", this->name.c_str(), " added sprite with the following parameters { ID: ", id, " Sprite Width: ", width, " Sprite Height: ", height, " }" ) ;
          this->sprite_map[ id ].sprite_width  = width  ;
          this->sprite_map[ id ].sprite_height = height ;
        }
      }
    }

    void NyxDrawSpriteData::setParentRef( const nyx::Chain<Impl>& parent )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent chain reference as ", reinterpret_cast<const void*>( &parent ) ) ;
      this->rebuild_chain = true ;
      this->parent = &parent ;
    }
    
    void NyxDrawSpriteData::setParentPassRef( const nyx::RenderPass<Impl>& parent )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference as ", reinterpret_cast<const void*>(  &parent ) ) ;
      this->parent_pass = &parent ;
    }
    
    void NyxDrawSpriteData::setSubpass( unsigned index )
    {
      Log::output( "Module ", this->name.c_str(), " set input subpass as ", index ) ;
      this->subpass = index ;
    }
    
    void NyxDrawSpriteData::setSubpassName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input subpass signal as ", name ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setSubpass, iris::OPTIONAL, name ) ;
    }

    void NyxDrawSpriteData::setViewRef( const Matrix& view )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference as ", reinterpret_cast<const void*>(  &view ) ) ;
      this->camera = &view ;
      this->vp_dirty_flag = true ;
    }
    
    const nyx::Chain<Impl>& NyxDrawSpriteData::chain()
    {
      return this->draw_chain ;
    }
    
    void NyxDrawSpriteData::setDevice( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set input device as ", id ) ;
      this->device = id ;
    }

    void NyxDrawSpriteData::setParentRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setParentRef    , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setParentPassRef, iris::OPTIONAL, name ) ;
      
    }
    
    void NyxDrawSpriteData::setSpriteInputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input add sprite signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setSprite         , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setSpriteTransform, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawSpriteData::setSpriteTextureInputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input set sprite texture signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setSpriteTexture, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawSpriteData::setTextureClearName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input remove model signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::removeSprite, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawSpriteData::setViewRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawSpriteData::setViewRef, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawSpriteData::setOutRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output reference as \"", name, "\"" ) ;
      
      this->bus.publish( this, &NyxDrawSpriteData::chain, name ) ;
    }
    
    void NyxDrawSpriteData::setInputNames( unsigned idx, const char* name )
    {
      idx = idx ;
      this->wait_and_publish.enroll( this, &NyxDrawSpriteData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxDrawSpriteData::setOutputName( const char* name )
    {
      this->wait_and_publish.publish( this, &NyxDrawSpriteData::signal, name ) ;
    }

    NyxDrawSpriteData::NyxDrawSpriteData()
    {
      this->projection = glm::mat4( 1.0f ) ;
      
      this->viewport.setWidth ( 1280 ) ;
      this->viewport.setHeight( 1024 ) ;
      
      this->name          = ""      ;
      this->device        = 0       ;
      this->parent_pass   = nullptr ;
      this->parent        = nullptr ;
      this->camera        = nullptr ;
      this->rebuild_chain = true    ;
    }
    
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawSprite Function Definitions">
    NyxDrawSprite::NyxDrawSprite()
    {
      this->module_data = new NyxDrawSpriteData() ;
    }

    NyxDrawSprite::~NyxDrawSprite()
    {
      delete this->module_data ;
    }

    void NyxDrawSprite::initialize()
    {
      data().copy_chain   .initialize( data().device, nyx::ChainType::Compute                     ) ;
      data().d_vertices   .initialize( data().device, 6   , false, nyx::ArrayFlags::Vertex        ) ;
      data().d_viewproj   .initialize( data().device, 1   , false, nyx::ArrayFlags::UniformBuffer ) ;
      data().d_sprites    .initialize( data().device, 1024, false, nyx::ArrayFlags::UniformBuffer ) ;
      data().d_transforms .initialize( data().device, 1024 ) ;
      data().h_transforms .resize( 1024 )                    ;
      data().h_sprites    .resize( 1024 )                    ;
      
      std::fill( data().h_transforms.begin(), data().h_transforms.end(), glm::mat4( 1.0f ) ) ;
      
      data().lock.lock() ;
      data().copy_chain.copy( data().h_transforms.data(), data().d_transforms ) ;
      data().copy_chain.submit() ;
      data().copy_chain.synchronize() ;
      
      data().copy_chain.copy( vkg::sprite_vertices, data().d_vertices ) ;
      data().copy_chain.submit() ;
      data().copy_chain.synchronize() ;
      data().lock.unlock() ;
      
      data().draw_chain.setMode( nyx::ChainMode::All ) ;
      
      if( data().parent_pass != nullptr )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.setTestDepth( true ) ;
        data().pipeline.initialize( *data().parent_pass, nyx::bytes::draw_sprite, sizeof( nyx::bytes::draw_sprite ) ) ;
        data().lock.lock() ;
        Impl::deviceSynchronize( data().device ) ;
        data().pipeline.bind( "projection", data().d_viewproj ) ;
        data().pipeline.bind( "sprite"    , data().d_sprites  ) ;
        data().pipeline.bind( "textures", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
        Impl::deviceSynchronize( data().device ) ;
        data().lock.unlock() ;
      }
      
      mars::TextureArray<Impl>::addCallback( this->module_data, &NyxDrawSpriteData::updateTextures, this->name() ) ;
    }

    void NyxDrawSprite::subscribe( unsigned id )
    {
      
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setInputNames       , iris::OPTIONAL, this->name(), "::input"                ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setOutputName       , iris::OPTIONAL, this->name(), "::output"               ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setParentRefName    , iris::OPTIONAL, this->name(), "::parent"               ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setSpriteInputName  , iris::OPTIONAL, this->name(), "::sprite_input"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::parseSprites        , iris::OPTIONAL, this->name(), "::sprites"              ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setSubpassName      , iris::OPTIONAL, this->name(), "::subpass"              ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setSpriteTexture    , iris::OPTIONAL, this->name(), "::sprite_texture_input" ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setTextureClearName , iris::OPTIONAL, this->name(), "::sprite_remove"        ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setOutRefName       , iris::OPTIONAL, this->name(), "::reference"            ) ;
      data().bus.enroll( this->module_data, &NyxDrawSpriteData::setDevice           , iris::OPTIONAL, this->name(), "::device"               ) ;
    }

    void NyxDrawSprite::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
    }

    void NyxDrawSprite::execute()
    {
      data().wait_and_publish.wait() ;
      
      data().syncVPMatrix() ;
      
      if( data().parent != nullptr && data().rebuild_chain )
      {
        Log::output( "Module ", this->name(), " rebuilding chain" ) ;
        data().rebuild_chain = false ;
        data().draw_chain.initialize( *data().parent, data().subpass ) ;
        data().dirty_flag = true ;
      }
      if( !data().pipeline.initialized() && data().parent_pass )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.setTestDepth( true ) ;
        data().pipeline.initialize( *data().parent_pass, nyx::bytes::draw_sprite, sizeof( nyx::bytes::draw_sprite ) ) ;
        data().lock.lock() ;
        Impl::deviceSynchronize( data().device ) ;
        data().pipeline.bind( "projection", data().d_viewproj ) ;
        data().pipeline.bind( "sprite"    , data().d_sprites  ) ;
        data().pipeline.bind( "textures", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
        Impl::deviceSynchronize( data().device ) ;
        data().lock.unlock() ;
      }

      data().redrawSprites() ;
      
      data().wait_and_publish.emit() ;
    }

    NyxDrawSpriteData& NyxDrawSprite::data()
    {
      return *this->module_data ;
    }

    const NyxDrawSpriteData& NyxDrawSprite::data() const
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
  return "NyxDrawSprite" ;
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
  return new nyx::vkg::NyxDrawSprite() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxDrawSprite* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxDrawSprite*>( module ) ;
  delete mod ;
  // </editor-fold>
}