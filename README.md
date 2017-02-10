HV4D
====



Implementation of [HV4D] (https://eden.dei.uc.pt/~cmfonsec/guerreiro-cccg2012.pdf), an algorithm to compute the hypervolume indicator in four dimensions, in quadratic time. Minimization is assumed.


**Note**: Although only *nondominated* points that strongly dominate the reference point contribute to the hypervolume, the code is prepared to deal with all other points, including *repeated* points, which are all ignored (in the case of repeated points, one of the copies is considered, and the remaining ones are skipped). Warnings will be raised if any of the points do not strongly dominate the reference point. Moreover, different points may have (some) equal coordinates.



License
--------
	

Except where indicated otherwise in individual source files, this software is Copyright © 2011, 2017 Andreia P. Guerreiro.

This program is free software. You can redistribute it and/or modify it under the terms of the GNU General Public License, version 3, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

Appropriate reference to this software should be made when describing research in which it played a substantive role, so that it may be replicated and verified by others. The algorithm which this software implements is described in detail in [1]. 



Building
--------


In GNU/Linux, the program can be compiled from source by invoking:

    make

It is recommended that you compile it specifically for your architecture. Depending on the compiler and version of the compiler you use, there are different ways to achieve this. For recent GCC versions, make will pick a suitable -march argument based on the processor of the build machine. This can be overridden by passing a `MARCH=` argument to make. Similarly if you use the Intel C compiler, it will pick a sensible default architecture (`-xHOST`) for you. If you want to override this, pass `XARCH=` to `make`. So, to build for an Intel *Core2* with the GCC compiler, you would use:

    make MARCH=core2

For the Intel C compiler you should use:

    make XARCH=SSSE3

Generally, `make` will try to pick good flags for you, but you can override them by passing an `OPT_CFLAGS` argument to `make` if you wish. To build an unoptimized version of `hv4d` you could run:

    make OPT_CFLAGS="-O0 -g"

Finally, if you do not want to see the command line of each compiler invocation, pass `S=1` to make.





General Usage
-------------------

**SYNOPSIS** 

    hv4d [OPTIONS] [FILE...]
    
**DESCRIPTION**

Compute the hypervolume indicator of the data set(s) in FILE(s).

With no FILE, read from the standard input.

**COMMAND LINE OPTIONS**

	 -h, --help          print this summary and exit.                          
	     --version       print version number and exit.                        
	 -v, --verbose       print some information (time, coordinate-wise maximum 
		             	 and minimum, etc)                                     
	 -q, --quiet         print only the hypervolume (as opposed to --verbose). 
	 -u, --union         treat all input sets within a FILE as a single set.   
	 -r, --reference=POINT use POINT as the reference point. POINT must be within  
		                 quotes, e.g., "10 10 10". If no reference point is  
		                 given, it is taken as the coordinate-wise maximum of  
		                 all input points.                                     
	 -s, --suffix=STRING Create an output file for each input file by appending
		                 this suffix. This is ignored when reading from stdin. 
		                 If missing, output is sent to stdout.    



Detailed Usage
-------------------

The program reads sets of points in the file(s) specified in the command line:

    ./hv4d data

or standard input:

    cat data | ./hv4d

In input files, each point is given in a separate line, and point coordinates in each line are separated by whitespace. An empty line, or a line beginning with a  hash sign (#), denotes a separate set.


Sets in an input file may be treated as a single set by using option `-u`: 

    ./hv4d data -u


The reference point can be set by giving option `-r`.

    ./hv4d -r "10 10 10 10" data

 If no reference point is given, the default is the coordinate-wise maximum of all input points in all files.

For the other options available, check the output of `./hv4d --help`.

    



Examples
--------

**Input File(s)**

Empty lines and lines starting with a hash sign (#) at the beginning and/or at the end of the file are ignored.

Example of valid content of input files:

    1   1   4   4
    4   4   1   1
    
    2   2   3   3
    3   3   2   2

Another example:

    #
    6 4 9 6
    7 3 7 1
    8 2 3 2
    9 1 2 4
    #
    1 9 5 4
    2 8 3 1
    #
    3 7 8 1
    4 6 3 9
    5 5 5 8
    #
    


**Compilation**

Example of a basic compilation:

    make march=corei7


**Execution**

The code can be tested with any of the data sets provided in folder `examples`. After compilation, run:

    ./hv4d examples/exampleInput.inp -r "1.1 1.1 1.1 1.1" 
    

The hypervolume indicator of each of the 2 data sets in `exampleInput.inp ` given the reference point `(1.1, 1.1, 1.1, 1.1)` will be printed in separate lines, in the same order as the point sets in the input file:

	0.936963193064112
	0.861528076506543

By running:


    ./hv4d examples/exampleInput.inp -r "1.1 1.1 1.1 1.1" -u
    
the result will be the hypervolume indicator of the union of the two sets in the file:

	1.05104266084533
    
    


Other sources
-------------

Version 1.1 of this code is also integrated in [PyGMO](http://esa.github.io/pygmo/index.html), and can be found [here](https://github.com/esa/pagmo/tree/master/src/util/hv_algorithm/hv4d_cpp_original).


References
----------

[1] Andreia P. Guerreiro, Carlos M. Fonseca, and Michael T. M. Emmerich. "A fast dimension-sweep algorithm for the hypervolume indicator in four dimensions". In CCCG, pages 77–82, 2012. [[pdf](https://eden.dei.uc.pt/~cmfonsec/guerreiro-cccg2012.pdf)]

[2] Andreia P. Guerreiro. Efficient algorithms for the assessment of stochastic multiobjective optimizers. Master’s thesis, IST, Technical University of Lisbon, Portugal, 2011. [[pdf](https://eden.dei.uc.pt/~cmfonsec/AndreiaGuerreiroMSc.pdf)]

