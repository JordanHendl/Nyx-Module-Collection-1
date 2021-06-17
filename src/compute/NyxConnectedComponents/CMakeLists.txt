FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_CC_HEADERS 
      NyxConnectedComponents.h
   )

SET( NYX_CC_SOURCES
      NyxConnectedComponents.cpp
   )

SET( NYX_CC_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
   )

ADD_LIBRARY          ( NyxConnectedComponents SHARED ${NYX_CC_SOURCES} ${NYX_CC_HEADERS} )
TARGET_LINK_LIBRARIES( NyxConnectedComponents PUBLIC ${NYX_CC_LIBRARIES}                 )

BUILD_TEST( TARGET NyxConnectedComponents DEPENDS ${NYX_CC_LIBRARIES} )

INSTALL( TARGETS NyxConnectedComponents DESTINATION ${LIB_DIR} COMPONENT release )