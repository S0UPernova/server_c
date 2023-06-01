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
#define PAGE_ROUTE "src/pages"
#define LAYOUTS_ROUTE "src/layouts"
// todo refactor this, and expand templating to allow components to be popped in... maybe something like angular.

#include "handle_file.h" // make sure to compile with handle_file.c
#include "string_functions.h" // make sure to compile with handle_placeholder.c

// Global so that I can free address
int sockfd, newsockfd = -1;

// function prototypes
void handle_sigint(int signum);
int ends_with(char *str, char *suffix);
int isDir(char *fileName);

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
    char file_path[PATH_MAX + 20] = "";
    if (!ends_with(uri, ".ico"))
    {
      snprintf(file_path, sizeof(file_path), "%s%s", PAGE_ROUTE, uri);
    }
    else
    {
      snprintf(file_path, sizeof(file_path), "%s%s", "src", uri);
    }

    // If the file path corresponds to a directory, set a flag
    int is_directory = 0;
    if (ends_with(uri, ".css") || ends_with(uri, ".js") || ends_with(uri, ".ico"))
    {
      is_directory = -1;
    }
    else {
      is_directory = isDir(file_path);
    }
    // if (access(file_path, F_OK) == 0 && file_path[strlen(file_path) - 1] == '/')
    if (is_directory == 0 && file_path[strlen(file_path) - 1] == '/')
    {
      snprintf(file_path, sizeof(file_path), "%s%sindex.html", PAGE_ROUTE, uri);
    }
    else if (is_directory == 0)
    {
      snprintf(file_path, sizeof(file_path), "%s%s/index.html", PAGE_ROUTE, uri);
    }

    // load requested file
    struct file *file = load_file(file_path);

    if (file == NULL)
    {

      if (ends_with(uri, "/"))
      {
        int length = strlen(uri);
        uri[length - 1] = '\0';
      }
      snprintf(file_path, sizeof(file_path), "%s%s.html", PAGE_ROUTE, uri);
      file = load_file(file_path);
      if (file == NULL)
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

    // Load the layout fil
    char layout_path[PATH_MAX];
    snprintf(layout_path, sizeof(layout_path), "%s/application.html", LAYOUTS_ROUTE);
    struct file *layout = load_file(layout_path);

    // Replace the placeholder tag in the layout file with the file contents
    char placeholder[20] = "<body-placeholder/>";
    char *final_contents = replace_placeholder(layout->content, placeholder, file->content);

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
    else if (ends_with(uri, ".ico"))
    {
      content_type = "image/x-icon";
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
      // Send the HTTP response
      ssize_t bytes_written = write(newsockfd, response, strlen(response));
      if (bytes_written < 0)
      {
        perror("Error sending response");
      }
    }
    else if (strcmp(content_type, "image/x-icon") == 0)
    {
      char *favicon_path = "src/favicon.ico";
      struct file *favicon = load_file(favicon_path);

      // Construct the HTTP response for the favicon file
      char favicon_response[BUFFER_SIZE];
      snprintf(favicon_response, sizeof(favicon_response),
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: image/x-icon\r\n"
               "Content-Length: %ld\r\n\r\n",
               favicon->size);

      // Send the HTTP response for the favicon file
      ssize_t favicon_bytes_written = write(newsockfd, favicon_response, strlen(favicon_response));
      if (favicon_bytes_written < 0)
      {
        perror("Error sending favicon response");
        free(favicon->content);
        continue;
      }

      // Send the favicon file contents
      ssize_t favicon_data_bytes_written = write(newsockfd, favicon->content, favicon->size);
      if (favicon_data_bytes_written < 0)
      {
        perror("Error sending favicon file");
        close_file(favicon);
        continue;
      }

      // Clean up resources
      close_file(favicon);
    }
    else
    {
      snprintf(response, sizeof(response),
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %ld\r\n\r\n%s",
               content_type, strlen(file->content), file->content);
      // Send the HTTP response
      ssize_t bytes_written = write(newsockfd, response, strlen(response));
      if (bytes_written < 0)
      {
        perror("Error sending response");
      }
    }
    close_file(file);
    close_file(layout);
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

int isDir(char *fileName)
{
  struct stat path;

  if (stat(fileName, &path) == -1)
  {
    return -1;
  }

  return S_ISREG(path.st_mode);
}