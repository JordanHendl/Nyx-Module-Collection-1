FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )

  SET( NYX_DRAW_TEX2D_HEADERS 
        NyxDrawTex2D.h
     )
  
  SET( NYX_DRAW_TEX2D_SOURCES
        NyxDrawTex2D.cpp
     )
  
  SET( NYX_DRAW_TEX2D_LIBRARIES
       iris_module
       iris_bus
       iris_profiling
       nyx_library
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY               ( NyxDrawTex2D SHARED ${NYX_DRAW_TEX2D_SOURCES} ${NYX_DRAW_TEX2D_HEADERS} )
  TARGET_INCLUDE_DIRECTORIES( NyxDrawTex2D PRIVATE ${GLM_INCLUDE_DIRS}                                )
  TARGET_LINK_LIBRARIES     ( NyxDrawTex2D PUBLIC ${NYX_DRAW_TEX2D_LIBRARIES}                         )
  
  BUILD_TEST( TARGET NyxDrawTex2D
              DEPENDS ${NYX_DRAW_TEX2D_LIBRARIES} )
  
  INSTALL( TARGETS NyxDrawTex2D DESTINATION ${LIB_DIR} COMPONENT release )
ENDIF()