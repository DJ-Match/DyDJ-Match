# Configuration File

## Structure

The configuration file is a list of tokens separated by whitespace. A token is the name of an option, or a parameter for an option. Example:
```
option1 parameter1 parameter2
option2 parameter1
```

## Options

| name                   | parameters            | description |
|:---                    | :---                  | :---        |
| `algo`                 | `<name> <parameter>*` | Add an algorithm to the list of algorithms. See below for details on the parameters. |
| `b`                    | `<unsigned int>`      | The numbers of disjoint matchings to compute. Can be specified multiple times. |
| `sanitycheck`          | none                  | Run the sanity check after every algorithm and batch. |
| `console_log`          | none                  | Enable logging |
| `seed`                 | `<int>`               | The seed for the random number generator |
| `algorithm_order_seed` | `<unsigned int>`      | The seed for randomizing the order of algorithms. `0` disables randomizing the order (default) |
| `count_color_ops`      | none                  | Enable counting the changes in edge colors per delta. This needs to be used before the algorithms to which it should apply.|
| `update_strategy`      | `<name> <parameter>*` | Set the update strategy to be used for dynamic algorithms. This is in effect for any `algo` options used until the next `update_strategy` is defined. See below for details on the parameters. |

Note that for historic reasons, the number of disjoint matchings is denoted by b here, not k.

## Algorithms

Algorithms are given in the form
```
algo algorithm-name parameter1 parameter2 ...
```

The available algorithms and their parameters are
| name                  | parameters                                        | description |
|:---                   | :---                                              | :---        |
| `greedy`              | `do_local_swaps`                                  | The static greedy algorithm |
| `node_centered`       | `aggregate threshold`                             | The static node-centered algorithm |
| `batch_node_centered` | `aggregate threshold`                             | The batch-dynamic node-centered algorithm |
| `k_edge_coloring`     | `common_color max_rotate pp`                      | The static k-Edge-Coloring algorithm |
| `dyn_k_edge_coloring` | `common_color max_rotate pp imp ft mode [select]` | The dynamic/hybrid k-Edge-Coloring algorithm |
| `dyn_greedy`          | `num_retries pp ft imp random`                    | The dynamic greedy algorithm |
| `batch_greedy`        | `do_local_swaps`                                  | The batch-dynamic greedy algorithm |
| `invariant_greedy`    | none                                              | The "post-processing" algorithm |

- `aggregate`: Which aggregation method to use. Values are `0-4`, corresponding to `SUM, MAX, AVG, MEDIAN, B_SUM`.
- `common_color, max_rotate`: Values are `0` to enable or `1` to disable.
- `ft`: filter threshold. A number. Updates with weight ratio in `[1/ft, ft]` are filtered. `ft = 1` is equivalent to no filtering.
- `imp`: use improved post-processing. `+` for improved, `-` for standard
- `mode`: Switch between fully-dynamic (`d`) and static-dynamic-hybrid (`h`) mode. The option `select` must be present if and only if `mode = h`.
- `num_retries`: Recursion depth of the `increaseWeight()` function.
- `pp`: Post-processing. `1` to enable or `0` to disable.
- `random`: randomize arc selection. `1`, `2`, `3` for randomization with 1, 2, 3 randomly selected candidates, any other integer for no randomization.
- `select`: At which batch density to switch from dynamic to static k edge-coloring.
- `do_local_swaps`: Disable/enable swapping. Values are `0` to disable or `1` to enable.
- `threshold`: Threshold for classifying light/heavy edges in node-centered algorithms. Values are decimal numbers.


## Examples
(see also the directory `examples/`)

### Minimal Example
```
b 8
algo k_edge_coloring 1 0 1
```

### All Options Used
```
b 2 b 4 b 8
count_color_ops
algo greedy 1 0
algo greedy_b 1 0
algo node_centered 0 0.2
algo batch_node_centered 0 0.2
algo gpa 1 0 2
algo k_edge_coloring 1 0 1
algo dyn_greedy 3
algo coloring_paths 0.97 1.2 17 3
algo batch_greedy 1 0 1
sanitycheck
console_log
seed 3598756
algorithm_order_seed 123098
```
