FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_WINDOW_MODULE_HEADERS 
      Window.h
   )

SET( NYX_WINDOW_MODULE_SOURCES
      Window.cpp
   )

SET( NYX_WINDOW_MODULE_LIBRARIES
     iris_module
     iris_bus
     nyx_vkg
     nyx_library
     nyx_window
   )

ADD_LIBRARY               ( NyxWindow SHARED ${NYX_WINDOW_MODULE_SOURCES} ${NYX_WINDOW_MODULE_HEADERS} )
TARGET_LINK_LIBRARIES     ( NyxWindow PUBLIC ${NYX_WINDOW_MODULE_LIBRARIES}                            )
#TARGET_INCLUDE_DIRECTORIES( NyxWindow PRIVATE ${Vulkan_INCLUDE_DIRS}                                   )
BUILD_TEST( TARGET NyxWindow DEPENDS ${NYX_WINDOW_MODULE_LIBRARIES} ) 

INSTALL( TARGETS NyxWindow DESTINATION ${LIB_DIR} )