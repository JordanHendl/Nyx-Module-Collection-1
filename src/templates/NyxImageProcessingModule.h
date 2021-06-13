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

#include <climits>
#include <templates/NyxModule.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/library/Pipeline.h>
#include <NyxGPU/vkg/Vulkan.h>

#include <unordered_map>
#include <functional>

namespace nyx
{
  using Framework = nyx::vkg::Vulkan ;
  using Log       = iris::log::Log   ;
  
  static void signal() {}
  
  class NyxImageProcessingModule : public nyx::NyxModule
  {
    public:
      
      using DispatchCallback = std::function<void( nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> ;

      /** Default Constructor.
       */
      NyxImageProcessingModule() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      virtual ~NyxImageProcessingModule() ;
      
      /** Method to iniitalize this object & allocate the pipeline, chain, and transformation buffers.
       */
      virtual void initialize() ;
      
      /** Method to subscribe the needed things of this object with a bus.
       * @param The bus to use for subscribing.
       */
      virtual void subscribe( iris::Bus& bus ) ;
      
      /** Method to shutdown this object and deallocate all allocated data.
       */
      virtual void shutdown() ;
      
      /** Method to set the pipeline used by the child module.
       * @param bytes The bytes of the .NYX file.
       * @param size The size of the bytes.
       */
      void setPipeline( const unsigned char* bytes, unsigned size ) ;
      
      /** Method to retrieve a reference to this object's render chain.
       * @return Reference to this object's render chain.
       */
      nyx::Chain<Framework>& chain() ;
      
      /** Method to draw this object's data. 
       * Calls the specified input callback for every element being drawn.
       * For setting the callback, see @setPerDrawCallback.
       */
      void dispatch() ;
      
      nyx::Pipeline<Framework>& pipeline() ;
      
      void setCallback( std::function<void( nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> callback ) ;

    private:
      void setFinishSignal( const char* name ) ;
      
      DispatchCallback         callback         ;  
      nyx::ArrayFlags          array_flag       ;
      const unsigned char*     pipeline_bytes   ;
      unsigned                 pipeline_size    ;
      iris::Bus*               child_bus        ;
      nyx::Chain<Framework>    compute_chain    ;
      nyx::Pipeline<Framework> compute_pipeline ;
      bool                     initialized      ;
  };
  
  NyxImageProcessingModule::NyxImageProcessingModule()
  {
    this->compute_chain.setMode( nyx::ChainMode::All ) ;

    this->array_flag            = nyx::ArrayFlags::UniformBuffer ;
    this->initialized           = false                          ;
    this->child_bus             = nullptr                        ;
    this->pipeline_bytes        = nullptr                        ;
    this->pipeline_size         = 0                              ;
  }
  
  NyxImageProcessingModule::~NyxImageProcessingModule()
  {
    if( this->compute_chain.initialized() ) this->compute_chain.reset() ;
    
    this->child_bus    = nullptr ;
  }
  
  void NyxImageProcessingModule::initialize()
  {
    Log::output( "NyxImageProcessingModule ", this->name(), " initializing..." ) ;
    
    if( !this->pipeline_bytes || this->pipeline_size == 0 )
    {
      Log::output( Log::Level::Fatal, " Trying to initialize a Nyx Image Processing Module without having set the proper parameters first. See file " __FILE__, " at line ", __LINE__, ".\n",
        "Parameters: \n",
        "--Pipeline Bytes Valid", this->pipeline_bytes == nullptr,  "\n", 
        "--Pipeline Byte Size  ", this->pipeline_size,              "\n" ) ;     
    }
    
    this->initialized = true ;
    
    if( !this->compute_chain.initialized() )
    {
      this->compute_chain.setMode( nyx::ChainMode::All ) ;
      this->compute_chain.initialize( this->gpu(), nyx::ChainType::Compute ) ;
    }

    if( !this->compute_pipeline.initialized() )
    {
      this->compute_pipeline.initialize( this->gpu(), this->pipeline_bytes, this->pipeline_size  ) ;
    }
    
    Log::output( "NyxImageProcessingModule ", this->name(), " initialized!" ) ;
  }

  void NyxImageProcessingModule::shutdown()
  {
    
  }
  
  void NyxImageProcessingModule::setCallback( std::function<void( nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> callback )
  {
    this->callback = callback ;
  }

  void NyxImageProcessingModule::setFinishSignal( const char* name ) 
  {
    Log::output( "NyxImageProcessingModule ", this->name(), " set finish signal to ", name ) ;
    this->child_bus->publish( &nyx::signal, name ) ;
  }
  
  void NyxImageProcessingModule::dispatch()
  {
    auto function = this->callback ;

    if( this->initialized )
    {
      if( function && this->compute_chain.initialized() )
      {
        function( this->compute_chain, this->compute_pipeline ) ;
      }
    }
  }
  
  void NyxImageProcessingModule::setPipeline( const unsigned char* bytes, unsigned size )
  {
    this->pipeline_bytes = bytes ;
    this->pipeline_size  = size  ;
  }
  
  nyx::Pipeline<Framework>& NyxImageProcessingModule::pipeline()
  {
    return this->compute_pipeline ;
  }
  
  nyx::Chain<Framework>& NyxImageProcessingModule::chain()
  {
    return this->compute_chain ;
  }
  
  void NyxImageProcessingModule::subscribe( iris::Bus& bus )
  {
    this->child_bus = &bus ;
    
    NyxModule::subscribe( bus ) ;
    
    bus.enroll( this, &NyxImageProcessingModule::setFinishSignal, iris::OPTIONAL, this->name(), "::finish" ) ;
  }
}