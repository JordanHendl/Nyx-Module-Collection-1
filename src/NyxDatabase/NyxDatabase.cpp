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

#include "NyxDatabase.h"
#include "converted_kitty.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/config/Configuration.h>
#include <iris/config/Parser.h>
#include <mars/Manager.h>
#include <mars/Model.h>
#include <mars/Texture.h>
#include <mars/TextureArray.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <nyxgpu/library/Image.h>
#include <string>

static const unsigned VERSION = 1 ;
namespace nyx
{
  using Framework      = nyx::vkg::Vulkan                                   ;
  using ModelManager   = mars::Manager<unsigned, mars::Model<Framework>>    ;
  using TextureManager = mars::Manager<unsigned, mars::Texture<Framework>>  ;
  using TextureArray   = mars::TextureArray<Framework>                      ;
  using Log            = iris::log::Log                                     ;
  
  struct DatabaseData
  {
    using ModelRequests    = std::vector<std::pair<unsigned, ModelManager  ::Callback*>> ;
    using TextureRequests  = std::vector<std::pair<unsigned, TextureManager::Callback*>> ;
    
    mars::Texture<Framework>    default_tex     ;
    iris::Bus                   bus             ;
    std::string                 name            ;
    std::string                 json_path       ;
    ModelRequests               model_request   ;
    TextureRequests             texture_request ;
    iris::config::Configuration database        ;
    unsigned                    device          ;
    
    /** Default constructor.
     */
    DatabaseData() ;
    
    void wait() {} ;
    void signal() {} ;
    
    std::string seekTexturePath( unsigned tex_id ) ;
    void requestModel( unsigned model_id, ModelManager::Callback* cb ) ;
    void requestTexture( unsigned texture_id, TextureManager::Callback* cb ) ;
    void loadModels() ;
    void loadTextures() ;
    void loadTexture( unsigned id ) ;
    void setInputNames( unsigned idx, const char* name ) ;
    void setOutputName( const char* name ) ;
    void setDatabaseJSON( const char* name ) ;
    void setDevice( unsigned id ) ;
  };
  
  void DatabaseData::loadTexture( unsigned tex_id ) 
  {
    const auto token = this->database.begin()[ "models" ] ;
    unsigned    id   ;
    std::string path ;

    for( unsigned index = 0; index < token.size(); index++ )
    {
      auto tex = token.token( index ) ;
           id    = tex[ "ID"   ].number() ;
           path  = tex[ "Path" ].string() ;

      if( id == tex_id && !TextureManager::has( id ) )
      {
        auto ref = TextureManager::create( id, path.c_str(), this->device ) ;
        TextureArray::set( id, *ref ) ;
        TextureArray::signal() ;
      }
    }
  }

  void DatabaseData::loadModels()
  {
    const auto token = this->database.begin()[ "models" ] ;
    unsigned    id        ;
    std::string path      ;
    std::string mesh_name ;

    if( this->database.isInitialized() )
    {
      for( auto& model_id : this->model_request )
      {
        for( unsigned index = 0; index < token.size(); index++ )
        {
          auto model     = token.token( index ) ;
          auto materials = model[ "materials" ] ;
          id   = model[ "ID"   ].number() ;
          path = model[ "Path" ].string() ;
          
          if( id == model_id.first && !ModelManager::has( id ) && model_id.second )
          {
            auto ref = ModelManager::create( id, path.c_str(), this->device ) ;

            for( unsigned material_index = 0; material_index < materials.size(); material_index++ )
            {
              unsigned index      = 0                                 ;
              unsigned mesh_index = UINT32_MAX                        ;
              auto&    meshes     = ref->meshes()                     ;
              auto     mat        = materials.token( material_index ) ;
              auto     diffuse    = mat[ "diffuse" ]                  ;
//              auto  color   = mat[ "color" ] ;
               
              mesh_name = mat[ "mesh" ].string() ;
              index = 0 ;
              for( auto& mesh : meshes )
              {
                if( mesh->name == mesh_name )
                {
                  mesh_index = index ;
                }
                
                index++ ;
              }
              
              if( mesh_index != UINT32_MAX )
              {
                if( diffuse )
                {
                  ref->setTexture( mesh_index, "diffuse", diffuse.number() ) ;
                  this->loadTexture( diffuse.number() ) ;
                }
              }
            }
            
            model_id.second->callback( id, ref ) ;
            delete model_id.second ;
            model_id.second = nullptr ;
          }
          else if( ModelManager::has( id ) && model_id.second )
          {
            auto ref = ModelManager::reference( id ) ;
            model_id.second->callback( id, ref ) ;
            delete model_id.second ;
            model_id.second = nullptr ;
          }
        }
        if( model_id.second )
        {
          delete model_id.second ;
        }
      }
      this->model_request.clear() ;
    }
  }

  void DatabaseData::loadTextures() 
  {
    const auto token = this->database.begin()[ "models" ] ;
    unsigned    id        ;
    std::string path      ;
    bool        dirty     ;
    
    dirty = false ;
    if( this->database.isInitialized() )
    {
      for( auto& tex_req : this->texture_request )
      {
        for( unsigned index = 0; index < token.size(); index++ )
        {
          auto tex     = token.token( index ) ;
          id   = tex[ "ID"   ].number() ;
          path = tex[ "Path" ].string() ;
          
          if( id == tex_req.first && !TextureManager::has( id ) && tex_req.second )
          {
            auto ref = TextureManager::create( id, path.c_str(), this->device ) ;

            tex_req.second->callback( id, ref ) ;
            delete tex_req.second ;
            tex_req.second = nullptr ;
            
            TextureArray::set( id, *ref ) ;
            dirty = true ;
          }
          else if( TextureManager::has( id ) && tex_req.second )
          {
            auto ref = TextureManager::reference( id ) ;
            tex_req.second->callback( id, ref ) ;
            delete tex_req.second ;
            tex_req.second = nullptr ;
          }
        }
        if( tex_req.second )
        {
          delete tex_req.second ;
        }
      }
      
      if( dirty ) TextureArray::signal() ;
      this->texture_request.clear() ;
    }
  }

  void DatabaseData::setDatabaseJSON( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set database JSON file as \"", name, "\"" ) ;
    this->json_path = name ;
  }
  
  void DatabaseData::requestModel( unsigned model_id, ModelManager::Callback* cb )
  {
    this->model_request.push_back( { model_id, cb } ) ;
  }
  
  void DatabaseData::requestTexture( unsigned texture_id, TextureManager::Callback* cb )
  {
    this->texture_request.push_back( { texture_id, cb } ) ;
  }

  void DatabaseData::setInputNames( unsigned idx, const char* name )
  {
    idx = idx ;
    Log::output( "Module ", this->name.c_str(), " set required input signal as \"", name, "\"" ) ;
    this->bus.enroll( this, &DatabaseData::wait, iris::REQUIRED, name ) ;
  }
  
  void DatabaseData::setOutputName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set output signal as \"", name, "\"" ) ;
    this->bus.publish( this, &DatabaseData::signal, name ) ;
  }
  
  void DatabaseData::setDevice( unsigned id )
  {
    Log::output( "Module ", this->name.c_str(), " set device ID as \"", id, "\"" ) ;
    this->device = id ;
  }

  DatabaseData::DatabaseData()
  {
    ModelManager  ::addFulfiller( this, &DatabaseData::requestModel  , 0 ) ;
    TextureManager::addFulfiller( this, &DatabaseData::requestTexture, 0 ) ;
    
    this->device    = 0  ;
    this->name      = "" ;
    this->json_path = "" ;
  }

  Database::Database()
  {
    this->module_data = new DatabaseData() ;
  }

  Database::~Database()
  {
    delete this->module_data ;
  }

  void Database::initialize()
  {
    data().model_request.reserve( 100 ) ;
    data().texture_request  .reserve( 100 ) ;
    
    if( !data().json_path.empty() ) data().database.initialize( data().json_path.c_str() ) ;
    data().default_tex.initialize( nyx::bytes::converted_kitty, sizeof( nyx::bytes::converted_kitty ), data().device ) ;
    
    mars::TextureArray<Framework>::initialize( 2048 ) ;
    
    for( unsigned index = 0; index < 2048; index++ )
    {
      mars::TextureArray<Framework>::set( index, data().default_tex ) ;
    }
  }

  void Database::subscribe( unsigned id )
  {
    data().bus = iris::Bus() ;
    data().bus.setChannel( id ) ;
    data().name = this->name() ;
    data().bus.enroll( this->module_data, &DatabaseData::setInputNames         , iris::OPTIONAL, this->name(), "::inputs"         ) ;
    data().bus.enroll( this->module_data, &DatabaseData::setOutputName         , iris::OPTIONAL, this->name(), "::output"         ) ;
    data().bus.enroll( this->module_data, &DatabaseData::setDatabaseJSON       , iris::OPTIONAL, this->name(), "::path"           ) ;
    data().bus.enroll( this->module_data, &DatabaseData::setDevice             , iris::OPTIONAL, this->name(), "::device"         ) ;
  }

  void Database::shutdown()
  {
  }

  void Database::execute()
  {
    data().bus.wait() ;
    data().loadTextures() ;
    data().loadModels  () ;
    data().bus.emit() ;
  }

  DatabaseData& Database::data()
  {
    return *this->module_data ;
  }

  const DatabaseData& Database::data() const
  {
    return *this->module_data ;
  }
}

/** Exported function to retrieve the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDatabase" ;
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
  return new ::nyx::Database() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::Database* mod ;
  
  mod = dynamic_cast<::nyx::Database*>( module ) ;
  delete mod ;
}