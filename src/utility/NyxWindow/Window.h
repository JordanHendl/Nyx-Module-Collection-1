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

namespace nyx
{
  namespace vkg
  {
    class Window : public iris::Module
    {
      public:
        
        /** Default Constructor.
         */
        Window() ;

        /** Virtual deconstructor. Needed for inheritance.
         */
        ~Window() ;

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
        
        /** Forward-declared structure to contain this object's internal data.
         */
        struct NyxWindowData *module_data ;

        /** Method to retrieve a reference to this object's internal data.
         * @return Reference to this object's internal data.
         */
        NyxWindowData& data() ;
        
        /** Method to retrieve a const-reference to this object's internal data.
         * @return Const-reference to this object's internal data.
         */
        const NyxWindowData& data() const ;
    };
  }
}