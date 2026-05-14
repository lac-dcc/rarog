#ifndef RAROG_TEST_ANALYSIS
#define RAROG_TEST_ANALYSIS

#include "mlir/Pass/Pass.h"

namespace rarog {
std::unique_ptr<mlir::Pass> createTestAnalysisPass();
}

#endif