    #include <stdio.h>
	#include "..\cutest-1.5\CuTest.h"
    
    CuSuite* TestIdentSuite();
    
    void RunAllTests(void) {
        CuString *output = CuStringNew();
        CuSuite* suite = CuSuiteNew();
        
        CuSuiteAddSuite(suite, TestIdentSuite());
    
        CuSuiteRun(suite);
        CuSuiteSummary(suite, output);
        CuSuiteDetails(suite, output);
        printf("%s\n", output->buffer);
    }
    
    int main(void) {
        RunAllTests();
    }