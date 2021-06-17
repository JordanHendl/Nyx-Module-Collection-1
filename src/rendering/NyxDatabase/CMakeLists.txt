FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )
  SET( NYX_DATABASE_HEADERS 
        NyxDatabase.h
     )
  
  SET( NYX_DATABASE_SOURCES
        NyxDatabase.cpp
     )
  
  SET( NYX_DATABASE_LIBRARIES
       iris_module
       iris_bus
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY               ( NyxDatabase SHARED ${NYX_DATABASE_SOURCES} ${NYX_DATABASE_HEADERS} )
  TARGET_LINK_LIBRARIES     ( NyxDatabase PUBLIC ${NYX_DATABASE_LIBRARIES}                     )
  
  BUILD_TEST( TARGET NyxDatabase DEPENDS ${NYX_DATABASE_LIBRARIES} )
  INSTALL   ( TARGETS NyxDatabase DESTINATION ${LIB_DIR} COMPONENT release )
ENDIF()