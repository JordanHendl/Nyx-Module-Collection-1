/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   NyxBinarize.cpp
 * Author: jhendl
 * 
 * Created on June 4, 2021, 9:56 AM
 */

static constexpr unsigned VERSION = 1 ;

#include "NyxBinarize.h"
#include "binarize.h"
#include <NyxGPU/library/Array.h>
#include <NyxGPU/library/Image.h>
#include <NyxGPU/library/Chain.h>
#include <NyxGPU/vkg/Vulkan.h>

namespace nyx
{ 
  NyxBinarize::NyxBinarize()
  {
    auto function = [=] ( nyx::Chain<Framework>& chain, nyx::Pipeline<Framework>& pipeline )
    {
      if( this->data.input )
      {
        const float wx = this->data.input->width () / 32 ;
        const float wy = this->data.input->height() / 32 ;
        
        this->data.mutex.lock() ;
        
        chain.transition( this->data.binarized_map, nyx::ImageLayout::General ) ;
        chain.submit() ;
        chain.synchronize() ;
        
        this->pipeline().bind( "input_tex" , *this->data.input        ) ;
        this->pipeline().bind( "output_tex", this->data.binarized_map ) ;
        
        chain.begin() ;
        chain.push( pipeline, this->data.threshold ) ;
        chain.dispatch( pipeline, wx, wy ) ;
        
        chain.submit     () ;
        chain.synchronize() ;
        this->data.bus.emit() ;
        this->data.mutex.unlock() ;
      }
    };
    
    NyxImageProcessingModule::setPipeline( nyx::bytes::binarize, sizeof( nyx::bytes::binarize ) ) ;
    NyxImageProcessingModule::setCallback( function                                             ) ;
  }
  
  NyxBinarize::~NyxBinarize()
  {
    
  }
  
  void NyxBinarize::initialize()
  {
    NyxImageProcessingModule::initialize() ;
  }
  
  void NyxBinarize::subscribe( unsigned id )
  {
    this->data.bus.setChannel( id ) ;
    NyxImageProcessingModule::subscribe( this->data.bus ) ;
    
    this->data.bus.enroll( &this->data, &NyxBinarizeData::setInputName , iris::OPTIONAL, this->name(), "::input"     ) ;
    this->data.bus.enroll( &this->data, &NyxBinarizeData::setOutputName, iris::OPTIONAL, this->name(), "::output"    ) ;
    this->data.bus.enroll( &this->data, &NyxBinarizeData::setThreshold , iris::OPTIONAL, this->name(), "::threshold" ) ;
  }
  
  void NyxBinarize::shutdown()
  {
    NyxImageProcessingModule::shutdown() ;
    
  }
  
  void NyxBinarize::execute()
  {
    if( this->data.dirty )
    {
      this->data.dirty = false ;
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
  return "NyxBinarize" ;
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
  return new nyx::NyxBinarize() ;
}

/** Exported function to destroy an instance of this module.
 * @param module A Pointer to a Module object that is of this type.
 */
exported_function void destroy( ::iris::Module* module )
{
  ::nyx::NyxBinarize* mod ;
  
  mod = dynamic_cast<nyx::NyxBinarize*>( module ) ;
  delete mod ;
}
// </editor-fold>