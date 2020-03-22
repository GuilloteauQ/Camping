# Camping

``Camping`` is a MPI application that allows you to distribute the execution of large sets of parametric independent tasks (also called bag-of-tasks). 

# Usage

## Context

Imagine you have to make time measurements of an application with different parameters:

```bash
# Example of a simple time measurement
$ ./application param1 param2 param3
0.043
```

And you need to do thousands of **independent** measurements with different parameters (i.e. a campaign).

``Camping`` will distribute all the measurements on differents machines for you to accelerate the campaign.

## Campaign file

You can gather all the commands that you intent to run in a file for ``Camping`` to distribute them:

```json
{
  "exec_file": "sh example/sleep.sh",
  "prologue": [
    "mkdir -p /tmp/workdir",
    "cd /tmp/workdir",
    "touch prologue_works"
  ],
  "epilogue": [
    "cd /tmp/workdir",
    "touch epilogue_works"
  ],
  "params": [
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10"
  ]
}
```

* ``exec_file``: Path for the executable (Mandatory)

* ``prologue``: Code executed by all the nodes before the start of the campaign (Optional)

* ``epilogue``: Code executed by all the nodes after the end of the campaign (Optional)

* ``params``: List of string representing the parameters to plug into the executable (Mandatory)


## Full command

```bash
mpirun -np NB_PROCESSES -H HOSTFILE camping my_campaign.json
```

# Installation

## Dependencies

* ``MPI`` (development was done with ``openmpi``)

### Installation

 ```bash
make
```
