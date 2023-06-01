#include <stdio.h>
struct file
{
  FILE *address;
  long size;
  char *content;
};
struct file *load_file(char * file_path);
void close_file(struct file *file);