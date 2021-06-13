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

#pragma once

#include <templates/NyxDrawModule.h>
#include <Mars/Font.h>
#include <Mars/Factory.h>
#include <Mars/Manager.h>
#include <Iris/data/Bus.h>

namespace nyx
{
  /** A module for managing converting images on the host to Vulkan images on the GPU.
   */
  class NyxDrawText2D : public nyx::NyxDrawModule<mars::Reference<mars::Font<nyx::vkg::Vulkan>>>
  {
    public:

      /** Default Constructor.
       */
      NyxDrawText2D() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      ~NyxDrawText2D() ;

      /** Method to initialize this module after being configured.
       */
      void initialize() ;

      /** Method to subscribe this module's configuration to the bus.
       * @param id The id to use for this graph.
       */
      void subscribe( unsigned id ) ;

      /** Method to shut down this object's operation.
       */
      void shutdown() ;

      /** Method to execute a single instance of this module's operation.
       */
      void execute() ;

    private:
      iris::Bus bus ;
  };
}