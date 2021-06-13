SET( EXT_GLM_VER       0.9                            )
SET( EXT_GLM_REL       9.8                            )
SET( EXT_GLM_REL_VER   ${EXT_GLM_VER}.${EXT_GLM_REL}  )

SET( EXT_GLM_TAR_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/glm-${EXT_GLM_REL_VER}.tar.gz ) 
SET( EXT_GLM_BLD_DIR   ${CMAKE_BINARY_DIR}/ext/Glm/                              )
SET( EXT_GLM_SRC_DIR   ${EXT_GLM_BLD_DIR}/glm-0.9.9.8/                           )
SET( EXT_GLM_CMAKE_DIR ${EXT_GLM_SRC_DIR}                                        )
SET( EXT_GLM_INCS      ${EXT_GLM_BLD_DIR}/include/                               )

FILE( MAKE_DIRECTORY ${EXT_GLM_BLD_DIR}/include )

MESSAGE( INFO "Untarring GLM" )
EXECUTE_PROCESS( 
  COMMAND tar -xf ${EXT_GLM_TAR_DIR}
  WORKING_DIRECTORY ${EXT_GLM_BLD_DIR}
  )

FILE( COPY ${EXT_GLM_SRC_DIR}/glm DESTINATION ${EXT_GLM_INCS} )
SET( GLM_INCLUDE_DIRS ${EXT_GLM_INCS} CACHE INTERNAL "GLM Include Dirs" )
