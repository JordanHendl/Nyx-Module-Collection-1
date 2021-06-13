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

#include "NyxStartDraw.h"
#include <Athena/Manager.h>
#include <Iris/data/Bus.h>
static nyx::vkg::NyxStartDraw converter ;
static iris::Bus                bus       ;
static athena::Manager          manager   ;

int main()
{
  converter.setName( "test" ) ;
  converter.subscribe( 0 ) ;
  converter.initialize() ;
  bus.emit<const char*>( "width"     , "test::input_width"    ) ;
  bus.emit<const char*>( "height"    , "test::input_height"   ) ;
  bus.emit<const char*>( "channel"   , "test::input_channels" ) ;
  bus.emit<const char*>( "bytes"     , "test::input_bytes"    ) ;
  bus.emit<const char*>( "output"    , "test::output"         ) ;
  bus.emit<const char*>( "cmd_output", "test::cmd_output"     ) ;
  
  converter.execute() ; // SHOULD halt ;
  
  return 0 ;
}