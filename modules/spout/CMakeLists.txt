#--------------------------------------------------------------------
# Inviwo spout Module
ivw_module(Spout)

#--------------------------------------------------------------------
# Add header files
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/spoutprocessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/Spout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutCommon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutCopy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutDirectX.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutGLDXinterop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutGLextensions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutMemoryShare.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutReceiver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSDK.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSender.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSenderMemory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSenderNames.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSharedMemory.h
)
ivw_group("Header Files" ${HEADER_FILES})

#--------------------------------------------------------------------
# Add source files
set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/processors/spoutprocessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutCopy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutDirectX.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutGLDXinterop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutGLextensions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutMemoryShare.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutReceiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSDK.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSenderMemory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSenderNames.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/SpoutSharedMemory.cpp
)
ivw_group("Source Files" ${SOURCE_FILES})

#--------------------------------------------------------------------
# Create module
ivw_create_module(${SOURCE_FILES} ${HEADER_FILES})
