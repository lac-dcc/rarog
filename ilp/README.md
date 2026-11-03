# Mathematical Formulation

## 1. Sets & Parameters

* **$I$**: Set of all buffers, where each buffer $i \in I$ is characterized by:
* $b_i \in \mathbb{R}_{\ge 0}$: Birth time of buffer $i$.
* $d_i \in \mathbb{R}_{\ge 0}$: Death time of buffer $i$.
* $S_i \in \mathbb{Z}_{> 0}$: Size (memory footprint) of buffer $i$.

* **$\mathcal{O}$**: Set of pairs of buffers that overlap in time and must not occupy intersecting memory regions:

$$\mathcal{O} = \left\{ (i, j) \in I \times I \;\mid\; i < j \land \max(b_i, b_j) < \min(d_i, d_j) \right\}$$

* **$M$**: A sufficiently large scalar (Big-$M$) upper bound on the memory space, defined as:

$$M = \sum_{i \in I} S_i$$

---

## 2. Decision Variables

* **$y_i \in \mathbb{Z}_{\ge 0}$**: The starting memory address allocated to buffer $i \in I$.
* **$x_{i,j} \in \{0, 1\}$**: Binary relative positioning indicator for each overlapping pair $(i, j) \in \mathcal{O}$:
* $x_{i,j} = 1 \implies$ buffer $i$ is positioned below buffer $j$ in memory ($y_i + S_i \le y_j$).
* $x_{i,j} = 0 \implies$ buffer $j$ is positioned below buffer $i$ in memory ($y_j + S_j \le y_i$).

* **$\text{MMU} \in \mathbb{Z}_{\ge 0}$**: Maximum Memory Usage (0-indexed highest memory slot occupied).

---

## 3. Objective Function

Minimize the Maximum Memory Usage (MMU):

$$\min \quad \text{MMU}$$

---

## 4. Constraints

### Spatial Non-Overlapping Constraints

For every pair of buffers $(i, j) \in \mathcal{O}$ that overlap in time:

$$y_i + S_i \le y_j + M \cdot (1 - x_{i,j}) \quad \forall (i, j) \in \mathcal{O}$$

$$y_j + S_j \le y_i + M \cdot x_{i,j} \quad \forall (i, j) \in \mathcal{O}$$

### Maximum Memory Usage (MMU) Bound

The variable $\text{MMU}$ must be greater than or equal to the maximum address index used by any buffer $i \in I$:

$$\text{MMU} \ge y_i + S_i - 1 \quad \forall i \in I$$

### Domain Constraints

$$y_i \ge 0 \quad \forall i \in I$$

$$x_{i,j} \in \{0, 1\} \quad \forall (i, j) \in \mathcal{O}$$

$$\text{MMU} \ge 0$$
