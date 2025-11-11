# Comprehensive vendor library validation system
# This ensures editline and secp256k1 libraries are always properly built
# before any dependent targets attempt to link against them

# Function to comprehensively validate a static library
function(validate_static_library LIB_NAME LIB_PATH SOURCE_DIR MAKEFILE_PATH)
    set(VALIDATION_FAILED FALSE)
    set(FAILURE_REASONS "")
    
    # Check 1: Library file exists
    if(NOT EXISTS "${LIB_PATH}")
        set(VALIDATION_FAILED TRUE)
        list(APPEND FAILURE_REASONS "${LIB_NAME}: Static library file '${LIB_PATH}' does not exist")
    else()
        # Check 2: Library file is not empty (minimum reasonable size)
        file(SIZE "${LIB_PATH}" LIB_SIZE)
        if(LIB_SIZE LESS 10240)  # Less than - indicates incomplete build
            set(VALIDATION_FAILED TRUE)
            list(APPEND FAILURE_REASONS "${LIB_NAME}: Static library '${LIB_PATH}' is too small (${LIB_SIZE} bytes), indicating incomplete build")
        endif()
        
        # Check 3: Verify it's actually a static library using file command
        find_program(FILE_COMMAND file)
        if(FILE_COMMAND)
            execute_process(
                COMMAND ${FILE_COMMAND} "${LIB_PATH}"
                OUTPUT_VARIABLE FILE_OUTPUT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(NOT FILE_OUTPUT MATCHES "(ar archive|current ar archive)")
                set(VALIDATION_FAILED TRUE)
                list(APPEND FAILURE_REASONS "${LIB_NAME}: File '${LIB_PATH}' is not a valid static library archive (file output: ${FILE_OUTPUT})")
            endif()
        endif()
    endif()
    
    # Check 4: Verify Makefile state consistency  
    if(EXISTS "${MAKEFILE_PATH}")
        # If Makefile exists but library doesn't, or Makefile is newer than library, rebuild is needed
        if(NOT EXISTS "${LIB_PATH}")
            set(VALIDATION_FAILED TRUE)
            list(APPEND FAILURE_REASONS "${LIB_NAME}: Makefile exists but library is missing - stale build state detected")
        else()
            # Check if Makefile is significantly newer than library (indicating potential stale state)
            file(TIMESTAMP "${MAKEFILE_PATH}" MAKEFILE_TIME)
            file(TIMESTAMP "${LIB_PATH}" LIBRARY_TIME)
            if(MAKEFILE_TIME GREATER LIBRARY_TIME)
                # Additional check: see if any source files are newer than the library
                file(GLOB_RECURSE SOURCE_FILES "${SOURCE_DIR}/*.c" "${SOURCE_DIR}/*.h" "${SOURCE_DIR}/*.am" "${SOURCE_DIR}/*.in")
                set(SOURCE_NEWER_FOUND FALSE)
                foreach(SOURCE_FILE ${SOURCE_FILES})
                    file(TIMESTAMP "${SOURCE_FILE}" SOURCE_TIME)
                    if(SOURCE_TIME GREATER LIBRARY_TIME)
                        set(SOURCE_NEWER_FOUND TRUE)
                        break()
                    endif()
                endforeach()
                
                if(SOURCE_NEWER_FOUND)
                    set(VALIDATION_FAILED TRUE)
                    list(APPEND FAILURE_REASONS "${LIB_NAME}: Source files are newer than library - rebuild required")
                endif()
            endif()
        endif()
    endif()
    
    # Check 5: Verify expected symbols exist in the library (for editline specifically)
    if(LIB_NAME STREQUAL "editline" AND NOT VALIDATION_FAILED)
        find_program(NM_COMMAND nm)
        if(NM_COMMAND AND EXISTS "${LIB_PATH}")
            execute_process(
                COMMAND ${NM_COMMAND} "${LIB_PATH}"
                OUTPUT_VARIABLE NM_OUTPUT
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            # Check for key editline symbols
            if(NOT NM_OUTPUT MATCHES "readline" OR NOT NM_OUTPUT MATCHES "rl_")
                set(VALIDATION_FAILED TRUE)
                list(APPEND FAILURE_REASONS "${LIB_NAME}: Library doesn't contain expected editline symbols")
            endif()
        endif()
    endif()
    
    # Return validation results
    set(${LIB_NAME}_VALIDATION_FAILED ${VALIDATION_FAILED} PARENT_SCOPE)
    set(${LIB_NAME}_FAILURE_REASONS "${FAILURE_REASONS}" PARENT_SCOPE)
endfunction()

# Function to force clean rebuild of a library
function(force_clean_rebuild LIB_NAME SOURCE_DIR)
    message(STATUS "🔧 ${LIB_NAME}: Forcing clean rebuild due to validation failures")
    
    # Remove all build artifacts
    file(REMOVE_RECURSE "${SOURCE_DIR}/.libs")
    file(REMOVE_RECURSE "${SOURCE_DIR}/src/.libs")
    
    # Remove object files and libtool files
    file(GLOB_RECURSE OBJECTS "${SOURCE_DIR}/*.o" "${SOURCE_DIR}/*.lo" "${SOURCE_DIR}/*.la" "${SOURCE_DIR}/*.lai")
    foreach(OBJ ${OBJECTS})
        file(REMOVE "${OBJ}")
    endforeach()
    
    # Run make clean if Makefile exists
    if(EXISTS "${SOURCE_DIR}/Makefile")
        message(STATUS "🧹 ${LIB_NAME}: Running make clean")
        execute_process(
            COMMAND make clean
            WORKING_DIRECTORY "${SOURCE_DIR}"
            OUTPUT_QUIET
            ERROR_QUIET
        )
    endif()
    
    message(STATUS "✅ ${LIB_NAME}: Clean completed, library will be rebuilt")
endfunction()

# Main validation function for all vendor libraries
function(validate_and_ensure_vendor_libraries)
    message(STATUS "🔍 Validating vendor libraries...")
    
    set(EDITLINE_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline/src/.libs/libeditline.a")
    set(SECP256K1_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp/.libs/libsecp256k1.a")
    set(EDITLINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline")
    set(SECP256K1_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp")
    
    # Validate editline
    validate_static_library("editline" 
        "${EDITLINE_LIB_PATH}" 
        "${EDITLINE_SOURCE_DIR}"
        "${EDITLINE_SOURCE_DIR}/Makefile")
    
    # Validate secp256k1    
    validate_static_library("secp256k1" 
        "${SECP256K1_LIB_PATH}" 
        "${SECP256K1_SOURCE_DIR}"
        "${SECP256K1_SOURCE_DIR}/Makefile")
    
    # Handle editline validation failures
    if(editline_VALIDATION_FAILED)
        message(STATUS "❌ editline validation failed:")
        foreach(REASON ${editline_FAILURE_REASONS})
            message(STATUS "   • ${REASON}")
        endforeach()
        force_clean_rebuild("editline" "${EDITLINE_SOURCE_DIR}")
    else()
        message(STATUS "✅ editline validation passed")
    endif()
    
    # Handle secp256k1 validation failures  
    if(secp256k1_VALIDATION_FAILED)
        message(STATUS "❌ secp256k1 validation failed:")
        foreach(REASON ${secp256k1_FAILURE_REASONS})
            message(STATUS "   • ${REASON}")
        endforeach()
        force_clean_rebuild("secp256k1" "${SECP256K1_SOURCE_DIR}")
    else()
        message(STATUS "✅ secp256k1 validation passed")
    endif()
    
    # Set global flags to indicate validation results
    set(VENDOR_LIBS_VALIDATED TRUE PARENT_SCOPE)
    set(EDITLINE_NEEDS_REBUILD ${editline_VALIDATION_FAILED} PARENT_SCOPE) 
    set(SECP256K1_NEEDS_REBUILD ${secp256k1_VALIDATION_FAILED} PARENT_SCOPE)
    
    if(editline_VALIDATION_FAILED OR secp256k1_VALIDATION_FAILED)
        message(STATUS "🔄 Some vendor libraries need rebuilding")
    else()
        message(STATUS "✅✅✅  All vendor libraries validated successfully") 
    endif()
endfunction()