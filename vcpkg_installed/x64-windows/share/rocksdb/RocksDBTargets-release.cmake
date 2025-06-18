#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "RocksDB::rocksdb" for configuration "Release"
set_property(TARGET RocksDB::rocksdb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(RocksDB::rocksdb PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/rocksdb.lib"
  )

list(APPEND _cmake_import_check_targets RocksDB::rocksdb )
list(APPEND _cmake_import_check_files_for_RocksDB::rocksdb "${_IMPORT_PREFIX}/lib/rocksdb.lib" )

# Import target "RocksDB::rocksdb-shared" for configuration "Release"
set_property(TARGET RocksDB::rocksdb-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(RocksDB::rocksdb-shared PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/rocksdb-shared.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/rocksdb-shared.dll"
  )

list(APPEND _cmake_import_check_targets RocksDB::rocksdb-shared )
list(APPEND _cmake_import_check_files_for_RocksDB::rocksdb-shared "${_IMPORT_PREFIX}/lib/rocksdb-shared.lib" "${_IMPORT_PREFIX}/bin/rocksdb-shared.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
