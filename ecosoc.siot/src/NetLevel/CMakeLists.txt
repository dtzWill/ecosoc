add_llvm_loadable_module( LLVMNetLevel
  NetLevel.cpp GraphStructure.cpp FunctionInfo.cpp
  )



FIND_PACKAGE( Boost 1.40 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

TARGET_LINK_LIBRARIES( LLVMNetLevel ${Boost_LIBRARIES} )
