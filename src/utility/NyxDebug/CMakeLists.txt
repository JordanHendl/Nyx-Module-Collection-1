FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_VKGDEBUG_HEADERS 
      Debug.h
   )

SET( NYX_VKGDEBUG_SOURCES
      Debug.cpp
   )

SET( NYX_VKGDEBUG_LIBRARIES
     iris_module
     iris_bus
     nyx_vkg
   )

ADD_LIBRARY               ( NyxDebug SHARED ${NYX_VKGDEBUG_SOURCES} ${NYX_VKGDEBUG_HEADERS} )
TARGET_LINK_LIBRARIES     ( NyxDebug PUBLIC ${NYX_VKGDEBUG_LIBRARIES}                       )

BUILD_TEST( TARGET NyxDebug DEPENDS ${NYX_VKGDEBUG_LIBRARIES} )
INSTALL( TARGETS NyxDebug DESTINATION ${LIB_DIR} COMPONENT release )