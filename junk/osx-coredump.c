#include <stdio.h>
#include <unistd.h>

/*
 * So it seems that no core is written but instead a text summary
 * is extracted and saved to ~/Library/Logs/DiagnosticReports/
 */

void a_function(void)
{
    *(volatile char *)0 = 1;
}

int main(int argc, char **argv)
{
    a_function();
    return 0;
}
