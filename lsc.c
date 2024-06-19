#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

static bool all_entries;
static bool long_listing;

static struct option long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {NULL, 0, NULL, 0}
};

int main(int argc, char **argv) {
  int opt;

  while ((opt = getopt_long(argc, argv, "al", long_options, (int *) 0)) != -1) {
    switch (opt) {
      case 'a':
        all_entries = true;
        break;
      case 'l':
        long_listing = true;
        break;
      default:
        break;
    }
  }

  // The path to ls.
  char *target_path = (optind == argc - 1) ? argv[optind] : "./";

  struct stat statbuf;

  if (stat(target_path, &statbuf) == 0) {
    if (S_ISDIR(statbuf.st_mode)) {
      DIR *dir_stream = opendir(target_path);
      struct dirent *dir;

      while ((dir = readdir(dir_stream)) != NULL) {
        char *dir_name = dir->d_name;

        if (!strcmp(dir_name, ".") || !strcmp(dir_name, "..")) {
          continue;
        }
        
        printf("%s\n", dir_name);
      }
    } else if (S_ISREG(statbuf.st_mode)) {
      
    }
  }
}
