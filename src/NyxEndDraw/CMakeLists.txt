FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_END_DRAW_HEADERS 
      NyxEndDraw.h
   )

SET( NYX_END_DRAW_SOURCES
      NyxEndDraw.cpp
   )

SET( NYX_END_DRAW_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
   )

ADD_LIBRARY          ( NyxEndDraw SHARED ${NYX_END_DRAW_SOURCES} ${NYX_END_DRAW_HEADERS} )
TARGET_LINK_LIBRARIES( NyxEndDraw PUBLIC ${NYX_END_DRAW_LIBRARIES}                       )

BUILD_TEST( TARGET NyxEndDraw
            DEPENDS ${NYX_END_DRAW_LIBRARIES} )

INSTALL( TARGETS NyxEndDraw DESTINATION ${LIB_DIR} )