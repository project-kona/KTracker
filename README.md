# KTracker  

KTracker is a ptrace based tracer that determines an application's cache-line granularity write set in fixed time windows.  
Details about KTracker are in the ASPLOS 2021 paper: [Rethinking Software Runtimes for Disaggregated Memory](https://asplos-conference.org/abstracts/asplos21-paper210-extended_abstract.pdf).   
The artifacts and instructions are available from: [asplos21-ae](https://github.com/project-kona/asplos21-ae).


## Description  

KTracker tracks an application's write set at cache-line granularity by comparing snapshots of the application’s memory in software. KTracker uses ptrace to attach to a running process and create snapshots of its memory. Later, it diffs the application’s memory with the copy to find out dirty cache lines KTracker runs the application for a fixed amount of time, which gives us an indication of the application performance in real time. KTracker updates its memory snapshot every second (a configurable parameter) and includes all accessed pages. 

KTracker uses an alternative approach to fork and copy-on-write to snapshot an application’s memory. While a copy-on-write approach would be faster than KTracker, it also has side-effects on the running applications by causing write page faults. While the application runs at full speed during the execution, KTracker suffers from overheads in doing the diffs. KTracker can also run in write-protection mode, where it write-protects pages to track which pages have changed, emulating the virtual memory copy-on-write mechanism. 


## Instructions  

These instructions have been tested on a clean Ubuntu 20.04 installation running on a CloudLab C6420 machine.  
Make sure you have sudo access, at least 128GB RAM and at least 100GB free space for application datasets and logs.
The experiments take a long time to run and it is best to launch inside a screen session.   

Before following these instructions, make sure you have installed the applications by following 
instructions from https://github.com/project-kona/apps.

Clone the repository and submodules
```
git clone --recurse-submodules https://github.com/project-kona/KTracker.git  
```

Run all applications and produce the data 
```
cd KTracker/scripts  
./run_all.sh  
```

All output data and plots will be generated in the `results` directory.  
For each run, a new directory is created `res_<date>`.   
The generated plots are in a `plots` directory, under this `res_<date>` directory (redis\_amplif.pdf and results\_wp.pdf).  

