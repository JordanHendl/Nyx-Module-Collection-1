FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )

  SET( NYX_DRAW_MODEL_HEADERS 
        NyxDrawModel.h
     )
  
  SET( NYX_DRAW_MODEL_SOURCES
        NyxDrawModel.cpp
     )
  
  SET( NYX_DRAW_MODEL_INCLUDE_DIRS
      ${GLM_INCLUDE_DIRS}
     )
  SET( NYX_DRAW_MODEL_LIBRARIES
       iris_module
       iris_bus
       iris_profiling
       nyx_library
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY               ( NyxDrawModel SHARED ${NYX_DRAW_MODEL_SOURCES} ${NYX_DRAW_MODEL_HEADERS} )
  TARGET_INCLUDE_DIRECTORIES( NyxDrawModel PRIVATE ${GLM_INCLUDE_DIRS}                                )
  TARGET_LINK_LIBRARIES     ( NyxDrawModel PUBLIC ${NYX_DRAW_MODEL_LIBRARIES}                         )
  
  BUILD_TEST( TARGET NyxDrawModel
              DEPENDS ${NYX_DRAW_MODEL_LIBRARIES} )
  
  INSTALL( TARGETS NyxDrawModel DESTINATION ${LIB_DIR} COMPONENT release )
ENDIF()