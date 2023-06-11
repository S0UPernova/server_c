#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "handle_file.h"

#define BUFFER_SIZE 1024
struct file *load_file(char *file_path)
{
  FILE *requested_file = NULL;
  requested_file = fopen(file_path, "rb");
  if (requested_file == NULL)
  {
    return NULL;
  }
  // Determine the file size
  fseek(requested_file, 0, SEEK_END);
  long file_size = ftell(requested_file);
  fseek(requested_file, 0, SEEK_SET);

  if (!(file_size > 0))
  {
    return NULL;
  }

  // Allocate memory to store the file contents
  // char *file_content = malloc(file_size + 1);
  char *file_content = malloc(file_size + 1);
  if (file_content == NULL)
  {
    perror("Memory allocation failed");
    free(file_content);
    fclose(requested_file);
    return NULL;
  }

  // not sure if this is needed, but eh
  file_content[file_size +1] = '\0';

  // Read the file contents into memory
  size_t bytes_read = fread(file_content, 1, file_size, requested_file);
  if (bytes_read != file_size)
  {
    perror("Error reading the file");
    fclose(requested_file);
    free(file_content);
    return NULL;
  }
  if (requested_file != NULL && bytes_read > 0) {
    struct file *output = malloc(sizeof(struct file));
    output->address = requested_file;
    output->size = file_size;
    output->content = file_content;
    return output;
  }
  return NULL;
}

// Closes struct file, and frees memory used by it.
void close_file(struct file *file)
{
  fclose(file->address);
  free(file->content);
  free(file);
  return;
}

// returns -1 on failure else the length of the response
ssize_t respond(int socketfd, struct response *response, struct file *file)
{
  if (file != NULL)
  {
    // Construct the HTTP response for the favicon file
    snprintf(response->content, (strlen(response->content) + BUFFER_SIZE),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n\r\n",
             response->status, response->content_type, file->size);

    // Send the HTTP response for the file file
    ssize_t file_bytes_written = write(socketfd, response->content, strlen(response->content));
    file_bytes_written += write(socketfd, file->content, file->size);
    if (file_bytes_written < 0)
    {
      perror("Error sending response");
      return -1;
    }

    return file_bytes_written;
  }
  else
  {
    char temp[BUFFER_SIZE];

    // Construct the HTTP response for the favicon file
    snprintf(temp, (strlen(response->content) + BUFFER_SIZE),
             "HTTP/1.1 %s\r\n"
             "Server: webserver-c\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n\r\n"
             "%s",
             response->status, response->content_type, strlen(response->content), response->content);

    // Send the HTTP response for the file file
    ssize_t file_bytes_written = write(socketfd, temp, strlen(temp));
    if (file_bytes_written < 0)
    {
      perror("Error sending response");
      return -1;
    }
    return file_bytes_written;
  }
}
