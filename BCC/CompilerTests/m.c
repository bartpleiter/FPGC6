struct home_address{
     int local_street;
     char *town;
     char *my_city;
     char *my_country;
   }addr;
   
int main() {
   

   addr.local_street = 7;
   addr.town = "Agra";

   return addr.local_street; //7
}

void interrupt()
{

}