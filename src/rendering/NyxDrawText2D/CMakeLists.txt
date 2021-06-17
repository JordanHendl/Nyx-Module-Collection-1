FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )
FIND_PACKAGE( Mars   REQUIRED )

SET( NYX_DRAW_TEXT2D_HEADERS 
      NyxDrawText2D.h
   )

SET( NYX_DRAW_TEXT2D_SOURCES
      NyxDrawText2D.cpp
   )

SET( NYX_DRAW_TEXT2D_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
     mars
     mars_nyxext
   )

ADD_LIBRARY               ( NyxDrawText2D SHARED ${NYX_DRAW_TEXT2D_SOURCES} ${NYX_DRAW_TEXT2D_HEADERS} )
TARGET_INCLUDE_DIRECTORIES( NyxDrawText2D PRIVATE ${GLM_INCLUDE_DIRS}                                  )
TARGET_LINK_LIBRARIES     ( NyxDrawText2D PUBLIC ${NYX_DRAW_TEXT2D_LIBRARIES}                          )

BUILD_TEST( TARGET NyxDrawText2D DEPENDS ${NYX_DRAW_TEXT2D_LIBRARIES} )

INSTALL( TARGETS NyxDrawText2D DESTINATION ${LIB_DIR} COMPONENT release )