#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "RocksDB::rocksdb" for configuration "Debug"
set_property(TARGET RocksDB::rocksdb APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(RocksDB::rocksdb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/rocksdbd.lib"
  )

list(APPEND _cmake_import_check_targets RocksDB::rocksdb )
list(APPEND _cmake_import_check_files_for_RocksDB::rocksdb "${_IMPORT_PREFIX}/debug/lib/rocksdbd.lib" )

# Import target "RocksDB::rocksdb-shared" for configuration "Debug"
set_property(TARGET RocksDB::rocksdb-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(RocksDB::rocksdb-shared PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/rocksdb-sharedd.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/rocksdb-sharedd.dll"
  )

list(APPEND _cmake_import_check_targets RocksDB::rocksdb-shared )
list(APPEND _cmake_import_check_files_for_RocksDB::rocksdb-shared "${_IMPORT_PREFIX}/debug/lib/rocksdb-sharedd.lib" "${_IMPORT_PREFIX}/debug/bin/rocksdb-sharedd.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
