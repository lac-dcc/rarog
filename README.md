<p align="center">
  <img alt="Project Banner" src="./assets/images/BannerRarog.png" width="95%" height="auto"/></br>
</p>

---

## Running models using IREE

### Dependencies

- [IREE's ONNX importer](https://iree.dev/guides/deployment-configurations/cpu/#compile-a-program) (The option [onnx] might not work when running pip on ZSH)
- [IREE compiler](https://iree.dev/guides/deployment-configurations/cpu/#get-the-iree-compiler)
- [IREE runtime](https://iree.dev/guides/deployment-configurations/cpu/#get-the-iree-runtime)

### Running the models

The script [iree.sh](scripts/iree.sh) is responsible to convert a model from ONNX to torch MLIR, and compile and run this converted model using the IREE compiler. The script assumes the IREE compiler is installed inside a Python environment located in a folder called `venv` in this repository. If this is not the case, you must:
- If IREE is installed inside a Python environment, set the variable `PYTHON_VENV_PATH`
- If IREE is built from source out of the `$PATH` environment variable, set the variables `IREE_IMPORT_ONNX`, `IREE_COMPILE` and `IREE_RUN_MODULE`

The script runs the first model by default, but you can change it by setting variable `MODEL_IDX` to some value between the available models before running the script.

---

## Running models using MLIR

### Dependencies

- [torch-mlir](https://github.com/llvm/torch-mlir?tab=readme-ov-file#install-torch-mlir-snapshot)
- [MLIR](https://mlir.llvm.org/getting_started/)

### Running the models

The running pipeline consists of 3 scripts:

- [create_linalg.sh](scripts/create_linalg.sh): converts the ONNX model to torch MLIR, then lower it to linalg dialect.
- [lower.sh](scripts/lower.sh): run the lowering pipeline for nasbench models, which consists in adding a main function and run passes to lower from linalg to llvm dialect.
- [run.sh](scripts/run.sh): execute the lowered model.

The run script assumes the MLIR runner binary is in the `$PATH` environment variable. If this is not the case, define the variable `MLIR_RUNNER` to its respective path. Also, the script assumes that some MLIR libraries are within the `/usr/lib/llvm/lib` directory. Set the variables `MLIR_UTILS` and `MLIR_C_UTILS` to the path to the files `libmlir_runner_utils.so` and `libmlir_c_runner_utils.so`, respectively, if they are in a different path.

---

## Generating memory allocation instances and outputs

### Dependencies

- [torch-mlir](https://github.com/llvm/torch-mlir?tab=readme-ov-file#install-torch-mlir-snapshot)
- [MLIR](https://mlir.llvm.org/getting_started/)

### Generating instances and outputs

The generation of instances and outputs for memory allocation is managed with the following scripts:

- [memory_allocation_instantiatior.sh](scripts/memory_allocation_instantiator.sh): generate instances for the memory allocation problem based on the nasbench models.
- [memory_allocation_instrumentation.sh](scripts/memory_allocation_instrumentation.sh): generate the output for the memory allocation problem by instrumenting malloc and free calls, in order to output the address and size of allocated buffers. There are also instrumented versions for the lower and run scripts.

---

## Generating models from Nasbench

### Dependencies (Python libraries)

- tensorflow
- tqdm
- tf2onnx
- onnx

```
pip install tensorflow tqdm tf2onnx onnx
```

### Generating the models

The Nasbench has over 423k benchmarks but, due to space constraints, only 100 were included in this repository. The script [NASBenchConvert.py](scripts/NASBenchConvert.py) can generate the remaining models, although it will take some time to run (ONNX files are already included on the .gitignore). However, it's necessary to extract the file [nasbench_full.json.tar.gz](nasbench_data/nasbench_full.json.tar.gz) first.