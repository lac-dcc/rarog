add_mlir_library(RarogUtils
    NaiveAllocation.cpp
    FirstFitAllocation.cpp
    ILPAllocation.cpp

    LINK_LIBS PUBLIC
      MLIRIR
      MLIRPass
      MLIRAffineDialect
      MLIRArithDialect
      MLIRLinalgDialect
      MLIRBufferizationDialect
      MLIRFuncDialect
)

target_include_directories(RarogUtils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/Utils
)
