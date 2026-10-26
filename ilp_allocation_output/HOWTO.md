
To run the ILP to generate allocations, use the python scripts in the `ilp` folder, like so:
```
((venv) ) user@pc:~/rarog$ python ilp/main.py memory_allocation_input/model_1.in ilp_allocation_output/model_1.out
File memory_allocation_input/model_1.in
Set parameter Username
Set parameter LicenseID to value 2818587
Academic license - for non-commercial use only - expires 2027-05-05
Set parameter TimeLimit to value 3600
Gurobi Optimizer version 13.0.2 build v13.0.2rc1 (linux64 - "Ubuntu 24.04.4 LTS")

CPU model: AMD Ryzen 7 3700X 8-Core Processor, instruction set [SSE2|AVX|AVX2]
Thread count: 8 physical cores, 16 logical processors, using up to 16 threads

Non-default parameters:
TimeLimit  3600

Optimize a model with 1484 rows, 264 columns and 4681 nonzeros (Min)
Model fingerprint: 0xe3452bf6
Model has 19 linear objective coefficients
Variable types: 19 continuous, 245 integer (245 binary)
Coefficient statistics:
  Matrix range     [1e+00, 1e+05]
  Objective range  [1e+00, 1e+00]
  Bounds range     [1e+00, 1e+00]
  RHS range        [1e+00, 1e+00]

Presolve removed 990 rows and 0 columns
Presolve time: 0.02s
Presolved: 494 rows, 264 columns, 2520 nonzeros
Variable types: 0 continuous, 264 integer (245 binary)
Found heuristic solution: objective 541184.00000

Root relaxation: objective 3.871907e+05, 473 iterations, 0.01 seconds (0.01 work units)

    Nodes    |    Current Node    |     Objective Bounds      |     Work
 Expl Unexpl |  Obj  Depth IntInf | Incumbent    BestBd   Gap | It/Node Time

     0     0 387190.714    0   84 541184.000 387190.714  28.5%     -    0s
H    0     0                    532736.00000 387190.714  27.3%     -    0s
     0     0 467200.000    0   55 532736.000 467200.000  12.3%     -    0s
H    0     0                    524288.00000 467200.000  10.9%     -    0s
     0     0 471117.071    0   49 524288.000 471117.071  10.1%     -    0s
     0     0 489751.788    0   62 524288.000 489751.788  6.59%     -    0s
     0     0 489751.788    0   50 524288.000 489751.788  6.59%     -    0s
     0     0 489751.788    0   77 524288.000 489751.788  6.59%     -    0s
     0     0 489751.788    0   77 524288.000 489751.788  6.59%     -    0s
     0     0 505863.051    0   79 524288.000 505863.051  3.51%     -    0s
     0     0 508531.362    0   78 524288.000 508531.362  3.01%     -    0s
     0     0 508531.362    0   80 524288.000 508531.362  3.01%     -    0s
     0     0     cutoff    0      524288.000 524288.000  0.00%     -    0s

Cutting planes:
  Cover: 1
  Implied bound: 6
  MIR: 12
  Zero half: 16
  Mod-K: 1
  RLT: 4
  Relax-and-lift: 12

Explored 1 nodes (1462 simplex iterations) in 0.11 seconds (0.10 work units)
Thread count was 16 (of 16 available processors)

Solution count 3: 524288 532736 541184 

Optimal solution found (tolerance 1.00e-04)
Best objective 5.242880000000e+05, best bound 5.242880000000e+05, gap 0.0000%
Best solution = 524288.0
Best bound = 524288.0
Runtime = 0.10584187507629395
Nodes = 1.0
```
- [Output](./model_1.out)

### TODO
- Standardize the ILP output to be used in the allocation pass
- Integrate the invocation with the MLIR pass to avoid creating intermediate
  files altogether
- Create script to run a batch of tests with the ILP