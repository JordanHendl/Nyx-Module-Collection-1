FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )

  SET( NYX_DRAW_MODEL__HEADERS 
        NyxDrawModel.h
     )
  
  SET( NYX_DRAW_MODEL__SOURCES
        NyxDrawModel.cpp
     )
  
  SET( NYX_DRAW_MODEL__LIBRARIES
       iris_module
       iris_bus
       iris_profiling
       nyx_library
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY          ( NyxDrawModel SHARED ${NYX_DRAW_MODEL__SOURCES} ${NYX_DRAW_MODEL__HEADERS} )
  TARGET_LINK_LIBRARIES( NyxDrawModel PUBLIC ${NYX_DRAW_MODEL__LIBRARIES}                          )
  
  BUILD_TEST( TARGET NyxDrawModel
              DEPENDS ${NYX_DRAW_MODEL_LIBRARIES} )
  
  INSTALL( TARGETS NyxDrawModel DESTINATION ${LIB_DIR} )
ENDIF()