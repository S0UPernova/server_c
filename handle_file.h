#include <stdio.h>
struct file
{
  FILE *address;
  long size;
  char *content;
};
struct response
{
  char status[BUFFER_SIZE];
  char content_type[BUFFER_SIZE];
  long length;
  char content[BUFFER_SIZE];
};

struct file *load_file(char * file_path);
void close_file(struct file *file);
ssize_t respond(int socketfd, struct response *response, struct file *file);