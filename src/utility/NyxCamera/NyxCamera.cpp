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
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>
#include <Iris/profiling/Timer.h>
#include <NyxGPU/event/Event.h>
#include <NyxGPU/vkg/Vulkan.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <string>
#include <array>
#include <chrono>
#include <thread>

static const unsigned VERSION = 1 ;
namespace nyx
{
  using Log = iris::log::Log ;
  
  struct CameraData
  {
    std::array<bool, 4> buttons        ;
    iris::Bus           bus            ;
    glm::mat4           camera         ;
    glm::vec3           position       ;
    glm::vec3           front          ;
    glm::vec3           right          ;
    glm::vec3           euler          ;
    glm::vec3           up             ;
    const glm::vec3*    target         ;
    std::string         name           ;
    
    nyx::EventManager manager ;

    /** Default constructor.
     */
    CameraData() ;
    
    void setInputTranslationName( const char* name ) ;
    void setInputTransformName( const char* name ) ;
    void setOutputName( const char* name ) ;
    void handleEvent( const nyx::Event& event ) ;
    void setLookAtName( const char* name ) ;
    void lookAt( const glm::vec3& position ) ;
    void translate( const glm::vec4& position ) ;
    void transform( const glm::mat4& trans ) ;
    const glm::mat4& view() ;
  };
  
  void CameraData::handleEvent( const nyx::Event& event )
  {
    if( event.type() == nyx::Event::Type::KeyDown )
    {
      switch( event.key() )
      {
        case nyx::Key::W : this->buttons[ 0 ] = true ; break ;
        case nyx::Key::S : this->buttons[ 1 ] = true ; break ;
        case nyx::Key::D : this->buttons[ 2 ] = true ; break ;
        case nyx::Key::A : this->buttons[ 3 ] = true ; break ;
        default : break ;
      }
    }
    else if( event.type() == nyx::Event::Type::KeyUp )
    {
      switch( event.key() )
      {
        case nyx::Key::W : this->buttons[ 0 ] = false ; break ;
        case nyx::Key::S : this->buttons[ 1 ] = false ; break ;
        case nyx::Key::D : this->buttons[ 2 ] = false ; break ;
        case nyx::Key::A : this->buttons[ 3 ] = false ; break ;
        default : break ;
      }
    }
  }
  
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

  void CameraData::setOutputName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set output camera matrix as \"", name, "\"" ) ;
    this->bus.publish( this, &CameraData::view, name ) ;
  }

  void CameraData::setLookAtName( const char* name )
  {
    Log::output( "Module ", this->name.c_str(), " set the look at signal as \"", name, "\"" ) ;
    this->bus.enroll( this, &CameraData::lookAt, iris::OPTIONAL, name ) ;
  }

  void CameraData::lookAt( const glm::vec3& position )
  {
    this->target = &position ;
    this->camera = glm::lookAt( this->position, this->position + *this->target, this->up ) ;
    this->bus.emit() ;
  }

  void CameraData::translate( const glm::vec4& position )
  {
//    glm::vec3 tmp = { position.x, position.y, position.z } ;
    position.length() ;
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
    this->right          = glm::vec3( 0.0f, 0.0f, -1.0f ) ;
    this->up             = glm::vec3( 0.0f, 1.0f, 0.0f  ) ;
    this->euler          = glm::vec3( -90.f, 0.0f, 0.0f );
    this->target         = &this->front                  ;
    this->camera         = glm::mat4( 1.0f )             ;
    this->position       = glm::vec3( 0.f, -2.0f, -1.f ) ;
    this->front          = glm::vec3( 0.0f, 1.0f, 0.0f ) ;
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
    data().camera = glm::lookAt( data().position, data().position + *data().target, data().up ) ;
    data().manager.enroll( this->module_data, &CameraData::handleEvent, this->name() ) ;
    data().bus.emit() ;
  }

  void Camera::subscribe( unsigned id )
  {
    
    data().bus.setChannel( id ) ;
    data().name = this->name() ;
    data().bus.enroll( this->module_data, &CameraData::setInputTransformName  , iris::OPTIONAL, this->name(), "::transform" ) ;
    data().bus.enroll( this->module_data, &CameraData::setInputTranslationName, iris::OPTIONAL, this->name(), "::translate" ) ;
    data().bus.enroll( this->module_data, &CameraData::setLookAtName          , iris::OPTIONAL, this->name(), "::target"    ) ;
    data().bus.enroll( this->module_data, &CameraData::setOutputName          , iris::OPTIONAL, this->name(), "::output"    ) ;
  }

  void Camera::shutdown()
  {
  }

  void Camera::execute()
  {
    glm::vec3 front     ;
    float     xoffset ;
    float     yoffset ;
    
    xoffset = data().manager.mouseDeltaX() ;
    yoffset = data().manager.mouseDeltaY() ;
    
    if( data().buttons[ 0 ] ) data().position += ( data().front * 0.01f ) ;
    if( data().buttons[ 1 ] ) data().position -= ( data().front * 0.01f ) ;
    if( data().buttons[ 2 ] ) data().position += ( data().right * 0.01f ) ;
    if( data().buttons[ 3 ] ) data().position -= ( data().right * 0.01f ) ;
    
    xoffset *= 0.1 ;
    yoffset *= 0.1 ;

    data().euler.x += xoffset ;
    data().euler.y += yoffset ;
    if ( data().euler.y > 89.0f  ) data().euler.y = 89.0f  ;
    if ( data().euler.y < -89.0f ) data().euler.y = -89.0f ;

    front.x = std::cos( glm::radians( data().euler.x ) * std::cos( glm::radians( data().euler.y ) ) ) ;
    front.y = std::sin( glm::radians( data().euler.y )                                              ) ;
    front.z = std::sin( glm::radians( data().euler.x ) * std::cos( glm::radians( data().euler.y ) ) ) ;

    data().front = glm::normalize( front                                                  ) ;
    data().right = glm::normalize( glm::cross( data().front, glm::vec3( 0.f, 1.0, 0.f ) ) ) ;
    data().up    = glm::normalize( glm::cross( data().right, data().front               ) ) ;

    data().camera = glm::lookAt( data().position, data().position + ( data().front ), data().up ) ;
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