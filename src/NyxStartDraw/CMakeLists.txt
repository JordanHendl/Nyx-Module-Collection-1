FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_START_DRAW_HEADERS 
      NyxStartDraw.h
   )

SET( NYX_START_DRAW_SOURCES
      NyxStartDraw.cpp
   )

SET( NYX_START_DRAW_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
   )

ADD_LIBRARY          ( NyxStartDraw SHARED ${NYX_START_DRAW_SOURCES} ${NYX_START_DRAW_HEADERS} )
TARGET_LINK_LIBRARIES( NyxStartDraw PUBLIC ${NYX_START_DRAW_LIBRARIES}                         )

BUILD_TEST( TARGET NyxStartDraw
            DEPENDS ${NYX_START_DRAW_LIBRARIES} )

INSTALL( TARGETS NyxStartDraw DESTINATION ${LIB_DIR} )