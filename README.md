CS519 Semeseter Project - Accelerating Software Page Table Walks
Micheal Wu (mw811) & Parth A. Patel (pap223)

Setup:
1) Run kernel-setup.sh [Path to kernel source tree]

    Ex: ./kernel-setup.sh ~/linux-4.19.80
    
  NOTE: All KERNEL CHANGES WERE MADE TO VERSION 4.19.80, MAY NOT COMPILE WITH OTHER VERSIONS
  
2) Run 'make' in both pagefault-test/ and matrix-multiply/, and insert the produced executables into the target VM

  NOTE: COMPILED WITH GNU11

Testing:
1) pagefault-test

    Accepts 3 arguments:
    
        Arg 1: Number of threads to use
      
        Arg 2: Type of Access (0 - sequential, 1 - random, 2 - semi-random)
      
        Arg 3: Cache Parameter (0 - disable, 1 - enable)
      
    Ex: ./pagefault-test 4 0 1
    
      Runs the sequential access test on 4 threads, all with the software cache enabled
        
2) matrix-multiply:

    Accepts 2 arguments:
    
       Arg 1: Matrix Size
       
       Arg 2: Cache Parameter
       
    Ex: ./matrix-mult 1000 0
    
      Performs a 1000x1000 matrix multiplication with the software cache disabled
