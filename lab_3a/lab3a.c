// // // // // // // // // // // // // // //
// Nathan Tsai       // Regan Hsu         //
// nwtsai@gmail.com  // hsuregan@ucla.edu //
// 304575323         // 604296090         //
// // // // // // // // // // // // // // //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include "ext2_fs.h"

// GLOBAL VARIABLES //

// File descriptors
int disk_image_fd = -1;

// Disk size
uint32_t d_size = 0;

// Block size
uint32_t b_size;

// Group count
int group_count;

// Global file system structs from the ext2_fs.h header file
struct ext2_group_desc* group_desc_array;
struct ext2_super_block superblock;
struct ext2_inode inode;
struct ext2_dir_entry directory;

// Linked list of inode blocks
typedef struct i_block 
{
  struct i_block* next;
  uint32_t block_within, address;
} i_block;

// HELPER FUNCTION DECLARATIONS //

// Auxilary functions that help the processing functions
void writeIndirectBlock(int inode_number, int level_of_indirection, uint32_t block_number);
i_block* readIndirectBlock(i_block* t, int type, uint32_t nBlock);
i_block* readBlock(uint32_t inode_num);

// Processing functions
void processSupers();
void processGroups();
void processBitmaps();
void processInodes();
void processDirectories();
void processIndirectBlocks();

// HELPER FUNCTION IMPLEMENTATIONS //

// Recursive function that follows the indirect pointers and prints block numbers
void writeIndirectBlock(int inode_number, int level_of_indirection, uint32_t block_number)
{
  // If the level of indirection has reached 0, simply return as there are no more pointers to follow
  if(level_of_indirection == 0)
  {
    return;
  }
  
  // Checking for pread return value
  int pread_status;
  
  // Looping variable
  uint32_t a;
  
  // Allocate space on the heap for the current block that we want to analyze
  uint32_t* current_block = (uint32_t*)malloc(b_size);
  
  // Calculate the base offset of the current block we are reading
  uint32_t BASE = b_size * block_number;
  
  // Read the disk image
  pread_status = pread(disk_image_fd, current_block, b_size, BASE);
  if(pread_status < 0)
  {
    fprintf(stderr, "Error: unable to read block from disk\n");
    exit(2);
  }
  
  // For all blocks in the current block, print out information as long as the block pointer is not NULL
  for(a = BASE; a < BASE + b_size; a += 4)
  {
    if(current_block[(a - BASE)/4] != 0)
    {
      // Calculating the current file offset for the block
      uint32_t OFFSET = 12;
      if(level_of_indirection == 2)
      {
        OFFSET = 12 + 256;
      }
      else if(level_of_indirection == 3)
      {
        OFFSET = 12 + 256 + 256*256;
      }
      OFFSET = OFFSET + (a - BASE) / 4;
      
      /*
        INDIRECT
        I-node number of the owning file (decimal)
        (decimal) level of indirection for the block being scanned ... 1 single indirect, 2 double indirect, 3 tripple
        file offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
        block number of the (1,2,3) indirect block being scanned (decimal) ... not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
        block number of the referenced block (decimal)
      */

      // Print to stdout
      fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", 
        inode_number, 
        level_of_indirection, 
        OFFSET, 
        block_number, 
        current_block[(a - BASE)/4]
      );
    }
  }
  
  // For all blocks in the current block, recursively call the this function but decrease the level of indirection
  for(a = BASE; a < BASE + b_size; a += 4)
  {
    if(current_block[(a - BASE)/4] != 0)
    {
      writeIndirectBlock(inode_number, level_of_indirection - 1, current_block[(a - BASE)/4]);
    }
  }
  
  // Free the heap space for memory reuse
  free(current_block);
}

// Read an indirect inode element
i_block* readIndirectBlock(i_block* t, int type, uint32_t nBlock) 
{
  uint32_t a, *block;
  int pread_status;
  block = malloc(b_size);
  pread_status = pread(disk_image_fd, block, b_size, b_size*nBlock);
  a = 0;
  if(pread_status < 0)
  {
    fprintf(stderr, "Error: unable to read block from disk\n");
    exit(2);
  }
  while(a < b_size/4) 
  {
    if (block[a] && type == 1) 
    {
      t->next = malloc(sizeof(i_block));
      t->next->address = b_size*block[a];
      t->next->block_within = nBlock;
      t->next->next = NULL;
      t = t->next;
    }
    else if(block[a])
    {
      t = readIndirectBlock(t,type-1,block[a]);
    }
    a++;
  }
  free(block);
  return t;
}

// Read an inode element directly
i_block* readBlock(uint32_t i_num) 
{
  if (i_num <= 0)
  {
    return NULL;
  }
  int a, group, offset, pread_status;
  uint32_t b;
  uint8_t inode_bitmap;
  i_block *h, *t;
  struct ext2_inode node;
  i_block* element;
  h = t = NULL;
  a = i_num - 1, offset = a % superblock.s_inodes_per_group, group = a / superblock.s_inodes_per_group;
  b = 0;
  pread_status = pread(disk_image_fd, &inode_bitmap, 1, group_desc_array[group].bg_inode_bitmap * b_size + offset / 8);
  if(pread_status < 0)
  {
    fprintf(stderr, "Error: unable to read inode bitmap from disk\n");
    exit(2);
  }
  if ((1 << (offset % 8)) & inode_bitmap)
  {
    pread_status = pread(disk_image_fd, &node, sizeof(struct ext2_inode), b_size * group_desc_array[group].bg_inode_table + offset * sizeof(struct ext2_inode));
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read inode from disk\n");
      exit(2);
    }
  }
  
  // Loop from 0 to 12
  while(b < 12) 
  {
    if(!node.i_block[b])
    {
      break;
    }
    element = malloc(sizeof(i_block));
    element->next = NULL;
    element->address = node.i_block[b] * b_size;
    element->block_within = -1;
    if(t)
    {
      t->next = element;
    }
    else 
    {
      h = element;
    }
    t = element;
    b++;
  }
  
  // When b hits 12, loop from 12 to 14
  if(b == 12)
  {
    while(b < 15)
    {
      if(node.i_block[b])
      {
        t = readIndirectBlock(t,b - 11, node.i_block[b]);  
      }
      b++;
    }
  }
  return h;
}

// Create superblock summary
void processSupers() 
{
  // Checking for pread return value
  int pread_status;
  
  // Read the disk image
  pread_status = pread(disk_image_fd, &superblock, EXT2_MIN_BLOCK_SIZE, EXT2_MIN_BLOCK_SIZE);
  if(pread_status < EXT2_MIN_BLOCK_SIZE) 
  {
    fprintf(stderr, "Error: unable to read superblock from disk\n");
    exit(2);
  }
  
  // Calculate the b_size as given in comments in the header file
  b_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;

  /*
    SUPERBLOCK
    total number of blocks (decimal)
    total number of i-nodes (decimal)
    block size (in bytes, decimal)
    i-node size (in bytes, decimal)
    blocks per group (decimal)
    i-nodes per group (decimal)
    first non-reserved i-node (decimal)
  */
    
  // Print to stdout
  fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", 
    superblock.s_blocks_count, 
    superblock.s_inodes_count,
    b_size, superblock.s_inode_size, 
    superblock.s_blocks_per_group, 
    superblock.s_inodes_per_group, 
    superblock.s_first_ino
  );
}

// Create group summary
void processGroups()
{
  // Checking for pread return value
  int pread_status;
  
  // For loop parameter
  int i;
  
  // Disk variables
  __uint16_t head;
  __uint16_t group_desc_array_size;
  
  // Find the beginning of the superblock and increment
  head = superblock.s_first_data_block;
  head++;
  
  // Find the number of groups by dividing block count and blocks per group, adding 1 if any remainder is left over
  group_count = superblock.s_blocks_count / superblock.s_blocks_per_group + (superblock.s_blocks_count % superblock.s_blocks_per_group ? 1 : 0);
  
  // Set the size of the group_desc_arry and then allocate space on heap for the array
  group_desc_array_size = group_count * sizeof(struct ext2_group_desc);
  group_desc_array = malloc(group_desc_array_size);
  
  // Read the disk into group_desc_array
  pread_status = pread(disk_image_fd, group_desc_array, group_desc_array_size, head * b_size);
  if(pread_status < group_desc_array_size)
  {
    fprintf(stderr, "Error: Unable to read group_desc_array from disk\n");
    exit(2);
  }
    
  // Print formatted output for each group that exists
  for(i = 0; i < group_count; i++)
  {
    /*
      GROUP
      group number (decimal, starting from zero)
      total number of blocks in this group (decimal)
      total number of i-nodes in this group (decimal)
      number of free blocks (decimal)
      number of free i-nodes (decimal)
      block number of free block bitmap for this group (decimal)
      block number of free i-node bitmap for this group (decimal)
      block number of first block of i-nodes in this group (decimal)
    */

    // Print to stdout
    fprintf(stdout, "GROUP,%u,%d,%d,%d,%d,%d,%d,%d\n", 
      i, 
      superblock.s_blocks_count, 
      superblock.s_inodes_per_group, 
      group_desc_array[i].bg_free_blocks_count,
      group_desc_array[i].bg_free_inodes_count, 
      group_desc_array[i].bg_block_bitmap,
      group_desc_array[i].bg_inode_bitmap,
      group_desc_array[i].bg_inode_table
    );
  }
}

// Create a free block bitmap and free I-node bitmap summary
void processBitmaps()
{
  // Checking for pread return value
  int pread_status;
  
  // For loop parameters
  int a;
  uint32_t b;
  int c;
  
  // Block and inode number
  int block_number = 1;
  int inode_number = 1;
  
  // Cap values
  int inode_cap = 0;
  int block_cap = 0;
  
  // Bitmap arrays that live on the heap
  uint8_t* free_block_bitmap = (uint8_t*)malloc(b_size * sizeof(uint8_t));
  uint8_t* free_inode_bitmap = (uint8_t*)malloc(b_size * sizeof(uint8_t));

  // Loop through each group
  for(a = 0; a < group_count; a++) 
  {
    // Add the number of blocks and inodes per group for each group there is to get total values
    block_cap += superblock.s_blocks_per_group;
    inode_cap += superblock.s_inodes_per_group;
    
    // If we are dealing with the very last group
    if (group_count - 1 == a) 
    {
      block_cap = superblock.s_blocks_count;
      inode_cap = superblock.s_inodes_count;
    }
  
    // Read from disk to populate our free block bitmap
    pread_status = pread(disk_image_fd, free_block_bitmap, b_size, group_desc_array[a].bg_block_bitmap * b_size);
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read free block bitmap from disk\n");
      exit(2);
    }
    
    // For each value in the block size
    for(b = 0; b < b_size; b++) 
    {
      for(c = 0; c < 8; c++) 
      {
        // If the block number exceeds the total number of blocks, we've gone out of bounds
        if(block_number > block_cap)
        {
          break;
        }
        
        // If we have encountered a free block, print the corresponding block number
        if(((1 << c) & (free_block_bitmap[b])) == 0) 
        {
          // Print to stdout
          fprintf(stdout, "BFREE,%d\n", block_number);
        }
        
        // Increment the block number
        block_number++;
      }
    }

    // Read from disk to populate our free inode bitmap
    pread_status = pread(disk_image_fd, free_inode_bitmap, b_size, group_desc_array[a].bg_inode_bitmap * b_size);
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read free inode bitmap from disk\n");
      exit(2);
    }
    
    // For each value in the block size
    for(b = 0; b < b_size; b++) 
    {
      for(c = 0; c < 8; c++) 
      {
        // If the inode number exceeds the total number of inodes, we've gone out of bounds
        if(inode_number > inode_cap)
        {
          break;
        }
        
        // If we have encountered a free inode, print the corresponding inode number
        if(((1 << c) & (free_inode_bitmap[b])) == 0) 
        {
          // Print to stdout
          fprintf(stdout, "IFREE,%d\n", inode_number);
        }
        
        // Increment the inode number
        inode_number++;
      }
    }
  }
  
  // Free the heap values for memory reuse
  free(free_block_bitmap);
  free(free_inode_bitmap);
}

// Create inode summary
void processInodes()
{
  // Checking for pread return value
  int pread_status;
  
  // For loop parameters
  int a;
  uint32_t b;
  int c;
  int offset;
  
  // Variables describing the inode metadata
  int inode_number;
  int inode_cap;
  char file_type;
  uint32_t octal_mode;
  uint32_t num_blocks;
  uint8_t* inode_bitmap;               
  inode_cap = 0;
  inode_number = 1;
  inode_bitmap = (uint8_t*)malloc(b_size * sizeof(uint8_t));
  
  // Loop through each group
  for(a = 0; a < group_count; a++)
  {
    // Calculate the inode maximum value
    inode_cap = (a == group_count - 1) ? superblock.s_inodes_count : inode_cap + superblock.s_inodes_per_group;
    
    // Read from disk to populate the inode_bitmap
    pread_status = pread(disk_image_fd, inode_bitmap, b_size, b_size * group_desc_array[a].bg_inode_bitmap);
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read inode bitmap from disk\n");
      exit(2);
    }
    
    // For each value in the block size
    for(b = 0; b < b_size; b++)
    {
      for(c = 0; c < 8; c++)
      {
        // If the inode number exceeds the total number of inodes, we've gone out of bounds
        if(inode_number > inode_cap) 
        {
          break;
        }
        
        // If we encounter an inode that is NOT free
        if(((1 << c) & (inode_bitmap[b])) != 0)
        {
          // Read the inodes from disk
          pread_status = pread(disk_image_fd, &inode, sizeof(struct ext2_inode), (c + 8 * b) * sizeof(struct ext2_inode) + b_size * group_desc_array[a].bg_inode_table);
          if(pread_status < 0)
          {
            fprintf(stderr, "Error: unable to read inode from disk\n");
            exit(2);
          }
          
          // Calculate the octal mode of the inode
          octal_mode = inode.i_mode & 0x01FF;
          
          // An I-node is only valid if the mode and link count are both non-zero
          if(octal_mode != 0 && inode.i_links_count != 0)
          {
            // Calculate the file type of the inode
            file_type = (0x8000 & inode.i_mode) ? 'f' : (0xA000 & inode.i_mode) ? 's' : (0x4000 & inode.i_mode) ? 'd' : '?';
            
            // Convert epoch creation time to mm/dd/yy hh:mm:ss
            time_t c_time = (time_t)inode.i_ctime;
            char c_time_string[26];
            struct tm* c_time_info;
            c_time_info = gmtime(&c_time);
            strftime(c_time_string, 26, "%m/%d/%y %H:%M:%S", c_time_info);
            
            // Convert epoch creation time to mm/dd/yy hh:mm:ss
            time_t m_time = (time_t)inode.i_mtime;
            char m_time_string[26];
            struct tm* m_time_info;
            m_time_info = gmtime(&m_time);
            strftime(m_time_string, 26, "%m/%d/%y %H:%M:%S", m_time_info);
            
            // Convert epoch creation time to mm/dd/yy hh:mm:ss
            time_t a_time = (time_t)inode.i_atime;
            char a_time_string[26];
            struct tm* a_time_info;
            a_time_info = gmtime(&a_time);
            strftime(a_time_string, 26, "%m/%d/%y %H:%M:%S", a_time_info);
  
            // Calculate the number of blocks, given log(block_size) we can use 1 << x to achieve 2^x, giving us block_size
            num_blocks = inode.i_blocks / (1 << superblock.s_log_block_size);
            
            /*
              INODE
              inode number (decimal)
              file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
              mode (octal)
              owner (decimal)
              group (decimal)
              link count (decimal)
              creation time (mm/dd/yy hh:mm:ss, GMT)
              modification time (mm/dd/yy hh:mm:ss, GMT)
              time of last access (mm/dd/yy hh:mm:ss, GMT)
              file size (decimal)
              number of blocks (decimal)
            */
            
            // Print to stdout
            fprintf(stdout, "INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", 
              inode_number,
              file_type,
              octal_mode, 
              inode.i_uid, 
              inode.i_gid, 
              inode.i_links_count, 
              c_time_string,
              m_time_string, 
              a_time_string,
              inode.i_size,
              num_blocks
            );
            
            // Print 15 block addresses (decimal, 12 direct, one indirect, one double indirect, one tripple indirect)
            for(offset = 0; offset <= EXT2_TIND_BLOCK; offset++)
            {
              // Print to stdout
              fprintf(stdout, ",%d", inode.i_block[offset]);
              
              // If we printed the last block address, append a new line
              if(offset == EXT2_TIND_BLOCK)
              {
                // Print a new line to stdout
                fprintf(stdout, "\n");
              }
            }
          }
        }
        
        // Increment the inode number
        inode_number++;
      }
    }
  }
  
  // Free the heap space
  free(inode_bitmap);
}

// Create directory summary
void processDirectories()
{
  int a, c ,pread_status;
  uint8_t* i_block_bitmap;
  uint32_t offset, b, n_inode_parent, max_inode;
  max_inode = 0;
  n_inode_parent = 1;
  i_block_bitmap = (uint8_t*)malloc(b_size * sizeof(uint8_t));

  for(a = 0; a < group_count; a++)
  {
    max_inode += superblock.s_inodes_per_group;
    if(a == group_count - 1)
    {
      max_inode = superblock.s_inodes_count;
    }
    pread_status = pread(disk_image_fd, i_block_bitmap, b_size, b_size * group_desc_array[a].bg_inode_bitmap);
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read directory from disk\n");
      exit(2);
    }
    for(b = 0; b < b_size; b++)
    {
      for(c = 0; c < 8; c++)
      {
        if(n_inode_parent > max_inode)
        {
          break;
        }
        if((1 << c) & i_block_bitmap[b])
        {
          pread_status = pread(disk_image_fd, &inode, sizeof(struct ext2_inode), (c+8*b)*sizeof(struct ext2_inode) + (b_size* group_desc_array[a].bg_inode_table));
          if(pread_status < 0)
          {
            fprintf(stderr, "Error: unable to read directory from disk\n");
            exit(2);
          }
          if(0x4000 & inode.i_mode)
          {
            i_block* h = readBlock(n_inode_parent);
            i_block* temp = h;
            while(temp != NULL)
            {
              uint32_t position = temp->address;
              memset(directory.name, 0, EXT2_NAME_LEN);
              pread_status = pread(disk_image_fd, &directory, 8, position);
              if(pread_status < 0)
              {
                fprintf(stderr, "Error: unable to read directory from disk\n");
                exit(2);
              }
              pread_status = pread(disk_image_fd, &(directory.name), directory.name_len, position + 8);
              if(pread_status < 0)
              {
                fprintf(stderr, "Error: unable to read directory from disk\n");
                exit(2);
              }
              if(directory.inode != 0)
              {
                /*
                  DIRENT
                  parent inode number (decimal) ... the I-node number of the directory that contains this entry
                  byte offset (decimal) of this entry within the directory
                  inode number of the referenced file (decimal)
                  entry length (decimal)
                  name length (decimal)
                  name (string, surrounded by single-quotes).
                */
                
                offset = 0;
                fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,\'%s\'\n", n_inode_parent, offset, directory.inode, directory.rec_len, directory.name_len, directory.name);
                
                // Increment the offset
                offset += directory.rec_len;
              }
              while(position + directory.rec_len < b_size + temp->address)
              {
                position += directory.rec_len;
                memset(directory.name, 0, EXT2_NAME_LEN);
                pread_status = pread(disk_image_fd, &directory, 8, position);
                if(pread_status < 0)
                {
                  fprintf(stderr, "Error: unable to read directory from disk\n");
                  exit(2);
                }
                pread_status = pread(disk_image_fd, &(directory.name), directory.name_len, position+8);
                if(pread_status < 0)
                {
                  fprintf(stderr, "Error: unable to read directory from disk\n");
                  exit(2);
                }
                if(directory.inode != 0)
                {
                  /*
                    DIRENT
                    parent inode number (decimal) ... the I-node number of the directory that contains this entry
                    byte offset (decimal) of this entry within the directory
                    inode number of the referenced file (decimal)
                    entry length (decimal)
                    name length (decimal)
                    name (string, surrounded by single-quotes).
                  */
                  
                  // Print to stdout
                  fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,\'%s\'\n", 
                    n_inode_parent, 
                    offset, 
                    directory.inode, 
                    directory.rec_len, 
                    directory.name_len, 
                    directory.name
                  );
                  
                  // Increment the offset
                  offset += directory.rec_len;
                }
              }
              
              // Move on to the next element
              temp = temp->next;
            }
          }
        }
        n_inode_parent++;
      }
    }
  }
  free(i_block_bitmap);
}

// Create indirect block reference summary
void processIndirectBlocks()
{
  // Checking for pread return value
  int pread_status;
  
  // For loop parameters
  int a;
  uint32_t b;
  int c;
  int offset;
  
  // Variables describing the inode metadata
  int inode_number;
  int inode_cap;
  uint8_t* inode_bitmap;               
  inode_cap = 0;
  inode_number = 1;
  inode_bitmap = (uint8_t*)malloc(b_size * sizeof(uint8_t));
  
  // Loop through each group
  for(a = 0; a < group_count; a++)
  {
    // Calculate the inode maximum value
    inode_cap += superblock.s_inodes_per_group;
    if (a == group_count - 1)
    {
      inode_cap = superblock.s_inodes_count;
    }
    
    // Read from disk to populate the inode_bitmap
    pread_status = pread(disk_image_fd, inode_bitmap, b_size, b_size * group_desc_array[a].bg_inode_bitmap);
    if(pread_status < 0)
    {
      fprintf(stderr, "Error: unable to read inode bitmap from disk\n");
      exit(2);
    }
    
    // For each value in the block size
    for(b = 0; b < b_size; b++)
    {
      for(c = 0; c < 8; c++)
      {
        // If the inode number exceeds the total number of inodes, we've gone out of bounds
        if(inode_number > inode_cap) 
        {
          break;
        }
        
        // If we encounter an inode that is NOT free
        if(((1 << c) & (inode_bitmap[b])))
        {
          // Read the inodes from disk
          pread_status = pread(disk_image_fd, &inode, sizeof(struct ext2_inode), (c + 8 * b) * sizeof(struct ext2_inode) + b_size * group_desc_array[a].bg_inode_table);
          if(pread_status < 0)
          {
            fprintf(stderr, "Error: unable to read inode from disk\n");
            exit(2);
          }
          
          // Calculate the number of blocks, given log(block_size) we can use 1 << x to achieve 2^x, giving us block_size
          uint32_t num_blocks = inode.i_blocks / (1 << superblock.s_log_block_size);
          
          // If number of blocks exceeds the minimum log block size
          if(num_blocks > EXT2_MIN_BLOCK_LOG_SIZE)
          {
            // Loop through 12 -> 14 to check single -> triple indirect pointers
            for(offset = EXT2_IND_BLOCK; offset <= EXT2_TIND_BLOCK; offset++)
            {
              if(inode.i_block[offset] != 0)
              {
                int level_of_indirection = offset + 1 - EXT2_IND_BLOCK;
                writeIndirectBlock(inode_number, level_of_indirection, inode.i_block[offset]);
              }
            }
          }
        }
        
        // Increment the inode number
        inode_number++;
      }
    }
  }
  
  // Free the heap space
  free(inode_bitmap);
}

// MAIN //

int main(int argc, char** argv)
{
  int close_status;
  
  // Check if the number of arguments is correct
  if(argc != 2)
  {
    fprintf(stderr, "Error: Incorrect number of arguments\nCorrect usage: ./lab3a EXT2_test.img\n");
    exit(1);
  }
  
  // Grab the name of the disk
  char* disk_image_name = argv[1];
  
  // Open the file and save the file descriptor to a global variable
  disk_image_fd = open(disk_image_name, O_RDONLY);
  if (disk_image_fd < 0) 
  {
    fprintf(stderr, "Error: Could not open disk image\n");
    exit(2);
  }
  
  // Use stat to calculate the size of the disk
  struct stat disk_info;
  stat(argv[1], &disk_info);
  d_size = disk_info.st_size;
  
  // Process the different components of the file system image
  processSupers();
  processGroups();
  processBitmaps();
  processInodes();
  processDirectories();
  processIndirectBlocks();

  // Close the disk image file before exiting normally
  close_status = close(disk_image_fd);
  if (close_status < 0) 
  {
    fprintf(stderr, "Error: Could not close disk image\n");
    exit(2);
  }
  
  // Free heap space
  free(group_desc_array);
  
  // Exit normally
  exit(0);
}