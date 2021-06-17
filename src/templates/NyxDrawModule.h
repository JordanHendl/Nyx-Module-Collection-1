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


#include <glm/glm.hpp>
#include <unordered_map>
#include <functional>

namespace nyx
{
  using Framework = nyx::vkg::Vulkan ;
  using Log       = iris::log::Log   ;
  
  static void signal() {}
  
  template<typename Drawable>
  class NyxDrawModule : public nyx::NyxModule
  {
    public:

      /** Default Constructor.
       */
      NyxDrawModule() ;

      /** Virtual deconstructor. Needed for inheritance.
       */
      virtual ~NyxDrawModule() ;
      
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
      
      /** Method to recieve the subpass ID of this module.
       * @return The subpass ID of this module.
       */
      unsigned subpass() const ;
      
      /** Method to set the key to use for this object's transformation UBO.
       * @param key The string key to associate with the transformation buffer in the pipeline.
       */
      void setTransformKey( std::string key ) ;
      
      /** Method to set the size of this object's internal transformation buffer.
       * @param sz The size in elements to use for this transformation buffer.
       */
      void setTransformSize( unsigned sz ) ;

      /** Method to draw this object's data. 
       * Calls the specified input callback for every element being drawn.
       * For setting the callback, see @setPerDrawCallback.
       */
      void draw() ;
      
      /** Method to retrieve the amount of drawables in this object's buffer.
       * @return The amount of elements being selected to draw.
       */
      unsigned drawableCount() const ;

      /** Method to draw this object's input with the specified parameters.
       * @param vertices The vertex buffer to use for drawing instanced.
       * @param count The amount of instances to draw.
       */
      template<typename Vertex>
      void drawInstanced( nyx::Array<Framework, Vertex>& vertices, unsigned count ) ;
      
      nyx::Pipeline<Framework>& pipeline() ;
      
      void setPerDrawCallback( std::function<void( unsigned, Drawable&, nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> callback ) ;
      
      void setTransformFlag( nyx::ArrayFlags flag ) ;
      
      void setPostInitCallback( std::function<void()> callback ) ;
      
      bool dirty() ;
      
      void emit() ;
    private:
      using DrawCallback       = std::function<void( unsigned, Drawable&, nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> ;
      using InitializeCallback = std::function<void()> ;

      void addDrawable( unsigned id, const Drawable& drawable ) ;
      void addDrawableTransform( unsigned id, const glm::mat4& transform ) ;
      void removeDrawable( unsigned id ) ;
      void setParentPass( const nyx::RenderPass<Framework>& render_pass ) ;
      void setParentChain( const nyx::Chain<Framework>& parent_chain ) ;
      void setDrawableName( const char* name ) ;
      void setDrawableRemoveName( const char* name ) ;
      void setParentName( const char* name ) ;
      void setReferenceName( const char* name ) ;
      void setSubpass( unsigned id ) ;
      void setFinishSignal( const char* name ) ;
      void setWidth( unsigned value ) ;
      void setHeight( unsigned height ) ;
      
      nyx::ArrayFlags                        array_flag            ;
      unsigned                               width                 ;
      unsigned                               height                ;
      const nyx::Chain<Framework>*           parent_chain          ;
      const nyx::RenderPass<Framework>*      parent_pass           ;
      const unsigned char*                   pipeline_bytes        ;
      unsigned                               pipeline_size         ;
      iris::Bus*                             child_bus             ;
      unsigned                               subpass_id            ;
      nyx::Chain<Framework>                  copy_chain            ;
      nyx::Chain<Framework>                  render_chain          ;
      nyx::Pipeline<Framework>               render_pipeline       ;
      nyx::Array<Framework, glm::mat4>       d_transforms          ;
      bool                                   transforms_dirty      ;
      bool                                   drawables_dirty       ;
      DrawCallback                           per_drawable_function ;
      InitializeCallback                     init_callback         ;
      std::string                            transform_key         ;
      std::vector<glm::mat4>                 transforms            ;
      std::unordered_map<unsigned ,Drawable> drawables             ;
      std::string                            reference_key         ;
      bool                                   initialized           ;
  };
  
  template<typename Drawable>
  NyxDrawModule<Drawable>::NyxDrawModule()
  {
    this->render_chain.setMode( nyx::ChainMode::All ) ;

    this->array_flag            = nyx::ArrayFlags::UniformBuffer ;
    this->width                 = 1280                           ;
    this->height                = 1024                           ;
    this->initialized           = false                          ;
    this->transforms_dirty      = true                           ;
    this->drawables_dirty       = true                           ;
    this->parent_chain          = nullptr                        ;
    this->parent_pass           = nullptr                        ;
    this->child_bus             = nullptr                        ;
    this->pipeline_bytes        = nullptr                        ;
    this->per_drawable_function = nullptr                        ;
    this->pipeline_size         = 0                              ;
    this->subpass_id            = UINT_MAX                       ;
  }
  
  template<typename Drawable>
  NyxDrawModule<Drawable>::~NyxDrawModule()
  {
    if( this->render_chain.initialized() ) this->render_chain.reset() ;
    
    this->parent_chain = nullptr ;
    this->parent_pass  = nullptr ;
    this->child_bus    = nullptr ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::initialize()
  {
    nyx::Viewport viewport ;
    
    Log::output( "NyxDrawModule ", this->name(), " initializing..." ) ;
    
    if( !this->parent_chain || !this->parent_pass || this->transforms.size() == 0 || this->transform_key == ""  || !this->pipeline_bytes || this->pipeline_size == 0  || this->subpass_id == UINT_MAX )
    {
      Log::output( Log::Level::Fatal, " Trying to initialize a Nyx Draw Module without having set the proper parameters first. See file " __FILE__, " at line ", __LINE__, ".\n",
        "Parameters: \n",
        "--Parent Chain   Valid", this->parent_chain == nullptr,    "\n",  
        "--Parent Pass    Valid", this->parent_pass == nullptr,     "\n", 
        "--Pipeline Bytes Valid", this->pipeline_bytes == nullptr,  "\n", 
        "--Pipeline Byte Size  ", this->pipeline_size,              "\n",     
        "--Transform size      ", this->transforms.size(),          "\n", 
        "--Subpass ID          ", this->subpass_id,                 "\n",  
        "--Shader Transform Key", this->transform_key.c_str(),      "\n" ) ;
    }
    
    this->initialized = true ;
    
    viewport.setWidth ( this->width  ) ;
    viewport.setHeight( this->height ) ;
    
    if( !this->d_transforms.initialized() ) this->d_transforms.initialize( this->gpu(), this->transforms.size(), false, this->array_flag ) ;
    if( !this->copy_chain  .initialized() ) this->copy_chain  .initialize( this->gpu(), nyx::ChainType::Compute                          ) ;
    
    if( this->render_chain.initialized() )
    {
      this->render_chain.reset() ;
    }

    if( this->render_pipeline.initialized() )
    {
      this->render_pipeline.reset() ;
    }
    
    this->render_chain.setMode        ( nyx::ChainMode::All                                            ) ;
    this->render_chain.initialize     ( *this->parent_chain, this->subpass_id                          ) ;
    this->render_pipeline.setTestDepth( true                                                           ) ;
    this->render_pipeline.addViewport ( viewport                                                       ) ;
    this->render_pipeline.initialize  ( *this->parent_pass, this->pipeline_bytes, this->pipeline_size  ) ;
    this->render_pipeline.bind        ( this->transform_key.c_str(), this->d_transforms                ) ;
    if( this->init_callback ) this->init_callback() ;
    Log::output( "NyxDrawModule ", this->name(), " initialized!" ) ;
  }

  template<typename Drawable>
  void NyxDrawModule<Drawable>::setPerDrawCallback( std::function<void( unsigned, Drawable&, nyx::Chain<Framework>&, nyx::Pipeline<Framework>& )> callback )
  {
    this->per_drawable_function = callback ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setPostInitCallback( std::function<void()> callback )
  {
    this->init_callback = callback ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setTransformFlag( nyx::ArrayFlags flag )
  {
    this->array_flag = flag ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setParentPass( const nyx::RenderPass<Framework>& render_pass )
  {
    Log::output( "NyxDrawModule ", this->name(), " set parent render pass to ", static_cast<const void*>( &render_pass ) ) ;
    this->parent_pass = &render_pass ;
    this->drawables_dirty = true ;
    if( this->parent_chain && this->subpass_id != UINT_MAX )
    {
      NyxDrawModule::initialize() ;
    }
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setParentChain( const nyx::Chain<Framework>& chain )
  {
    Log::output( "NyxDrawModule ", this->name(), " set parent render chain to ", static_cast<const void*>( &chain ) ) ;
    this->parent_chain = &chain ; 
    this->drawables_dirty = true ;
    
    if( this->parent_pass && this->subpass_id != UINT_MAX )
    {
      NyxDrawModule::initialize() ;
    }
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setParentName( const char* name )
  {
    Log::output( "NyxDrawModule ", this->name(), " set parent key to ", name ) ;
    this->child_bus->enroll( this, &NyxDrawModule::setParentChain, iris::OPTIONAL, name ) ;
    this->child_bus->enroll( this, &NyxDrawModule::setParentPass , iris::OPTIONAL, name ) ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setReferenceName( const char* name )
  {
    Log::output( "NyxDrawModule ", this->name(), " set output reference key to ", name ) ;
    this->reference_key = name ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::shutdown()
  {
    
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::addDrawable( unsigned id, const Drawable& drawable )
  {
    auto iter = this->drawables.find( id ) ;
    if( iter == this->drawables.end() )
    {
      this->drawables.insert( iter, { id, drawable } ) ;
      this->drawables_dirty = true ;
    }
    else
    {
      Log::output( Log::Level::Warning, "Trying to add a drawable to module ", this->name(), " when to ID ", id, " even though one already exists." ) ;
    }
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::addDrawableTransform( unsigned id, const glm::mat4& transform )
  {
    if( id < this->transforms.size() )
    {
      this->transforms[ id ] = transform ;
      this->transforms_dirty = true      ;
    }
    else
    {
      Log::output( Log::Level::Warning, "Trying to add a drawable transform to module ", this->name(), " to ID ", id, " even though one already exists." ) ;
    }
  }
  
  template<typename Drawable>
  bool NyxDrawModule<Drawable>::dirty()
  {
    return this->drawables_dirty ;
  }

  template<typename Drawable>
  void NyxDrawModule<Drawable>::emit()
  {
    this->child_bus->emitIndexed( this->render_chain, this->subpass_id, this->reference_key.c_str() ) ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::removeDrawable( unsigned id )
  {
    auto iter = this->drawables.find( id ) ;
    if( iter != this->drawables.end() )
    {
      this->drawables.erase( iter ) ;
      this->drawables_dirty = true ;
    }
    else
    {
      Log::output( Log::Level::Warning, "Trying to remove a drawable from module ", this->name(), " at ID ", id, " even though one does not exist." ) ;
    }
  }
  
  template<typename Drawable>
  unsigned NyxDrawModule<Drawable>::drawableCount() const
  {
    return this->drawables.size() ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setDrawableName( const char* name )
  {
    this->child_bus->enroll( this, &NyxDrawModule::addDrawable         , iris::OPTIONAL, name ) ;
    this->child_bus->enroll( this, &NyxDrawModule::addDrawableTransform, iris::OPTIONAL, name ) ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setDrawableRemoveName( const char* name )
  {
    this->child_bus->enroll( this, &NyxDrawModule::removeDrawable, iris::OPTIONAL, name ) ;
  }

  template<typename Drawable>
  void NyxDrawModule<Drawable>::setSubpass( unsigned id ) 
  {
    Log::output( "NyxDrawModule ", this->name(), " set subpass to ", id ) ;
    this->subpass_id = id ;
    
    if( this->parent_chain && this->parent_pass )
    {
      this->initialize() ;
    }
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setFinishSignal( const char* name ) 
  {
    Log::output( "NyxDrawModule ", this->name(), " set finish signal to ", name ) ;
    this->child_bus->publish( &nyx::signal, name ) ;
  }

  template<typename Drawable>
  void NyxDrawModule<Drawable>::setWidth( unsigned value )
  {
    this->width = value ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setHeight( unsigned value )
  {
    this->height = value ;
  }
  
  template<typename Drawable>
  template<typename Vertex>
  void NyxDrawModule<Drawable>::drawInstanced( nyx::Array<Framework, Vertex>& vertices, unsigned count )
  {
    this->render_chain.drawInstanced( count, this->render_pipeline, vertices ) ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::draw()
  {
    auto function = this->per_drawable_function ;
    
    if( this->initialized )
    {
      if( this->transforms_dirty && this->copy_chain.initialized() )
      {
        this->copy_chain.copy( this->transforms.data(), this->d_transforms ) ;
        this->copy_chain.submit     () ;
        this->copy_chain.synchronize() ;
        this->transforms_dirty = false ;
      }
      
      if( function && this->drawables_dirty && this->render_chain.initialized() )
      {
        this->render_chain.begin() ;
        
        for( auto& drawable : this->drawables )
        {
          function( drawable.first, drawable.second, this->render_chain, this->render_pipeline ) ;
        }
        
        this->render_chain.end() ;
//        Log::output( "NyxDrawModule ", this->name(), " signalling ", this->reference_key.c_str() ) ;
        this->emit() ;
      }
      else
      {
        this->render_chain.advance() ;
      }
      this->drawables_dirty = false ;
    }
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setPipeline( const unsigned char* bytes, unsigned size )
  {
    this->pipeline_bytes = bytes ;
    this->pipeline_size  = size  ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setTransformKey( std::string key )
  {
    this->transform_key = key ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::setTransformSize( unsigned sz )
  {
    this->transforms.resize( sz ) ;
  }
  
  template<typename Drawable>
  nyx::Pipeline<Framework>& NyxDrawModule<Drawable>::pipeline()
  {
    return this->render_pipeline ;
  }
  
  template<typename Drawable>
  nyx::Chain<Framework>& NyxDrawModule<Drawable>::chain()
  {
    return this->render_chain ;
  }
  
  template<typename Drawable>
  void NyxDrawModule<Drawable>::subscribe( iris::Bus& bus )
  {
    this->child_bus = &bus ;
    
    NyxModule::subscribe( bus ) ;
    
    bus.enroll( this, &NyxDrawModule::setParentName        , iris::OPTIONAL, this->name(), "::parent"    ) ;
    bus.enroll( this, &NyxDrawModule::setReferenceName     , iris::OPTIONAL, this->name(), "::reference" ) ;
    bus.enroll( this, &NyxDrawModule::setDrawableName      , iris::OPTIONAL, this->name(), "::drawable"  ) ;
    bus.enroll( this, &NyxDrawModule::setDrawableRemoveName, iris::OPTIONAL, this->name(), "::remove"    ) ;
    bus.enroll( this, &NyxDrawModule::setSubpass           , iris::OPTIONAL, this->name(), "::subpass"   ) ;
    bus.enroll( this, &NyxDrawModule::setFinishSignal      , iris::OPTIONAL, this->name(), "::finish"    ) ;
  }
}