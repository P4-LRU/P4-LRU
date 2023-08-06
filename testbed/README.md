# P4LRU Testbed

In this part, we describe a basic environment for testing P4LRU and show how to reproduce the testbed results shown in the paper.

Each system code contains two parts: server code and P4 code. Compiling the server code in the server is for sending and receiving code, while the P4 code is compiled in the switch for the P4LRU implementation.



## Minimum environment requests

+ Two servers with NIC supported DPDK and DPDK version == 20.11(we cannot ensure DPDK code can be compiled in other versions)
+ a Tofino switch with SDE version >= 9.2 (we tested in 9.2.0 and 9.7.0)

These two servers link with the Tofino switch. Packets can be transmitted through the switch



## Build P4 code

The P4 code of the three systems is in `./testbed/SYSTEM_NAME/P4/*.p4`. Each `*.p4` code corresponds the result line in the paper.

You can follow these steps to build and run the P4 program:

+ compile: Tofino SDE has `p4_build.sh` for compiling p4 code. There is an example:

  ```shell
  ./p4_build.sh /root/P4-LRU/testbed/LRUIndex/P4/lru_index.p4
  ```

+ run:  after compiling, you can run the P4 program

  ```shell
  cd $SDE
  ./run_switchd.sh -p lru_index
  ```

+ configuration: when running successfully, use ucli and bfrt to configurate the Tofino switch. Bfrt can be configuated with a python script. For testing, we give each P4 program a bfrt setting script.

```shell
ucli
#ucli config
#config parameters should change with your environment
port-add -/- 40G NONE
port-enb -/-
```

​	you can use a new shell to run the bfrt setting script(the parameters in the script should change with your environment):

```shell
./run_bfshell.sh -b /root/P4-LRU/testbed/LRUIndex/P4/set_lru_index.p4
```

At this point, you know how to compile, run and configurate a P4 program.



## Build server code

There are some differences in the three system server code, which results in different building methods. We will give a separate presentation.

For convenience, we write a DPDK driver for sending and receiving packets with DPDK, which is in `./testbed/driver`. If you use a different DPDK version, you may need to change the code in the driver.



### LRUIndex

code path: `./testbed/LRUIndex/`

You can follow these steps to begin a test:

+ compile: use `make` in `./testbed/LRUIndex/`

+ run the program:

  + run receiver first: 

  ```shell
  ./recver [-a NIC_code] -l 0 --file-prefix recver
  ```

  + run sender then: you can set the dataset path in `sender.cc`

  ```shell
  ./sender [-a NIC_code] -l 0 --file-prefix sender
  ```

+ waiting for finishing sending, `ctrl+C` to kill the receiver program. The results are shown in the shell.



### LRUMon

code path: `./testbed/LRUMon/`

You can follow these steps to begin a test:

+ compile: use `make` in `./testbed/LRUMon/`

+ run the program:

  + run receiver first: 

  ```shell
  ./recver [-a NIC_code] -l 0 --file-prefix recver
  ```

  + run sender then: you can set the dataset path in the last parameter

  ```shell
  ./sender [-a NIC_code] -l 0 --file-prefix sender DATASET_PATH
  ```

+ waiting for finishing sending, `ctrl+C` to kill the receiver program. The results are shown in the shell.



