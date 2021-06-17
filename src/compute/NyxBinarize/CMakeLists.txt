FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_BINARIZE_HEADERS 
      NyxBinarize.h
   )

SET( NYX_BINARIZE_SOURCES
      NyxBinarize.cpp
   )

SET( NYX_BINARIZE_LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_library
     nyx_vkg
   )

ADD_LIBRARY          ( NyxBinarize SHARED ${NYX_BINARIZE_SOURCES} ${NYX_BINARIZE_HEADERS} )
TARGET_LINK_LIBRARIES( NyxBinarize PUBLIC ${NYX_BINARIZE_LIBRARIES}                       )

BUILD_TEST( TARGET NyxBinarize DEPENDS ${NYX_BINARIZE_LIBRARIES} )

INSTALL( TARGETS NyxBinarize DESTINATION ${LIB_DIR} COMPONENT release )