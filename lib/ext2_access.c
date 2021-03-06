// ext2 definitions from the real driver in the Linux kernel.
#include "ext2fs.h"

// This header allows your project to link against the reference library. If you
// complete the entire project, you should be able to remove this directive and
// still compile your code.
#include "reference_implementation.h"

// Definitions for ext2cat to compile against.
#include "ext2_access.h"
#include <stdio.h>



///////////////////////////////////////////////////////////
//  Accessors for the basic components of ext2.
///////////////////////////////////////////////////////////

// Return a pointer to the primary superblock of a filesystem.
struct ext2_super_block * get_super_block(void * fs) {
  return (void*)((long int)fs + 1024);
}


// Return the block size for a filesystem.
__u32 get_block_size(void * fs) {
  struct ext2_super_block* sb = get_super_block(fs);
  int shift = sb->s_log_block_size;
  int size = 1024 << shift;
  return size;
}


// Return a pointer to a block given its number.
// get_block(fs, 0) == fs;
void * get_block(void * fs, __u32 block_num) {
  __u32 offset = block_num * get_block_size(fs);
  struct ext2_super_block* sb = get_super_block(fs);
  return (void*)((long int)fs + offset);
}


// Return a pointer to the first block group descriptor in a filesystem. Real
// ext2 filesystems will have several of these, but, for simplicity, we will
// assume there is only one.
struct ext2_group_desc * get_block_group(void * fs, __u32 block_group_num) {
  // descriptor table is in block immediately following the superblock
  void* sb = (void*)get_super_block(fs);
  void* dt = get_block(sb, 1);
  return dt;
}


// Return a pointer to an inode given its number. In a real filesystem, this
// would require finding the correct block group, but you may assume it's in the
// first one.
struct ext2_inode * get_inode(void * fs, __u32 inode_num) {
  struct ext2_group_desc* dt = get_block_group(fs, 1);
  int i_table_blocknum = dt->bg_inode_table;
  void* i_block = get_block(fs, i_table_blocknum);
  int size = sizeof(struct ext2_inode);
  struct ext2_inode* inode = (struct ext2_inode*)((long int)i_block + size*(inode_num - 1));
  return inode;
}



///////////////////////////////////////////////////////////
//  High-level code for accessing filesystem components by path.
///////////////////////////////////////////////////////////

// Chunk a filename into pieces.
// split_path("/a/b/c") will return {"a", "b", "c"}.
//
// This one's a freebie.
char ** split_path(char * path) {
    int num_slashes = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {
        num_slashes++;
    }

    // Copy out each piece by advancing two pointers (piece_start and slash).
    char ** parts = (char **) calloc(num_slashes, sizeof(char *));
    char * piece_start = path + 1;
    int i = 0;
    for (char * slash = strchr(path + 1, '/');
      slash != NULL;
      slash = strchr(slash + 1, '/')) {
      int part_len = slash - piece_start;
      parts[i] = (char *) calloc(part_len, sizeof(char));
      strncpy(parts[i], piece_start, part_len);
      piece_start = slash + 1;
      i++;
    }
    // Get the last piece.
    parts[i] = (char *) calloc(strlen(piece_start) + 1, sizeof(char));
    strncpy(parts[i], piece_start, strlen(piece_start));
    return parts;
}


// Convenience function to get the inode of the root directory.
struct ext2_inode * get_root_dir(void * fs) {
    return get_inode(fs, EXT2_ROOT_INO);
}


// Given the inode for a directory and a filename, return the inode number of
// that file inside that directory, or 0 if it doesn't exist there.
//
// name should be a single component: "foo.txt", not "/files/foo.txt".
__u32 get_inode_from_dir(void * fs, struct ext2_inode * dir, char * name) {
  // initialize variables
  unsigned int* dirblk = dir->i_block;
  int fst = dirblk[0], i, loc;
  struct ext2_dir_entry_2* entry = get_block(fs, fst);

  // error handling
  if ((entry->file_type != 2) || (fst == 0)) { return 0; }

  while (entry->rec_len != 0) {
    loc = 0;
    while (strlen(name) > entry->name_len) { entry = entry->rec_len + (void*)entry; }
    for (i = 0; i < entry->name_len; i++) {
      // file exists in directory
      if (entry->name[i] == name[i]) {
        loc = 1;
      }
      else {
        loc = 0;
        break;
      }
    }
    // if file exists, return the inode num of that file
    if (loc) { return entry->inode; }
    entry = entry->rec_len + (void*)entry;
  }
  return 0;
}


// Find the inode number for a file by its full path.
// This is the functionality that ext2cat ultimately needs.
__u32 get_inode_by_path(void * fs, char * path) {
  // initialize variables
  int i;
  // inode number
  __u32 num;
  struct ext2_inode* dir = get_root_dir(fs);
  // split the path
  char** splitpth = split_path(path);

  for (i = 0; splitpth[i] != 0 && i < 3; i++) {
    num = get_inode_from_dir(fs, dir, splitpth[i]);
    if (num == 0) { return 0; }
    // find inode num for a file
    dir = get_inode(fs, num);
  }
  return num;
}

