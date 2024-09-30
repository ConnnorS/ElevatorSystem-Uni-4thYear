/* expect CL-arguments
{source floor} {destination floor} */
int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Useage %s {source floor} {destination floor}\n", argv[0]);
    exit(1);
  }

  
  return 0;
}