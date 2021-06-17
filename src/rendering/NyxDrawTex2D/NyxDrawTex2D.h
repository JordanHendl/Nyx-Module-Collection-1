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
#include <Mars/Model.h>
#include <Iris/data/Bus.h>

namespace nyx
{
  /** A module for managing converting images on the host to Vulkan images on the GPU.
   */
  class NyxDrawTex2D : public nyx::NyxDrawModule<unsigned>
  {
    public:

      /** Default Constructor.
       */
      NyxDrawTex2D() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      ~NyxDrawTex2D() ;

      /** Method to initialize this module after being configured.
       */
      void initialize() ;

      /** Method to subscribe this module's configuration to the bus.
       * @param id The id to use for this graph.
       */
      void subscribe( unsigned id ) ;
      
      /** Method to update texture arrays.
       */
      void updateTextures() ;
      
      /** Method to shut down this object's operation.
       */
      void shutdown() ;

      /** Method to execute a single instance of this module's operation.
       */
      void execute() ;

    private:
      struct NyxDrawTex2DData
      {
        nyx::Chain<Framework>             copy_chain ;
        nyx::Array<Framework, glm::mat4 > d_viewproj ;
        nyx::Array<Framework, unsigned  > d_indices  ;
        nyx::Array<Framework, glm::vec4 > d_vertices ;
        std::vector<unsigned>             indices    ;
        iris::Bus                         bus        ;
        bool                              dirty      ;
        const glm::mat4*                  projection ;
        const glm::mat4*                  camera     ;
        unsigned                          count      ;
        
        NyxDrawTex2DData()                                      { this->dirty = false ; this->projection = nullptr ; this->camera = nullptr ;         } ;
        void setProjectionInput( const char* input            ) { this->bus.enroll( this, &NyxDrawTex2DData::setProjection, iris::OPTIONAL, input ) ; } ;
        void setCameraInput    ( const char* input            ) { this->bus.enroll( this, &NyxDrawTex2DData::setCamera    , iris::OPTIONAL, input ) ; } ;
        void setProjection     ( const glm::mat4& val         ) { this->projection = &val ; this->dirty = true ;                                      } ;
        void setCamera         ( const glm::mat4& val         ) { this->camera     = &val ; this->dirty = true ;                                      } ;
        
        void draw( nyx::Chain<Framework>& chain, nyx::Pipeline<Framework>& pipeline )
        {
          chain.begin() ;
          chain.draw( pipeline, this->d_vertices ) ; 
          chain.end() ;
        };
        
        void updateViewProj()
        {
          glm::mat4 viewproj ;
          if( this->dirty && this->projection != nullptr )
          {
            viewproj = ( glm::mat4( 1.0f ) ) ; 
            
            this->copy_chain.copy( &viewproj, this->d_viewproj ) ;
            this->copy_chain.submit     () ;
            this->copy_chain.synchronize() ;
          }
        }
        
        void updateTextureIds()
        {
          this->copy_chain.copy( this->indices.data(), this->d_indices ) ;
          this->copy_chain.submit     () ;
          this->copy_chain.synchronize() ;
        }
      };
      
      bool             updated_textures ;
      NyxDrawTex2DData data_2d          ;
      iris::Bus        bus              ;
  };
}