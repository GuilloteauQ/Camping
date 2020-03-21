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

```Camping`` will distribute all the measurements on differents machines for you to accelerate the campaign.

## Campaign file

You can gather all the commands that you intent to run in a file for ``Camping`` to distribute them:

```bash
# Content of my_campaign.txt
./application param1 param2 param3
./application param4 param5 param6
./application param7 param8 param9
# ...
```

You have to give a campaign file to ``Camping``.

You can give it by using the flags ``-f`` or ``--file`` followed to the path of the campaign file.

## Prologue and Epilogue

In certain situation you might want to have each node execute a piece of code before and/or after the beginning/end of the campaign.

These pieces of code are called:

* prologue: code executed by each node before start of the campaign

* epilogue: code executed by each node after the end of the campaign

You can give a prologue (resp. epilogue) to ``Camping`` by using the flags ``-p`` or ``--prologue`` (resp. ``-e`` or ``--epilogue``) followed by the code to execute.

## Full command

```bash
mpirun -np NB_PROCESSES -H HOSTFILE camping -f my_campaign.txt -p 'hostname' -e 'echo "Goodbye !"'
```

# Installation

## Dependencies

* ``MPI`` (development was done with ``openmpi``)

### Installation

 ```bash
make
```
