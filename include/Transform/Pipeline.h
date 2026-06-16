#ifndef RAROG_INCLUDE_TRANSFORM_PIPELINE_H
#define RAROG_INCLUDE_TRANSFORM_PIPELINE_H

namespace rarog {

void registerRarogBufferizationPipeline();
void registerRarogLoweringPipeline();
void registerInstrumentMallocPipeline();
void registerStaticAllocationPipeline();

} // namespace rarog

#endif // RAROG_INCLUDE_TRANSFORM_PIPELINE_H