#ifndef string_functions_h
#define string_functions_h

#ifdef __cplusplus
extern "C"
{
#endif
  char *replace_placeholder(char *str, char *placeholder, char *content);
  int ends_with(char *str, char *suffix);
  void replace_all(char **str, char *find, char *replace);
#ifdef __cplusplus
}
#endif

#endif // string_functions_h
