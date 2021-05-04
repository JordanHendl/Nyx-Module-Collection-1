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

#include "NyxDummyPlayer.h"
#include <iris/data/Bus.h>
#include <iris/log/Log.h>
#include <iris/config/Configuration.h>
#include <iris/config/Parser.h>
#include <mars/Manager.h>
#include <mars/Model.h>
#include <mars/Texture.h>
#include <nyxgpu/vkg/Vulkan.h>
#include <nyxgpu/library/Image.h>
#include <nyxgpu/event/Event.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <limits>

static const unsigned VERSION = 1 ;

namespace nyx
{
  using Impl      = nyx::vkg::Vulkan                                  ;
  using ModelManager   = mars::Manager<unsigned, mars::Model<Impl>>   ;
  using TextureManager = mars::Manager<unsigned, mars::Texture<Impl>> ;
  using Log            = iris::log::Log                               ;
  using Token          = iris::config::json::Token                    ;
  
  struct DummyPlayerData
  {
    using ModelRequests   = std::vector<unsigned> ;
    using TextureRequests = std::vector<unsigned> ;
    
    iris::Bus   bus         ;
    std::string draw_signal ;
    std::string name        ;
    bool        ready       ;
    bool        dirty       ;
    bool        dirty_pos   ;
    unsigned    model_id    ;
    unsigned    model_slot  ;
    glm::vec3   position    ;
    glm::mat4   transform   ;

    nyx::EventManager manager ;
    
    /** Default constructor.
     */
    DummyPlayerData() ;
    void setRequiredModel( unsigned id ) ;
    void handleEvents( const nyx::Event& event ) ;
    void setWait( const char* signal ) ;
    void setSignal( const char* signal ) ;
    void setDrawer( const char* signal ) ;
    void setPosition( const Token& token ) ;
    void wait() ;
    void signal() ;
    void loaded( unsigned key, mars::Reference<mars::Model<Impl>> model ) ;
  };

  void DummyPlayerData::loaded( unsigned key, mars::Reference<mars::Model<Impl>> model )
  {
    key = key ;
    this->bus.emitIndexed( model, this->model_slot, this->draw_signal.c_str() ) ;
  }

  void DummyPlayerData::setRequiredModel( unsigned id )
  {
    this->model_id   = id                ;
    this->dirty      = true              ;
    this->transform  = glm::mat4( 1.0f ) ;
  }

  void DummyPlayerData::setWait( const char* signal )
  {
    this->bus.enroll( this, &DummyPlayerData::wait, iris::REQUIRED, signal ) ;
  }

  void DummyPlayerData::setSignal( const char* signal )
  {
    this->bus.enroll( this, &DummyPlayerData::signal, iris::REQUIRED, signal ) ;
  }
  
  void DummyPlayerData::setPosition( const Token& token )
  {
    auto x = token[ "x" ] ;
    auto y = token[ "y" ] ;
    auto z = token[ "z" ] ;
    
    if( x ) this->position.x = x.decimal() ;
    if( y ) this->position.y = y.decimal() ;
    if( z ) this->position.z = z.decimal() ;
    
    
    this->transform = glm::translate( this->transform, this->position ) ;
    
    this->dirty_pos = true ;
  }
  
  void DummyPlayerData::handleEvents( const nyx::Event& event )
  {
    if( event.type() == nyx::Event::Type::KeyDown )
    {
      switch( event.key() )
      {
        case nyx::Key::Up    : this->position.y -= 0.1f ; this->dirty_pos = true ; break ;
        case nyx::Key::Down  : this->position.y += 0.1f ; this->dirty_pos = true ; break ;
        case nyx::Key::Left  : this->position.z -= 0.1f ; this->dirty_pos = true ; break ;
        case nyx::Key::Right : this->position.z += 0.1f ; this->dirty_pos = true ; break ;
        default: break ;
      }
    }
  }
  
  void DummyPlayerData::setDrawer( const char* signal )
  {
    this->draw_signal = signal ;
  }

  void DummyPlayerData::wait()
  {
  
  }

  void DummyPlayerData::signal()
  {
  
  }

  DummyPlayerData::DummyPlayerData()
  {
    this->model_slot = 0          ;
    this->model_id   = UINT32_MAX ;
    this->ready      = false      ;
    this->dirty      = false      ;
    this->dirty_pos  = true       ;
  }

  DummyPlayer::DummyPlayer()
  {
    this->module_data = new DummyPlayerData() ;
  }

  DummyPlayer::~DummyPlayer()
  {
    delete this->module_data ;
  }

  void DummyPlayer::initialize()
  {
    data().manager.enroll( this->module_data, &DummyPlayerData::handleEvents, this->name() ) ;
  }

  void DummyPlayer::subscribe( unsigned id )
  {
    data().bus = iris::Bus() ;
    data().bus.setChannel( id ) ;
    data().name = this->name() ;
    
    data().bus.enroll( this->module_data, &DummyPlayerData::setRequiredModel, iris::OPTIONAL, this->name(), "::model"    ) ;
    data().bus.enroll( this->module_data, &DummyPlayerData::setWait         , iris::OPTIONAL, this->name(), "::wait"     ) ;
    data().bus.enroll( this->module_data, &DummyPlayerData::setSignal       , iris::OPTIONAL, this->name(), "::signal"   ) ;
    data().bus.enroll( this->module_data, &DummyPlayerData::setDrawer       , iris::OPTIONAL, this->name(), "::renderer" ) ;
    data().bus.enroll( this->module_data, &DummyPlayerData::setPosition     , iris::OPTIONAL, this->name(), "::position" ) ;
  }

  void DummyPlayer::shutdown()
  {
  }

  void DummyPlayer::execute()
  {
    data().bus.wait() ;
    
    if( data().dirty && data().model_id != UINT32_MAX )
    {
      ModelManager::request( this->module_data, &DummyPlayerData::loaded, data().model_id ) ;
      data().dirty = false ;
    }
      
    if( data().dirty_pos )
    { 
      data().transform = glm::mat4( 1.0f ) ;
      data().transform = glm::translate( data().transform, data().position ) ;
      data().transform = glm::rotate( data().transform, glm::radians( 180.f ), glm::vec3( 0, 0, 1.0f ) ) ;
      data().bus.emitIndexed( data().transform, data().model_slot, data().draw_signal.c_str() ) ;
      data().dirty_pos = false ;
    }

    data().bus.emit() ;
  }

  DummyPlayerData& DummyPlayer::data()
  {
    return *this->module_data ;
  }

  const DummyPlayerData& DummyPlayer::data() const
  {
    return *this->module_data ;
  }
}

/** Exported function to retrieve the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxDummyPlayer" ;
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
  return new ::nyx::DummyPlayer() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::DummyPlayer* mod ;
  
  mod = dynamic_cast<::nyx::DummyPlayer*>( module ) ;
  delete mod ;
}