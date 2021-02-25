FIND_PACKAGE( Iris  REQUIRED )
FIND_PACKAGE( Nyx   REQUIRED )

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

ADD_LIBRARY          ( VulkanImageConverter SHARED ${NYX_IMAGE_CONVERTER__SOURCES} ${NYX_IMAGE_CONVERTER__HEADERS} )
TARGET_LINK_LIBRARIES( VulkanImageConverter PUBLIC ${NYX_IMAGE_CONVERTER__LIBRARIES}                               )

BUILD_TEST( TARGET VulkanImageConverter
            DEPENDS ${NYX_IMAGE_CONVERTER_LIBRARIES} )

INSTALL( TARGETS VulkanImageConverter DESTINATION ${LIB_DIR} )