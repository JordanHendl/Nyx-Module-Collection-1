FIND_PACKAGE( NyxGPU REQUIRED )
FIND_PACKAGE( Iris   REQUIRED )

SET( NYX_IMAGE_CONVERTER__HEADERS 
      ImageConverter.h
   )

SET( NYX_IMAGE_CONVERTER__SOURCES
      ImageConverter.cpp
   )

SET( NYX_IMAGE_CONVERTER__LIBRARIES
     iris_module
     iris_bus
     iris_profiling
     nyx_vkg
   )

ADD_LIBRARY          ( NyxImageConverter SHARED ${NYX_IMAGE_CONVERTER__SOURCES} ${NYX_IMAGE_CONVERTER__HEADERS} )
TARGET_LINK_LIBRARIES( NyxImageConverter PUBLIC ${NYX_IMAGE_CONVERTER__LIBRARIES}                               )

BUILD_TEST( TARGET NyxImageConverter
            DEPENDS ${NYX_IMAGE_CONVERTER_LIBRARIES} )

INSTALL( TARGETS NyxImageConverter DESTINATION ${LIB_DIR} )