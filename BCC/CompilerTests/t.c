
int glob = 0xf; //15

int buff[3];


int f()
{
    return glob;
}

int main() 
{
    
    glob += 3;
    return f(); //18
}


void interrupt()
{

}