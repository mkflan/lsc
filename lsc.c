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
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

// Print all entries in a directory, including dotfiles.
static bool all_entries;

// List a directory itself instead of its contents.
static bool list_directory;

// Use a long listing format.
static bool long_listing;

// Don't print entry owners in long listing.
static bool no_owner;

// Don't print entry group names in long listing.
static bool no_group;

// Display inode numbers of entries in long listing.
static bool inodes;

// Display numeric user and group IDs instead of the textual user and group name in long listing.
static bool numeric_ids;

// Recursively list subdirectories.
static bool recursive;

// Print the size of each entry in blocks.
static bool entry_size;

// Print one entry per line.
static bool one_per_line;

static struct option long_options[] =
{
  {"help", no_argument, NULL, 'h'},
  {"all", no_argument, NULL, 'a'},
  {"directory", no_argument, NULL, 'd'},
  {"no-group", no_argument, NULL, 'G'},
  {"inode", no_argument, NULL, 'i'},
  {"numeric-uid-gid", no_argument, NULL, 'n'},
  {"recursive", no_argument, NULL, 'R'},
  {"size", no_argument, NULL, 's'},
  {NULL, 0, NULL, 0}
};

static char *help_msg = "Usage: lsc [OPTIONS] [FILES]\n\
List information about the given FILEs, or the current directory if none are specified.\n \
\n\
  -a, --all       include all entries in the listing\n\
  -d, --directory list a directory itself instead of its contents\n\
  -l              use a long listing format\n\
  -g              don't print entry owners in long listing\n\
  -G, --no-group  don't print entry group name in long listing\n\
  -i, --inode     display inode numbers of entries in lost listing\n\
  -R, --recursive recursively list subdirectories\n\
  -s, --size      print the size of each entry in blocks\n\
  -1              display one entry per line in non-long-listing";

// Return a string representing the type and permissions of an entry.
char *entry_type_and_perms(__mode_t mode) {
  char *tp = (char *) malloc(11 * sizeof(char));
  if (tp == NULL) {
    fprintf(stderr, "%s", strerror(errno));
    exit(EXIT_FAILURE);
  }

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

// Handle properly displaying a long listing based on command line arguments that were passed.
void handle_long_listing(char *entry_name, struct stat entry_status) {
  char *out = "";

  if (inodes) {
    asprintf(&out, "%s%i ", out, entry_status.st_ino);
  }

  if (entry_size) {
    asprintf(&out, "%s%i ", out, entry_status.st_blocks / 2);
  }

  asprintf(&out, "%s%s %i ", out, entry_type_and_perms(entry_status.st_mode), entry_status.st_nlink);

  if (!no_owner) {
    if (numeric_ids) {
      asprintf(&out, "%s%i ", out, entry_status.st_uid);
    } else {
      struct passwd *user;
      if ((user = getpwuid(entry_status.st_uid)) == NULL) {   
        fprintf(stderr, "%s", strerror(errno));
        exit(EXIT_FAILURE);
      }
      
      asprintf(&out, "%s%s ", out, user->pw_name);
    }
  } 
  
  if (!no_group) {
    if (numeric_ids) {
      asprintf(&out, "%s%i ", out, entry_status.st_gid);
    } else {
      struct group *group;
      if ((group = getgrgid(entry_status.st_gid)) == NULL) {
        fprintf(stderr, "%s", strerror(errno));
        exit(EXIT_FAILURE);
      }

      asprintf(&out, "%s%s ", out, group->gr_name);
    }
  }

  char last_modification[64];
  strftime(last_modification, sizeof(last_modification), "%B %d %X", localtime(&entry_status.st_mtime));
  asprintf(&out, "%s%i %s %s\n", out, entry_status.st_size, last_modification, entry_name);
  
  // TODO: figure out how to display the FS blocks a directory takes.
  printf("%s", out);
    
  // Display symlinks properly.
  // if (S_ISLNK(entry_status.st_mode)) {
  //   char link_target_name[1024];
  //   ssize_t link_target_name_len;

  //   if ((link_target_name_len = readlink(entry_name, link_target_name, sizeof(link_target_name))) == -1) {
  //     fprintf(stderr, "%s", strerror(errno));
  //     exit(EXIT_FAILURE);
  //   } else {
  //     link_target_name[link_target_name_len] = '\0';
  //     printf("%s  %s -> %s\n", tp, entry_name, link_target_name);
  //   }
  // }  
}

// Handle displaying an individual entry.
void ls_entry(char *entry_path) {
  struct stat entry_status;
  if (lstat(entry_path, &entry_status) == -1) {
    fprintf(stderr, "%s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  char *entry_name = basename(entry_path);
  
  if (long_listing || no_group || numeric_ids) {
    handle_long_listing(entry_name, entry_status);
  } else {
    if (inodes) {
      printf("%i ", entry_status.st_ino);
    }

    if (entry_size) {
      printf("%i ", entry_status.st_blocks / 2);
    }

    if (one_per_line) {
      printf("%s\n", entry_name);
    } else {
      printf("%s  ", entry_name);
    }
  }
}

// Handle displaying a directory.
void ls_dir(char *dir_path) {
  DIR *dir_stream = opendir(dir_path);
  if (dir_stream == NULL) {
    fprintf(stderr, "%s: %s", strerror(errno), dir_path);
    exit(EXIT_FAILURE);
  }
  
  struct dirent *entry;

  while ((entry = readdir(dir_stream)) != NULL) {
    char *entry_name = entry->d_name;
    if (!strcmp(entry_name, ".") || !strcmp(entry_name, "..")) {
      continue;
    }
      
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir_path, entry_name);

    // TODO: fully implement recursive ls
    if (entry->d_type == DT_DIR && recursive) {
      printf("%s:", path);
      ls_dir(path);
    } else {
      ls_entry(path);
    }
  }

  closedir(dir_stream);
}

int main(int argc, char **argv) {
  int opt;

  while ((opt = getopt_long(argc, argv, "halgdinsGR1", long_options, (int *) 0)) != -1) {
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
      case 'g':
        no_owner = true;
        break;
      case 'd':
        list_directory = true;
        break;
      case 'i':
        inodes = true;
        break;
      case 'n':
        numeric_ids = true;
        break;
      case 's':
        entry_size = true;
        break;
      case 'G':
        no_group = true;
        break;
      case 'R':
        recursive = true;
        break;
      case '1':
        one_per_line = true;
        break;
      default:
        break;
    }
  }

  // The directory or file to list.
  char *target_path = (optind == argc - 1) ? argv[optind] : ".";

  struct stat target_status;

  if (lstat(target_path, &target_status) == -1) {
    fprintf(stderr, "%s: '%s'", strerror(errno), target_path);
    exit(EXIT_FAILURE);
  }

  if (S_ISDIR(target_status.st_mode) && !list_directory) {
    ls_dir(target_path);
  } else {
    ls_entry(target_path);
  }
}
