FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )

  SET( NYX_DRAW_SPRITE_HEADERS 
        NyxDrawSprite.h
     )
  
  SET( NYX_DRAW_SPRITE_SOURCES
        NyxDrawSprite.cpp
     )
  
  SET( NYX_DRAW_SPRITE_LIBRARIES
       iris_module
       iris_bus
       iris_profiling
       nyx_library
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY               ( NyxDrawSprite SHARED ${NYX_DRAW_SPRITE_SOURCES} ${NYX_DRAW_SPRITE_HEADERS} )
  TARGET_INCLUDE_DIRECTORIES( NyxDrawSprite PRIVATE ${GLM_INCLUDE_DIRS}                                  )
  TARGET_LINK_LIBRARIES     ( NyxDrawSprite PUBLIC ${NYX_DRAW_SPRITE_LIBRARIES}                          )
  
  BUILD_TEST( TARGET NyxDrawSprite DEPENDS ${NYX_DRAW_SPRITE_LIBRARIES} )
  
  INSTALL( TARGETS NyxDrawSprite DESTINATION ${LIB_DIR} COMPONENT release )
ENDIF()