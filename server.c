#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define PATH_MAX 1024

// todo add layout and use page files as partials

// Global so that I can free address
int sockfd, newsockfd = -1;

// function prototypes
void handle_sigint(int signum);
int ends_with(char *str, char *suffix);
int isDir(char *fileName);
char *replace_placeholder(char *str, char *placeholder, char *content);

int main()
{
  signal(SIGINT, handle_sigint);
  char buffer[BUFFER_SIZE] = "";
  memset(buffer, 0, sizeof(buffer));
  // Create a socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    perror("webserver (socket)");
    return 1;
  }
  printf("socket created successfully\n");

  // Enable the SO_REUSEADDR option
  int reuseaddr = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
  {
    perror("webserver (setsockopt)");
    close(sockfd);
    return 1;
  }

  // Create the address to bind the socket to
  struct sockaddr_in host_addr;
  int host_addrlen = sizeof(host_addr);

  host_addr.sin_family = AF_INET;
  host_addr.sin_port = htons(PORT);
  host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Create client address
  struct sockaddr_in client_addr;
  int client_addrlen = sizeof(client_addr);

  // Bind the socket to the address
  if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0)
  {
    perror("webserver (bind)");
    return 1;
  }
  printf("socket successfully bound to address\n");

  // Listen for incoming connections
  if (listen(sockfd, SOMAXCONN) != 0)
  {
    perror("webserver (listen)");
    return 1;
  }
  printf("server listening for connections\n");

  for (;;)
  {
    // Accept incoming connections
    newsockfd = accept(sockfd, (struct sockaddr *)&host_addr, (socklen_t *)&host_addrlen);
    if (newsockfd < 0)
    {
      
      perror("webserver (accept)");
      continue;
    }
    printf("connection accepted\n");

    // Get client address
    int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addrlen);
    if (sockn < 0)
    {
      
      perror("webserver (getsockname)");
      continue;
    }

    // Read from the socket
    int valread = read(newsockfd, buffer, BUFFER_SIZE);
    if (valread < 0)
    {
      
      perror("webserver (read)");
      continue;
    }

    // Read the request
    char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE] = "";
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), method, version, uri);

    // Construct the file path
    char file_path[PATH_MAX + 17] = "";
    snprintf(file_path, sizeof(file_path), "pages%s", uri);
    

    // If the file path corresponds to a directory, set a flag
    int is_directory = isDir(file_path);
    // if (access(file_path, F_OK) == 0 && file_path[strlen(file_path) - 1] == '/')
    if (is_directory == 0 && file_path[strlen(file_path) - 1] == '/')
    {
      snprintf(file_path, sizeof(file_path), "pages%sindex.html", uri);
    }
    else if (is_directory == 0)
    {      
      snprintf(file_path, sizeof(file_path), "pages%s/index.html", uri);
    }

    // Open the requested file
    FILE *requested_file = NULL;
    requested_file = fopen(file_path, "r");
    if (requested_file == NULL)
    {
      
      if (ends_with(uri, "/"))
      {
        int length = strlen(uri);
        uri[length - 1] = '\0';
      }
      snprintf(file_path, sizeof(file_path), "pages%s.html", uri);
      requested_file = fopen(file_path, "r");
      if (requested_file == NULL)
      {
        
        // If it's a directory and index.html doesn't exist, return 404 Not Found response
        char not_found[] = "HTTP/1.0 404 Not Found\r\n"
                           "Server: webserver-c\r\n"
                           "Connection: Closed\r\n"
                           "Content-Type: text/html\r\n\r\n"
                           "<html>Page not found</html>\r\n";
        ssize_t not_found_bytes_written = write(newsockfd, not_found, strlen(not_found));
        if (not_found_bytes_written < 0)
        {
          perror("Error sending 404 response");
        }

        continue;
      }
    }

    // Determine the file size
    fseek(requested_file, 0, SEEK_END);
    long file_size = ftell(requested_file);
    fseek(requested_file, 0, SEEK_SET);

    // Allocate memory to store the file contents
    // char *file_contents = malloc(file_size + 1);
    char *file_contents = malloc(file_size + 1);
    if (file_contents == NULL)
    {
      
      perror("Memory allocation failed");
      free(file_contents);
      fclose(requested_file);
      continue;
    }

    // Read the file contents into memory
    size_t bytes_read = fread(file_contents, 1, file_size, requested_file);
    if (bytes_read != file_size)
    {
      perror("Error reading the file");
      fclose(requested_file);
      free(file_contents);
      continue;
    }

    file_contents[file_size] = '\0';

    // Load the layout file
    char layout_path[PATH_MAX];
    snprintf(layout_path, sizeof(layout_path), "layouts/application.html");
    FILE *layout_file = fopen(layout_path, "r");
    if (layout_file == NULL)
    {
      perror("Error opening layout file");
      free(file_contents);
      continue;
    }

    // Determine the layout file size
    fseek(layout_file, 0, SEEK_END);
    long layout_size = ftell(layout_file);
    fseek(layout_file, 0, SEEK_SET);

    // Allocate memory to store the layout file contents
    char *layout_contents = malloc(layout_size + 1);
    if (layout_contents == NULL)
    {
      perror("Memory allocation failed");
      fclose(layout_file);
      free(file_contents);
      continue;
    }

    // Read the layout file contents into memory
    size_t layout_bytes_read = fread(layout_contents, 1, layout_size, layout_file);
    if (layout_bytes_read != layout_size)
    {
      perror("Error reading the layout file");
      fclose(layout_file);
      free(layout_contents);
      fclose(requested_file);
      free(file_contents);
      continue;
    }
    layout_contents[layout_size] = '\0';

    // Replace the placeholder tag in the layout file with the file contents
    char placeholder[20] = "<body-placeholder/>";
    char *final_contents = replace_placeholder(layout_contents, placeholder, file_contents);

    // Construct the HTTP response
    char response[BUFFER_SIZE];
    const char *content_type;
    if (ends_with(uri, ".css"))
    {
      content_type = "text/css";
    }
    else if (ends_with(uri, ".js"))
    {
      content_type = "application/javascript";
    }
    else
    {
      content_type = "text/html";
    }

    if (strcmp(content_type, "text/html") == 0)
    {
      snprintf(response, sizeof(response),
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %ld\r\n\r\n%s",
               content_type, strlen(final_contents), final_contents);
    }
    else
    {
      snprintf(response, sizeof(response),
               "HTTP/1.1 404 NOT FOUND\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %ld\r\n\r\n%s",
               content_type, strlen(file_contents), file_contents);
    }

    // Send the HTTP response
    ssize_t bytes_written = write(newsockfd, response, strlen(response));
    if (bytes_written < 0)
    {
      perror("Error sending response");
    }

    // Clean up resources
    fclose(requested_file);
    fclose(layout_file);
    free(file_contents);
    free(layout_contents);
    free(final_contents);

    close(newsockfd);
  }

  return 0;
}

// Signal handler for SIGINT
void handle_sigint(int signum)
{
  if (sockfd != -1)
  {
    close(sockfd);
  }
  if (newsockfd != -1)
  {
    close(newsockfd);
  }
  exit(signum);
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

int isDir(char *fileName)
{
  struct stat path;

  if (stat(fileName, &path) == -1) {
    // Error occurred while retrieving file information
    perror("stat");
    return -1;
  }

  return S_ISREG(path.st_mode);
}

// Function to replace a placeholder tag in a string with given content
char *replace_placeholder(char *str, char *placeholder, char *content)
{
  // Find the position of the placeholder tag
  const char *tag_start = strstr(str, placeholder);
  if (tag_start == NULL)
  {
    // Placeholder not found, return the original string
    return strdup(str);
  }

  // Calculate the length of the new string
  size_t placeholder_len = strlen(placeholder);
  size_t content_len = strlen(content);
  size_t new_str_len = strlen(str) - placeholder_len + content_len;

  // Create a new string to store the result
  char *new_str = malloc(new_str_len + 1);
  if (new_str == NULL)
  {
    perror("Memory allocation failed");
    return NULL;
  }

  // Copy the original string up to the placeholder position
  size_t prefix_len = tag_start - str;
  strncpy(new_str, str, prefix_len);
  new_str[prefix_len] = '\0'; // Add NULL terminator

  // Append the content
  strcat(new_str, content);

  // Append the rest of the original string
  strcat(new_str, tag_start + placeholder_len);
  return new_str;
}