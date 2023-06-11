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

// Helper function to check if a string ends with a specific suffix
int ends_with(char *str, char *suffix)
{
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (str_len < suffix_len)
    return 0;
  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

void replace_all(char **str, char *find, char *replace)
{
  int findLen = strlen(find);
  int replaceLen = strlen(replace);
  int strLen = strlen(*str);

  // Calculate the new length of the string after replacement
  int newLen = strLen;
  int occurrences = 0;
  char *pos = *str;
  while((pos = strstr(pos, find)) != NULL)
  {
    occurrences++;
    pos += findLen;
  }
  newLen += occurrences * (replaceLen - findLen);

  // Allocate memory fo the modified string
  char *newStr = (char *)malloc((newLen + 1) * sizeof(char));
  if (newStr == NULL)
  {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(1);
  }
  
  // Replace all occurrences of the string
  char *currentPos = *str;
  char *writePos = newStr;
  while ((pos = strstr(currentPos, find)) != NULL)
  {
    int copyLen = pos - currentPos;
    strncpy(writePos, currentPos, copyLen);
    writePos += copyLen;
    strncpy(writePos, replace, replaceLen);
    writePos += replaceLen;
    currentPos = pos + findLen;
  }
  strcpy(writePos, currentPos);

  // Free the original string, and assign the modified string
  free(*str);
  *str = newStr;
}
