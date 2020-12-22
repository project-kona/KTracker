# KTracker  

## Instructions  
These instructions have been tested on a clean Ubuntu 20.04 installation running on a CloudLab C6420 machine.  
Make sure you have sudo access and at least 100GB free space for application datasets and logs.  

Before following these instructions, make sure you have installed the applications by following 
instructions from https://github.com/project-kona/apps.

Clone the repository and submodules
```
git clone --recurse-submodules https://github.com/project-kona/KTracker.git  
```

Run all applications and produce the data 
(this will take a long time and it is best to launch this inside a screen session)
```
cd KTracker/scripts  
./run_all.sh  
```

All output data and plots will be generated in the `results` directory.  
For each run, a new directory is created `res_<date>`.   
The generated plots are in a `plots` directory, under this `res_<date>` directory (redis\_amplif.pdf and results\_wp.pdf).  

