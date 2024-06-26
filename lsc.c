#include <stdbool.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// Print all entries in a directory, including dotfiles.
static bool all_entries;

/* Use a long listing format.
   FORMAT: total TOTAL_SIZE_IN_BLOCKS
           entry_perms links_count owner associated_group size_in_bytes last_modified name 
 */
static bool long_listing;

// List a directory itself instead of its contents.
static bool list_directory;

// Recursively list subdirectories.
static bool recursive;

static struct option long_options[] =
{
  {"help", no_argument, NULL, 'h'},
  {"all", no_argument, NULL, 'a'},
  {"directory", no_argument, NULL, 'd'},
  {"recursive", no_argument, NULL, 'R'},
  {NULL, 0, NULL, 0}
};

static char *help_msg = "Usage: lsc [OPTIONS] [FILES]\n\
List information about the given FILEs, or the current directory if none are specified.\n \
\n\
  -a, --all       include all entries in the listing\n\
  -l              use a long listing format\n\
  -d, --directory list a directory itself instead of its contents\n\
  -R, --recursive recursively list subdirectories";

// Return a string representing the type and permissions of an entry.
char *entry_type_and_perms(__mode_t mode) {
  char *tp = (char *) malloc(11 * sizeof(char));

  // Set the type.
  switch (mode & S_IFMT) {
    case S_IFBLK:  tp[0] = 'b'; break;
    case S_IFCHR:  tp[0] = 'c'; break;
    case S_IFDIR:  tp[0] = 'd'; break;
    case S_IFIFO:  tp[0] = 'f'; break;
    case S_IFLNK:  tp[0] = 'l'; break;
    case S_IFREG:  tp[0] = '-'; break;
    case S_IFSOCK: tp[0] = 's'; break;
  } 

  // Set permissions.
  tp[1] = (mode & S_IRUSR) ? 'r' : '-';
  tp[2] = (mode & S_IWUSR) ? 'w' : '-';
  tp[3] = (mode & S_IXUSR) ? 'x' : '-';
  tp[4] = (mode & S_IRGRP) ? 'r' : '-';
  tp[5] = (mode & S_IWGRP) ? 'w' : '-';
  tp[6] = (mode & S_IXGRP) ? 'x' : '-';
  tp[7] = (mode & S_IROTH) ? 'r' : '-';
  tp[8] = (mode & S_IWOTH) ? 'w' : '-';
  tp[9] = (mode & S_IXOTH) ? 'x' : '-';
  tp[10] = '\0';

  return tp;
}

// Handle displaying an individual entry.
void ls_entry(char *entry_name, struct stat entry_status) {
  if (!long_listing) {
    printf("%s  ", entry_name);
  } else {
    // TODO: finish handling long listing.
    char *tp = entry_type_and_perms(entry_status.st_mode);

    // Display symlinks properly.
    if (S_ISLNK(entry_status.st_mode)) {
      char link_target_name[1024];
      ssize_t link_target_name_len;

      if ((link_target_name_len = readlink(entry_name, link_target_name, sizeof(link_target_name))) == -1) {
        fprintf(stderr, "%s", strerror(errno));
        exit(EXIT_FAILURE);
      } else {
        link_target_name[link_target_name_len] = '\0';
        printf("%s  %s -> %s\n", tp, entry_name, link_target_name);
      }
    } else {
      printf("%s  %s\n", tp, entry_name);
    }
  }
}

// Handle displaying a directory.
void ls_dir(char *dir_path) {
  DIR *dir_stream = opendir(dir_path);

  // FIX: figure out how to get the actual, relative paths of the entries to the dir the command was executed in so this doesn't get executed.
  if (dir_stream == NULL) {
    fprintf(stderr, "%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  struct dirent *entry;

  while ((entry = readdir(dir_stream)) != NULL) {
    char *entry_name = entry->d_name;

    // Don't display the `.` and `..` directories. 
    if (!strcmp(entry_name, ".") || !strcmp(entry_name, "..")) {
      continue;
    }

    // Skip dotfiles if the all entries option wasn't specified.
    if (entry_name[0] == '.' && !all_entries) {
      continue;
    }

    struct stat entry_status;

    lstat(entry_name, &entry_status);

    if (recursive) {
      printf("%s:", entry_name);
      ls_dir(entry_name);
    } else {
      ls_entry(entry_name, entry_status);
    }
  }
}

int main(int argc, char **argv) {
  int opt;

  while ((opt = getopt_long(argc, argv, "aldhR", long_options, (int *) 0)) != -1) {
    switch (opt) {
      case 'h':
        printf("%s", help_msg);
        exit(EXIT_SUCCESS);  
      case 'a':
        all_entries = true;
        break;
      case 'l':
        long_listing = true;
        break;
      case 'd':
        list_directory = true;
        break;
      case 'R':
        recursive = true;
        break;
      default:
        break;
    }
  }

  // The directory or file to list.
  char *target_path = (optind == argc - 1) ? argv[optind] : "./";

  struct stat target_status;

  if (lstat(target_path, &target_status) == 0) {
    if (S_ISDIR(target_status.st_mode) && !list_directory) {
      ls_dir(target_path);
    } else {
      ls_entry(target_path, target_status);
    }
  } else {
    fprintf(stderr, "%s: '%s'", strerror(errno), target_path);
    exit(EXIT_FAILURE);
  }
}
