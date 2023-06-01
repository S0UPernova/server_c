#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct file
{
  FILE *address;
  long size;
  char *content;
};

struct file *load_file(char * file_path)
{
  FILE *requested_file = NULL;
  requested_file = fopen(file_path, "r");
  if (requested_file == NULL) {
    return NULL;
  }
  // Determine the file size
  fseek(requested_file, 0, SEEK_END);
  long file_size = ftell(requested_file);
  fseek(requested_file, 0, SEEK_SET);

  // Allocate memory to store the file contents
  // char *file_content = malloc(file_size + 1);
  char *file_content = malloc(file_size + 1);
  if (file_content == NULL)
  {
    perror("Memory allocation failed");
    free(file_content);
    fclose(requested_file);
    return NULL;
    // continue;
  }

  // Read the file contents into memory
  size_t bytes_read = fread(file_content, 1, file_size, requested_file);
  if (bytes_read != file_size)
  {
    perror("Error reading the file");
    fclose(requested_file);
    free(file_content);
    // continue;
    return NULL;
  }
    // Add ending NULL byte
  file_content[file_size] = '\0';

  struct file *output = malloc(sizeof(struct file));
  output->address = requested_file;
  output->size = file_size;
  output->content = file_content;
  return output;
}


// Closes struct file, and frees memory used by it.
void close_file(struct file *file)
{
  fclose(file->address);
  free(file->content);
  free(file);
  return;
}

// Helper function to check if a string ends with a specific suffix
int ends_with(char *str, char *suffix)
{
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (str_len < suffix_len)
    return 0;
  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}