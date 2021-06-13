/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NyxConnectedComponents.cpp
 * Author: jhendl
 * 
 * Created on June 4, 2021, 9:56 AM
 */

static constexpr unsigned VERSION = 1 ;

#include "NyxConnectedComponents.h"
#include "cc_local.h"
#include "cc_boundary_analysis.h"
#include "cc_global.h"
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>

namespace nyx
{ 
  NyxConnectedComponents::NyxConnectedComponents()
  {
    auto function = [=] ( nyx::Chain<Framework>& chain, nyx::Pipeline<Framework>& pipeline )
    {
      if( this->input )
      {
        const float wx = ( this->input->width () + 32 - 1 ) >> 5 ;
        const float wy = ( this->input->height() + 32 - 1 ) >> 5 ;
        
        this->mutex.lock() ;
        
        this->pipeline()       .bind( "input_tex" , *this->input      ) ;
        this->pipeline()       .bind( "index_map" , this->d_indices   ) ;
        this->boundary_analysis.bind( "input_tex" , *this->input      ) ;
        this->boundary_analysis.bind( "index_map" , this->d_indices   ) ;
        this->global_uf        .bind( "index_map" , this->d_indices   ) ;
        this->global_uf        .bind( "output_tex", this->indexed_map ) ;
        
        chain.begin() ;
        chain.transition( this->indexed_map, nyx::ImageLayout::General ) ;
        chain.push( pipeline               , this->indexed_map.width() ) ;
        chain.push( this->boundary_analysis, this->indexed_map.width() ) ;
        chain.push( this->global_uf        , this->indexed_map.width() ) ;
        chain.dispatch( pipeline               , wx, wy ) ;
        chain.dispatch( this->boundary_analysis, wx, wy ) ;
        chain.dispatch( this->boundary_analysis, wx, wy ) ;
        chain.dispatch( this->global_uf        , wx, wy ) ;
        
        chain.submit     () ;
        chain.synchronize() ;
        this->bus.emit() ;
        this->mutex.unlock() ;
      }
    };
    
    this->input = nullptr ;
    this->dirty = false   ;
    NyxImageProcessingModule::setPipeline( nyx::bytes::cc_local, sizeof( nyx::bytes::cc_local ) ) ;
    NyxImageProcessingModule::setCallback( function                                             ) ;
  }
  
  NyxConnectedComponents::~NyxConnectedComponents()
  {
    
  }
  
  void NyxConnectedComponents::initialize()
  {
    NyxImageProcessingModule::initialize() ;
    
    this->boundary_analysis.initialize( this->gpu(), nyx::bytes::cc_boundary_analysis, sizeof( nyx::bytes::cc_boundary_analysis ) ) ;
    this->global_uf        .initialize( this->gpu(), nyx::bytes::cc_global           , sizeof( nyx::bytes::cc_global            ) ) ;
  }
  
  void NyxConnectedComponents::subscribe( unsigned id )
  {
    this->bus.setChannel( id ) ;
    NyxImageProcessingModule::subscribe( this->bus ) ;
    
    this->bus.enroll( this, &NyxConnectedComponents::setInputName , iris::OPTIONAL, this->name(), "::input"  ) ;
    this->bus.enroll( this, &NyxConnectedComponents::setOutputName, iris::OPTIONAL, this->name(), "::output" ) ;
  }
  
  void NyxConnectedComponents::shutdown()
  {
    NyxImageProcessingModule::shutdown() ;
    
  }
  
  void NyxConnectedComponents::execute()
  {
    if( this->dirty )
    {
      this->dirty = false ;
      this->dispatch() ;
    }
  }
}

// <editor-fold defaultstate="collapsed" desc="Exported function definitions">
/** Exported function to retrive the name of this module type.
 * @return The name of this object's type.
 */
exported_function const char* name()
{
  return "NyxConnectedComponents" ;
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
  return new nyx::NyxConnectedComponents() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxConnectedComponents* mod ;
  
  mod = dynamic_cast<nyx::NyxConnectedComponents*>( module ) ;
  delete mod ;
}
// </editor-fold>