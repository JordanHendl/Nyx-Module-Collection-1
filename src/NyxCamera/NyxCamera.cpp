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

#include "NyxCamera.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <string>

static const unsigned VERSION = 1 ;
namespace nyx
{
  using Log = iris::log::Log ;
  
  struct CameraData
  {
    iris::Bus   bus      ;
    glm::mat4   camera   ;
    glm::vec3   position ;
    std::string name     ;

    /** Default constructor.
     */
    CameraData() ;
    
    void setInputTranslationName( const char* name ) ;
    void setInputTransformName( const char* name ) ;
    void setOutputName( unsigned index, const char* name ) ;
    void setLookAtName( const char* name ) ;
    void lookAt( const glm::vec4& position ) ;
    void translate( const glm::vec4& position ) ;
    void transform( const glm::mat4& trans ) ;
    const glm::mat4& view() ;
  };
  
  void CameraData::setInputTranslationName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set input translation signal as \"", name, "\"" ) ;
    this->bus.enroll( this, &CameraData::translate, iris::OPTIONAL, name ) ;
  }

  void CameraData::setInputTransformName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set input transformation signal as \"", name, "\"" ) ;
    this->bus.enroll( this, &CameraData::transform, iris::OPTIONAL, name ) ;
  }

  void CameraData::setOutputName( unsigned index, const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set output camera matrix as \"", name, "\"" ) ;
    switch( index )
    {
      default: this->bus.publish( this, &CameraData::view, name ) ;
    }
  }

  void CameraData::setLookAtName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set the look at signal as \"", name, "\"" ) ;
    this->bus.enroll( this, &CameraData::lookAt, iris::OPTIONAL, name ) ;
  }

  void CameraData::lookAt( const glm::vec4& position )
  {
    glm::vec3 tmp = { position.x, position.y, position.z } ;
    this->camera = glm::lookAt( this->position, tmp, glm::vec3( 0, 1, 0 ) ) ;
    this->bus.emit() ;
  }

  void CameraData::translate( const glm::vec4& position )
  {
    glm::vec3 tmp = { position.x, position.y, position.z } ;
    this->camera = glm::translate( this->camera, tmp ) ;
    this->bus.emit() ;
  }

  void CameraData::transform( const glm::mat4& trans )
  {
    this->camera *= trans ;
    this->bus.emit() ;
  }

  const glm::mat4& CameraData::view()
  {
    return this->camera ;
  }

  CameraData::CameraData()
  {
    this->camera   = glm::mat4( 1.0f ) ;
    this->position = glm::vec4( 1.0f ) ;
  }

  Camera::Camera()
  {
    this->module_data = new CameraData() ;
  }

  Camera::~Camera()
  {
    delete this->module_data ;
  }

  void Camera::initialize()
  {
    data().bus.emit() ;
  }

  void Camera::subscribe( unsigned id )
  {
    data().bus = iris::Bus() ;
    data().bus.setChannel( id ) ;
    data().name = this->name() ;
    data().bus.enroll( this->module_data, &CameraData::setInputTransformName  , iris::OPTIONAL, this->name(), "::transform" ) ;
    data().bus.enroll( this->module_data, &CameraData::setInputTranslationName, iris::OPTIONAL, this->name(), "::translate" ) ;
    data().bus.enroll( this->module_data, &CameraData::setLookAtName          , iris::OPTIONAL, this->name(), "::lookat"    ) ;
    data().bus.enroll( this->module_data, &CameraData::setOutputName          , iris::OPTIONAL, this->name(), "::outputs"   ) ;
  }

  void Camera::shutdown()
  {
  }

  void Camera::execute()
  {
    
  }

  CameraData& Camera::data()
  {
    return *this->module_data ;
  }

  const CameraData& Camera::data() const
  {
    return *this->module_data ;
  }
}

/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxCamera" ;
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
  return new ::nyx::Camera() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::Camera* mod ;
  
  mod = dynamic_cast<::nyx::Camera*>( module ) ;
  delete mod ;
}