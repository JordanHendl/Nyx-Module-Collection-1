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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "NyxDatabase.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <mars/Manager.h>
#include <mars/Model.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <nyxgpu/library/Image.h>
#include <string>

static const unsigned VERSION = 1 ;
namespace nyx
{
  using Framework      = nyx::vkg::Vulkan                                ;
  using ModelManager   = mars::Manager<unsigned, mars::Model<Framework>> ;
  using TextureManager = mars::Manager<unsigned, nyx::Image<Framework>>  ;
  using Log            = iris::log::Log                                  ;
  
  struct DatabaseData
  {
    iris::Bus   bus  ;
    std::string name ;

    /** Default constructor.
     */
    DatabaseData() ;
    
    void setModelRequestSignal  ( const char* name ) ;
    void setTextureRequestSignal( const char* name ) ;
  };
  
  DatabaseData::DatabaseData()
  {
    
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
    data().bus.emit() ;
  }

  void Database::subscribe( unsigned id )
  {
    data().bus = iris::Bus() ;
    data().bus.setChannel( id ) ;
    data().name = this->name() ;
//    data().bus.enroll( this->module_data, &DatabaseData::setInputTransformName  , iris::OPTIONAL, this->name(), "::transform" ) ;
  }

  void Database::shutdown()
  {
  }

  void Database::execute()
  {
    
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

/** Exported function to retrive the name of this module type.
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