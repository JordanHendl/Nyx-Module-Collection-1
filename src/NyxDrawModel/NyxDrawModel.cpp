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

#include "NyxDrawModel.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/profiling/Timer.h>
#include <iris/config/Parser.h>
#include <nyxgpu/library/Array.h>
#include <nyxgpu/library/Image.h>
#include <nyxgpu/library/Chain.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <mars/Model.h>
#include <mars/Manager.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <string>
#include <vector>
#include <climits>
#include <mutex>
#include <map>

static const unsigned VERSION = 1 ;
namespace nyx
{
  namespace vkg
  {
    using Log     = iris::log::Log                             ;
    using Impl    = nyx::vkg::Vulkan                           ;
    using Manager = mars::Manager<unsigned, mars::Model<Impl>> ;
    
    struct NyxDrawModelData
    {
      using Matrix     = glm::mat4 ;
      using ModelPair  = std::tuple<mars::Reference<mars::Model<Impl>>, glm::mat4> ;
      using Models     = std::map<unsigned, ModelPair> ;
      using ModelIdVec = std::vector<unsigned> ;
      
      struct ViewProj
      {
        Matrix view_proj ;
      };
      
      struct Iterators
      {
        nyx::Iterator<Impl, glm::mat4> positions ;
        nyx::Iterator<Impl, ViewProj > viewproj  ;
        unsigned                       index     ;
      };
      
      nyx::Viewport                viewport         ;
      ViewProj                     mvp              ;
      nyx::Array<Impl, glm::mat4>  d_model_trans    ;
      nyx::Array<Impl, ViewProj>   d_viewproj       ;
      std::vector<Iterators>       iterators        ;
      ModelIdVec                   not_loaded       ;
      Models                       drawables        ;
      const Matrix*                camera           ;
      const Matrix*                projection       ;
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
      std::string                  pipeline_path    ;
      std::string                  database_req     ;
      bool                         dirty_flag       ;
      bool                         vp_dirty_flag    ;
      std::mutex                   lock             ;
      
      /** Default constructor.
       */
      NyxDrawModelData() ;
      
      /** Helper method to check and see if any models we were requested but found not loaded, have been loaded.
       */
      void checkForLoaded() ;

      /** Helper method to synchronize the VP matrix from the host to the device.
       */
      void syncVPMatrix() ;
      
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
      void setModel( unsigned index, unsigned model_id ) ;
      
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
      
      /** Method to set the path to the pipeline to use for drawing models.
       * @param path The path to the pipeline on disk used for drawing models.
       */
      void setPipelinePath( const char* path ) ;

      /** Method to set the output name of this module.
       * @param name The name to associate with this module's output.
       */
      void setOutputName( unsigned idx, const char* name ) ;
      
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
      
      /** Method to set then name of the input signal to wait on.
       * @param name The name of the signal to wait on.
       */
      void setWaitSignalName( const char* name ) ;
      
      /** Method to set then name of the input signal to signal when operation is finished.
       * @param name The name of the signal to signal.
       */
      void setFinishSignalName( const char* name ) ;

      /** Method to set the name of the database's request signal.
       * @param name The name to send the signal for a requested model to.
       */
      void setDatabaseRequest( const char* name ) ;

      /** Method to set the name of the input width parameter.
       * @param name The name to associate with the input.
       */
      void setInputName( unsigned idx, const char* name ) ;
      
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
    
    void NyxDrawModelData::checkForLoaded()
    {
      for( unsigned index = 0; index < this->not_loaded.size(); index++ )
      {
        auto ref = Manager::reference( this->not_loaded[ index ] ) ;
        if( ref )
        {
          this->drawables.insert( { index, std::make_tuple( ref, glm::mat4() ) } ) ;
          this->not_loaded.erase( this->not_loaded.begin() + index ) ;
          this->dirty_flag = true ;
        }
      }
    }
    
    void NyxDrawModelData::syncVPMatrix()
    {
      if( this->vp_dirty_flag && this->copy_chain.initialized() ) 
      {
        this->copy_chain.copy( &this->mvp, this->d_viewproj, 1 )  ;
        this->copy_chain.submit() ;
        this->vp_dirty_flag = false ;
      }
    }
    
    void NyxDrawModelData::redrawModels()
    {
      unsigned index ;
      
      if( this->dirty_flag && this->draw_chain.initialized() )
      {
        index = 0 ;
        this->iterators.resize( this->drawables.size() ) ;
        for( auto& model : this->drawables )
        {
          this->iterators[ index ].index     = index                          ;
          this->iterators[ index ].positions = this->d_model_trans.iterator() ;
          this->iterators[ index ].viewproj  = this->d_viewproj.iterator()    ;
          
          this->draw_chain.push( this->pipeline, this->iterators[ index ] ) ;
          std::get<0>( model.second )->draw( this->pipeline, this->draw_chain ) ;
        }
        this->dirty_flag = false ;
        this->bus.emit() ;
      }
    }
    
    void NyxDrawModelData::wait()
    {
      
    }
    
    void NyxDrawModelData::signal()
    {
      
    }
    
    void NyxDrawModelData::setInitialModels( unsigned index, unsigned model_id )
    {
      if( this->not_loaded.size() < index ) this->not_loaded.resize( index + 1 ) ;
      this->not_loaded[ index ] = model_id ;
    }
    
    void NyxDrawModelData::setModel( unsigned index, unsigned model_id )
    {
      auto ref = Manager::reference( model_id ) ;
      if( ref ) this->drawables.insert( { index, std::make_tuple( ref, glm::mat4() ) } ) ;
      else
      {
        this->not_loaded.push_back( model_id ) ;
        this->bus.emit( model_id, this->database_req.c_str() ) ;
      }
      
      this->dirty_flag = true ;
    }
    
    void NyxDrawModelData::setModelTransform( unsigned index, const glm::mat4& position )
    {
      if( this->d_model_trans.size() < index )
      {
        this->d_model_trans.reset() ;
        this->d_model_trans.initialize( this->device, index + 1024 ) ;
      }
      
      this->copy_chain.copy( &position, this->d_model_trans, 1, 0, index ) ;
      this->copy_chain.submit() ;
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
    
    void NyxDrawModelData::setPipelinePath( const char* path )
    {
      Log::output( "Module ", this->name.c_str(), " set pipeline path as \"", path, "\"" ) ;
      this->pipeline_path = path ;
    }
    
    void NyxDrawModelData::setParentRef( const nyx::Chain<Impl>& parent )
    {
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
    
    void NyxDrawModelData::setInputName( unsigned idx, const char* name )
    {
      idx  = idx  ;
      name = name ;
    }
    
    void NyxDrawModelData::setModelInputName( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set input add model signal as \"", name, "\"" ) ;
      this->bus.enroll( this, &NyxDrawModelData::setModel, iris::OPTIONAL, name ) ;
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
    
    void NyxDrawModelData::setOutputName( unsigned idx, const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set output ", idx, " as \"", name, "\"" ) ;
      
      switch( idx )
      {
        default: this->bus.publish( this, &NyxDrawModelData::chain, name ) ; this->wait_and_publish.publish( this, &NyxDrawModelData::signal, name ) ; break ;
      }
    }
    
    void NyxDrawModelData::setDatabaseRequest( const char* name )
    {
      Log::output( "Module ", this->name.c_str(), " set database request signal name as \"", name, "\"" ) ;
      this->database_req = name ;
    }
    
    void NyxDrawModelData::setWaitSignalName( const char* name )
    {
      this->wait_and_publish.enroll( this, &NyxDrawModelData::wait, iris::REQUIRED, name ) ;
    }
    
    void NyxDrawModelData::setFinishSignalName( const char* name )
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
      if( data().pipeline_path != "" && data().parent_pass != nullptr )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.initialize( data().device, *data().parent_pass, data().pipeline_path.c_str() ) ;
      }
      
      data().d_viewproj.initialize( data().device, 1 ) ;
      data().copy_chain.initialize( data().device, nyx::ChainType::Compute ) ;
      
      for( auto& id : data().not_loaded )
      {
        data().bus.emit( id, data().database_req.c_str() ) ;
      }
    }

    void NyxDrawModel::subscribe( unsigned id )
    {
      data().bus = iris::Bus() ;
      data().bus.setChannel( id ) ;
      data().name = this->name() ;
      
      data().bus.enroll( this->module_data, &NyxDrawModelData::setFinishSignalName, iris::OPTIONAL, this->name(), "::finish_signal"  ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setWaitSignalName  , iris::OPTIONAL, this->name(), "::wait_on"        ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setPipelinePath    , iris::OPTIONAL, this->name(), "::pipeline"       ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setParentRefName   , iris::OPTIONAL, this->name(), "::parent"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setDatabaseRequest , iris::OPTIONAL, this->name(), "::database"       ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setProjRefName     , iris::OPTIONAL, this->name(), "::projection"     ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setViewRefName     , iris::OPTIONAL, this->name(), "::camera"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setInitialModels   , iris::OPTIONAL, this->name(), "::initial_models" ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setModelInputName  , iris::OPTIONAL, this->name(), "::model_input"    ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setModelClearName  , iris::OPTIONAL, this->name(), "::model_remove"   ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setInputName       , iris::OPTIONAL, this->name(), "::inputs"         ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setOutputName      , iris::OPTIONAL, this->name(), "::outputs"        ) ;
      data().bus.enroll( this->module_data, &NyxDrawModelData::setDevice          , iris::OPTIONAL, this->name(), "::device"         ) ;
    }

    void NyxDrawModel::shutdown()
    {
      Impl::deviceSynchronize( data().device ) ;
    }

    void NyxDrawModel::execute()
    {
      data().wait_and_publish.wait() ;
      
      data().checkForLoaded() ;
      data().syncVPMatrix() ;
      
      if( !data().draw_chain.initialized() && data().parent != nullptr )
      {
        data().draw_chain.initialize( *data().parent, data().subpass ) ;
      }
      if( !data().pipeline.initialized() && data().parent_pass && !data().pipeline_path.empty() )
      {
        data().pipeline.addViewport( data().viewport ) ;
        data().pipeline.initialize( data().device, *data().parent_pass, data().pipeline_path.c_str() ) ;
      }
      else
      {
        data().redrawModels() ;
      }
      
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
}

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
}
