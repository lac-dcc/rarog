add_mlir_library(RarogTransform
    Pipeline.cpp
    AddNasbenchMainFunction.cpp
    InstrumentMalloc.cpp
    ReorderFrees.cpp
    StaticAllocation.cpp
    ReorderPOC.cpp

    LINK_LIBS PUBLIC
      MLIRIR
      MLIRPass
      MLIRAffineDialect
      MLIRArithDialect
      MLIRLinalgDialect
      MLIRBufferizationDialect
      MLIRFuncDialect
)

target_include_directories(RarogTransform
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/Transform
    ${PROJECT_SOURCE_DIR}/include/Utils
)