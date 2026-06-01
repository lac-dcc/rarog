#ifndef RAROG_INCLUDE_TRANSFORM_PIPELINE_H
#define RAROG_INCLUDE_TRANSFORM_PIPELINE_H

namespace rarog {

void registerRarogLoweringPipeline();
void registerInstrumentMallocPipeline();
void registerHoistDeallocPipeline();
void registerStaticAllocationPipeline();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_PIPELINE_H