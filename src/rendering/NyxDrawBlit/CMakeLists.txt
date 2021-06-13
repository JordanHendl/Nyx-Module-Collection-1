FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_DRAW_BLIT_HEADERS 
      NyxDrawBlit.h
   )

SET( NYX_DRAW_BLIT_SOURCES
      NyxDrawBlit.cpp
   )

SET( NYX_DRAW_BLIT_INCLUDE_DIRECTORIES
    ${GLM_INCLUDE_DIRS}
   )

SET( NYX_DRAW_BLIT_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
   )

ADD_LIBRARY               ( NyxDrawBlit SHARED  ${NYX_DRAW_BLIT_SOURCES} ${NYX_DRAW_BLIT_HEADERS} )
TARGET_INCLUDE_DIRECTORIES( NyxDrawBlit PRIVATE ${GLM_INCLUDE_DIRS}                               )
TARGET_LINK_LIBRARIES     ( NyxDrawBlit PUBLIC  ${NYX_DRAW_BLIT_LIBRARIES}                        )

BUILD_TEST( TARGET NyxDrawBlit
            DEPENDS ${NYX_DRAW_BLIT_LIBRARIES} )

INSTALL( TARGETS NyxDrawBlit DESTINATION ${LIB_DIR} COMPONENT release )