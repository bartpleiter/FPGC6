int main() 
{
    int *p = (int *)65000; // set address
    *p = 0; // write value
    while (*p < 120); // wait 120 frames (two seconds)
    return *p;
}

void interrupt()
{
    // TODO: check for intID 4
    
    // set pointer to manual address
    int *p = (int *)65000;

    // write value at pointer address
    *p += 1;
}