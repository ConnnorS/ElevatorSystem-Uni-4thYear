/* expectes {car name} {operation}*/
int main(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Useage %s {car name} {operation}\n", argv[0]);
    exit(1);
  }
  return 0;
}