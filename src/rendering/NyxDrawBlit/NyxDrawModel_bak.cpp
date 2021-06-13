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
 * File:   NyxDrawModel.cpp
 * Author: Jordan Hendl
 * 
 * Created on April 14, 2021, 6:58 PM
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include "NyxDrawModel_bak.h"
#include "draw_model.h"
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <Iris/profiling/Timer.h>
#include <Iris/config/Parser.h>
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <Mars/Model.h>
#include <Mars/Manager.h>
#include <Mars/Texture.h>
#include <Mars/TextureArray.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
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
    // <editor-fold defaultstate="collapsed" desc="Aliases">
    using Log            = iris::log::Log                                            ;
    using Impl           = nyx::vkg::Vulkan                                          ;
    using ModelManager   = mars::Manager<unsigned, mars::Model<Impl>>                ;
    using TextureManager = mars::Manager<unsigned, nyx::Image <Impl>>                ;
    using Matrix         = glm::mat4                                                 ;
    using ModelPair      = std::tuple<mars::Reference<mars::Model<Impl>>, glm::mat4> ;
    using Textures       = std::map<unsigned, mars::Reference<nyx::Image<Impl>>>     ;
    using Models         = std::map<unsigned, ModelPair>                             ;
    using IdVec          = std::vector<unsigned>                                     ;
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawModelData Class & Function Declerations">
    struct NyxDrawModelData
    {
      struct ViewProj
      {
        Matrix view_proj ;
      };
      
      struct Iterators
      {
        nyx::Iterator<Impl, ViewProj > viewproj    ;
        unsigned                       index       ;
        unsigned                       diffuse_tex ;
      };

      std::vector<glm::mat4>                     tmp              ;
      nyx::Viewport                              viewport         ;
      ViewProj                                   mvp              ;
      nyx::Array<Impl, glm::mat4>                d_transforms     ;
      nyx::Array<Impl, ViewProj>                 d_viewproj       ;
      std::unordered_map<std::string, Iterators> iterators        ;
      Models                                     drawables        ;
      const Matrix*                              camera           ;
      const Matrix*                              projection       ;
      unsigned                                   subpass          ;
      nyx::Pipeline<Impl>                        pipeline         ;
      const nyx::Chain<Impl>*                    parent           ;
      const nyx::RenderPass<Impl>*               parent_pass      ;
      nyx::Chain<Impl>                           draw_chain       ;
      nyx::Chain<Impl>                           copy_chain       ;
      unsigned                                   device           ;
      iris::Bus                                  bus              ;
      iris::Bus                                  wait_and_publish ;
      std::string                                name             ;
      bool                                       dirty_flag       ;
      bool                                       vp_dirty_flag    ;
      std::mutex                                 lock             ;
      
      /** Default constructor.
       */
      NyxDrawModelData() ;
      
      /** Helper method to synchronize the VP matrix from the host to the device.
       */
      void syncVPMatrix() ;
      
      /** Method to help update textures when the database is updated.
       */
      void updateTextures() ;

      /** Helper method to build a buffer of the draw operations of all models.
       */
      void redrawModels() ;
      
      /** Method to retrieve the const chain from this object.
       * @return The const reference to this object's chain object.
       */
      const nyx::Chain<Impl>& chain() ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void setModel( unsigned index, mars::Reference<mars::Model<Impl>> model ) ;
      
      /** Method to set the initial models to load on initialization of this module.
       * @param index The index to use for the model.
       * @param model_id The id associated with the model in the database.
       */
      void setInitialModels( unsigned index, unsigned model_id ) ;

      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void setModelTransform( unsigned index, const glm::mat4& position ) ;
      
      /** Method to set the model at the specified index.
       * @param index The index of model to set.
       * @param model_id The id associated with the model in the database.
       */
      void removeModel( unsigned index ) ;
      
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
      
      /** Helper function to load textures of a model.
       */
      void loadTextures( const mars::Reference<mars::Model<Impl>>& ref ) ;
      
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
      void setModelInputName( const char* name ) ;
      
      /** Method to set the name to associate with the model clear.
       * @param name The name to associate with the model clear.
       */
      void setModelClearName( const char* name ) ;
      
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
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawModelData Class & Function Definitions">

    void NyxDrawModelData::updateTextures()
    {
      this->lock.lock() ;
      Impl::deviceSynchronize( this->device ) ;
      this->pipeline.bind( "mesh_texture", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
      this->dirty_flag = true ;
      Impl::deviceSynchronize( this->device ) ;
      this->lock.unlock() ;
    }

    void NyxDrawModelData::syncVPMatrix()
    {
      if( this->copy_chain.initialized() && this->camera != nullptr && this->projection != nullptr ) 
      {
        this->mvp.view_proj = *this->projection * *this->camera ;
        this->lock.lock() ;
        this->copy_chain.copy( &this->mvp, this->d_viewproj )  ;
        this->copy_chain.submit() ;
        this->vp_dirty_flag = false ;
        this->lock.unlock() ;
      }
    }
    
    void NyxDrawModelData::redrawModels()
    {
      unsigned index ;
      
      if( this->dirty_flag && this->draw_chain.initialized() && this->drawables.size() != 0 )
      {
        index = 0 ;
   
        this->lock.lock() ;
        this->copy_chain.copy( &this->mvp, this->d_viewproj ) ;
        this->copy_chain.submit() ;
        this->copy_chain.synchronize() ;
        this->lock.unlock() ;
        
        this->draw_chain.begin() ;
        for( auto& model : this->drawables )
        {
          
          auto& meshes = std::get<0>( model.second )->meshes() ;

          for( auto mesh : meshes )
          {
            auto& iter = this->iterators[ mesh->name ] ;
            
            auto mesh_iter = mesh->textures.find( "diffuse" ) ;
            if( mesh_iter != mesh->textures.end() ) iter.diffuse_tex = mesh->textures[ "diffuse" ] ;
            else                                    iter.diffuse_tex = 0                           ;
            
            iter.index     = index                          ;
//            iter.positions = this->d_transforms.iterator() ;
            iter.viewproj  = this->d_viewproj.iterator   () ;
            this->draw_chain.push( this->pipeline, iter ) ;
            this->draw_chain.drawIndexed( this->pipeline, mesh->indices, mesh->vertices ) ;
          }
          
          index++ ;
        }
        
        this->draw_chain.end() ;
        this->dirty_flag = false ;
        this->draw_chain.end() ;
        this->bus.emit() ;
      }
      else
      {
        this->draw_chain.advance() ;
      }
    }
    
    void NyxDrawModelData::wait()
    {
    }
    
    void NyxDrawModelData::signal()
    {
    }
    
    void NyxDrawModelData::setModel( unsigned index, mars::Reference<mars::Model<Impl>> model )
    {
      this->drawables.insert( { index, std::make_tuple( model, glm::mat4( 1.0f ) ) } ) ;
      this->dirty_flag = true ;
    }
    
    void NyxDrawModelData::setModelTransform( unsigned index, const glm::mat4& position )
    {
      if( this->d_transforms.size() < index )
      {
        this->d_transforms.reset() ;
        this->d_transforms.initialize( this->device, index + 1024 ) ;
      }
      
      this->tmp[ index ] = position ;
      this->lock.lock() ;
      this->copy_chain.synchronize() ;
      this->copy_chain.copy( this->tmp.data(), this->d_transforms ) ;
      this->copy_chain.submit() ;
      this->copy_chain.synchronize() ;
      this->lock.unlock() ;
    }
    
    void NyxDrawModelData::removeModel( unsigned index )
    {
      auto iter = this->drawables.find( index ) ;
      
      if( iter != this->drawables.end() )
      {
        this->drawables.erase( iter ) ;
      }
      
      this->dirty_flag = true ;
    }
    
    void NyxDrawModelData::setParentRef( const nyx::Chain<Impl>& parent )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent chain reference as ", reinterpret_cast<const void*>( &parent ) ) ;
      this->parent = &parent ;
    }
    
    void NyxDrawModelData::setParentPassRef( const nyx::RenderPass<Impl>& parent )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference as ", reinterpret_cast<const void*>(  &parent ) ) ;
      this->parent_pass = &parent ;
    }
    
    void NyxDrawModelData::setSubpass( unsigned index )
    {
      Log::output( "Module ", this->name.c_str(), " set input subpass as ", index ) ;
      this->subpass = index ;
    }

    void NyxDrawModelData::setViewRef( const Matrix& view )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference as ", reinterpret_cast<const void*>(  &view ) ) ;
      this->camera = &view ;
      this->vp_dirty_flag = true ;
    }
    
    void NyxDrawModelData::setProjRef( const Matrix& proj )
    {
      Log::output( "Module ", this->name.c_str(), " set input projection reference as ", reinterpret_cast<const void*>(  &proj ) ) ;
      this->projection = &proj ;
      this->vp_dirty_flag = true ;
    }
    
    const nyx::Chain<Impl>& NyxDrawModelData::chain()
    {
      Log::output( "Module ", this->name.c_str(), " emitting chain!" ) ;
      return this->draw_chain ;
    }
    
    void NyxDrawModelData::setDevice( unsigned id )
    {
      Log::output( "Module ", this->name.c_str(), " set input device as ", id ) ;
      this->device = id ;
    }

    void NyxDrawModelData::setParentRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input parent reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::setParentRef    , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawModelData::setParentPassRef, iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawModelData::setSubpass      , iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawModelData::setModelInputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input add model signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::setModel         , iris::OPTIONAL, name ) ;
      this->bus.enroll( this, &NyxDrawModelData::setModelTransform, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawModelData::setModelClearName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input remove model signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::removeModel, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawModelData::setViewRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input camera reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::setViewRef, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawModelData::setProjRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input projection reference name as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::setProjRef, iris::OPTIONAL, name ) ;
    }
    
    void NyxDrawModelData::setOutRefName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output reference as \"", name, "\"" ) ;
      
      this->bus.publish( this, &NyxDrawModelData::chain, name ) ;
    }
    
    void NyxDrawModelData::setInputNames( unsigned idx, const char* name )
    {
      idx = idx ;
      this->wait_and_publish.enroll( this, &NyxDrawModelData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxDrawModelData::setOutputName( const char* name )
    {
      this->wait_and_publish.publish( this, &NyxDrawModelData::signal, name ) ;
    }

    NyxDrawModelData::NyxDrawModelData()
    {
      this->viewport.setWidth ( 1280 ) ;
      this->viewport.setHeight( 1024 ) ;
      
      this->name        = ""      ;
      this->device      = 0       ;
      this->parent_pass = nullptr ;
      this->parent      = nullptr ;
    }
    
    // </editor-fold>
    
    // <editor-fold defaultstate="collapsed" desc="NyxDrawModel Function Definitions">
    NyxDrawModel::NyxDrawModel()
    {
      this->module_data = new NyxDrawModelData() ;
    }

    NyxDrawModel::~NyxDrawModel()
    {
      delete this->module_data ;
    }

    void NyxDrawModel::initialize()
    {
      if( data().parent_pass != nullptr )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.initialize( data().device, *data().parent_pass, nyx::bytes::draw_model, sizeof( nyx::bytes::draw_model ) ) ;
      }
      
      data().copy_chain.initialize( data().device, nyx::ChainType::Compute ) ;
      
      data().d_viewproj  .initialize( data().device, 1                                           ) ;
      data().d_transforms.initialize( data().device, 1024, false, nyx::ArrayFlags::UniformBuffer ) ;
      data().tmp         .resize( 1024 )                                                           ;
      
      std::fill( data().tmp.begin(), data().tmp.end(), glm::mat4( 1.0f ) ) ;
      
      data().lock.lock() ;
      data().copy_chain.copy( data().tmp.data(), data().d_transforms ) ;
      data().copy_chain.submit() ;
      data().copy_chain.synchronize() ;
      data().lock.unlock() ;
      
      data().draw_chain.setMode( nyx::ChainMode::All ) ;
      
      if( data().pipeline.initialized() )
      {
        data().pipeline.bind( "transform", data().d_transforms ) ;
        data().lock.lock() ;
        Impl::deviceSynchronize( data().device ) ;
        data().pipeline.bind( "mesh_texture", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
        Impl::deviceSynchronize( data().device ) ;
        data().lock.unlock() ;
      }
      mars::TextureArray<Impl>::addCallback( this->module_data, &NyxDrawModelData::updateTextures, this->name() ) ;
    }

    void NyxDrawModel::subscribe( unsigned id )
    {
      
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      
      data().bus.enroll( this->module_data, &NyxDrawModelData::setInputNames     , iris::OPTIONAL, this->name(), "::input"        ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setOutputName     , iris::OPTIONAL, this->name(), "::output"       ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setParentRefName  , iris::OPTIONAL, this->name(), "::parent"       ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setProjRefName    , iris::OPTIONAL, this->name(), "::projection"   ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setViewRefName    , iris::OPTIONAL, this->name(), "::camera"       ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setModelInputName , iris::OPTIONAL, this->name(), "::model_input"  ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setModelClearName , iris::OPTIONAL, this->name(), "::model_remove" ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setOutRefName     , iris::OPTIONAL, this->name(), "::reference"    ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setDevice         , iris::OPTIONAL, this->name(), "::device"       ) ;
    }

    void NyxDrawModel::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
    }

    void NyxDrawModel::execute()
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
        data().pipeline.initialize( data().device, *data().parent_pass, nyx::bytes::draw_model, sizeof( nyx::bytes::draw_model ) ) ;
        data().pipeline.bind( "transform", data().d_transforms ) ;
        data().lock.lock() ;
        Impl::deviceSynchronize( data().device ) ;
        data().pipeline.bind( "mesh_texture", mars::TextureArray<Impl>::images(), mars::TextureArray<Impl>::count() ) ;
        Impl::deviceSynchronize( data().device ) ;
        data().lock.unlock() ;
      }
      data().redrawModels() ;
      
      data().wait_and_publish.emit() ;
    }

    NyxDrawModelData& NyxDrawModel::data()
    {
      return *this->module_data ;
    }

    const NyxDrawModelData& NyxDrawModel::data() const
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
  return "NyxDrawModel" ;
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
  return new nyx::vkg::NyxDrawModel() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::vkg::NyxDrawModel* mod ;
  
  mod = dynamic_cast<nyx::vkg::NyxDrawModel*>( module ) ;
  delete mod ;
  // </editor-fold>
}
