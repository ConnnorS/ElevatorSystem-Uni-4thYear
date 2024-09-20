void verify_operation(const char operation[8])
{
  const char operations[5][8] = {"Opening", "Open", "Closed", "Closing", "Between"};
  for (int i = 0; i < 5; i++)
  {
    if (strcmp(operation, operations[i]) == 0)
    {
      return;
    }
  }

  printf("Invalid operation specified\nPlease enter \"Opening\", \"Open\", \"Closed\", \"Closing\", or \"Between\"\n");
  exit(1);
}