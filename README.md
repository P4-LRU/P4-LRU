### Paper P4LRU

We put all of our testbed and simulative code in this repository.

Each system is composed of **P4 code**,  control plane code(empty) and **the server code**.

We implement a LRU version code and a baseline code based on random function of each system. 

**LRUTable** takes up one pipeline of a Tofino switch; **LRUMon** takes up two and **LRUIndex** takes up two or four.

We compile the P4 code in SDE 9.2.0 successfully, but this process may last for a while.



``CPU_simulate``: CPU simulate code

``testbed``: P4 code and the server code of three system