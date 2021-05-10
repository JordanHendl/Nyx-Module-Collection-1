FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars            )

IF( ${Mars_FOUND} )
  SET( NYX_DUMMY_PLAYER_HEADERS 
        NyxDummyPlayer.h
     )
  
  SET( NYX_DUMMY_PLAYER_SOURCES
        NyxDummyPlayer.cpp
     )
  
  SET( NYX_DUMMY_PLAYER_LIBRARIES
       iris_module
       iris_bus
       nyx_vkg
       mars
       mars_nyxext
     )
  
  ADD_LIBRARY          ( NyxDummyPlayer SHARED ${NYX_DUMMY_PLAYER_SOURCES} ${NYX_DUMMY_PLAYER_HEADERS} )
  TARGET_LINK_LIBRARIES( NyxDummyPlayer PUBLIC ${NYX_DUMMY_PLAYER_LIBRARIES}                           )
  
  BUILD_TEST( TARGET NyxDummyPlayer DEPENDS ${NYX_DUMMY_PLAYER_LIBRARIES} )
  INSTALL   ( TARGETS NyxDummyPlayer DESTINATION ${LIB_DIR} )
ENDIF()