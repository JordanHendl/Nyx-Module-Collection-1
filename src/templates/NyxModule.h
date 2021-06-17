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

#include <Iris/module/Module.h>
#include <Iris/data/Bus.h>
#include <Iris/log/Log.h>

namespace nyx
{
  class NyxModule : public ::iris::Module
  {
    public:

      /** Default Constructor.
       */
      NyxModule() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      virtual ~NyxModule() ;

      virtual void subscribe( iris::Bus& bus ) ;
      
      inline unsigned gpu() const ;
      
    private:
      
      void setGpu( unsigned id ) ;
      
      unsigned gpu_id = 0 ;
  };
  
  
  NyxModule::NyxModule()
  {
    this->gpu_id = 0 ;
  }
  
  NyxModule::~NyxModule()
  {
  
  }
  
  void NyxModule::subscribe( iris::Bus& bus )
  {
    bus.enroll( this, &NyxModule::setGpu, iris::OPTIONAL, this->name(), "::gpu" ) ;
  }
  
  unsigned NyxModule::gpu() const
  {
    return this->gpu_id ;
  }

  void NyxModule::setGpu( unsigned id )
  {
    this->gpu_id = id ;
  }
}