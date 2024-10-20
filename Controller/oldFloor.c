void append_floor(client_t *client, int *floor)
{
  /* only append if floor isn't in queue */
  for (int index = 0; index < client->queue_length; index++)
  {
    if (client->queue[index] == *floor)
    {
      return;
    }
  }
  client->queue_length += 1;
  client->queue = realloc(client->queue, sizeof(int) * client->queue_length);
  client->queue[client->queue_length - 1] = *floor;
}

void add_floors_to_queue(client_t *client, int *source_floor, int *destination_floor)
{
  /* if empty queue - just add in like usual */
  if (client->queue_length == 0)
  {
    append_floor(client, source_floor);
    append_floor(client, destination_floor);
    return;
  }

  /* if either floor won't fit in the range of the queue they'll need to be appended */
  if (!floors_are_in_range(*source_floor, *destination_floor, client->queue[0], client->queue[client->queue_length - 1]))
  {
    append_floor(client, source_floor);
    append_floor(client, destination_floor);
    return;
  }
  

  /* if both will fit in the range, then we'll add them in while sorting the queue accordingly */
}