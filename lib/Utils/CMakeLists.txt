add_mlir_library(RarogUtils
    NaiveAllocation.cpp
    FirstFitAllocation.cpp
    ILPAllocation.cpp
    IdeallocAllocation.cpp

    LINK_LIBS PUBLIC
      MLIRIR
      MLIRPass
      MLIRAffineDialect
      MLIRArithDialect
      MLIRLinalgDialect
      MLIRBufferizationDialect
      MLIRFuncDialect
)

target_link_libraries(RarogUtils PRIVATE
    idealloc-lib::idealloc-lib)

target_include_directories(RarogUtils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/Utils
)
