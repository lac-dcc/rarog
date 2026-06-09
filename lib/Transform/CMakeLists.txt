add_mlir_library(RarogTransform
    AddRarogMainFunction.cpp
    HoistAlloc.cpp
    HoistDealloc.cpp
    InstrumentMalloc.cpp
    Pipeline.cpp
    StaticAllocation.cpp

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