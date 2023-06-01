#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


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