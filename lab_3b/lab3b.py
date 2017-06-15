#!/usr/bin/python
# # # # # # # # # # # # # # # # # # # # #
# Nathan Tsai       # Regan Hsu         #
# nwtsai@gmail.com  # hsuregan@ucla.edu #
# 304575323         # 604296090         #
# # # # # # # # # # # # # # # # # # # # #

import sys
import csv
EXIT_CODE = 0

class Block:
    def __init__(self, block_num):
        self.block_num = block_num  # block number
        self.referred = set() # what inodes points to this block

class Superblock:
	def __init__(self, n_blocks, n_inodes, block_size, inode_size, bpg, ipg, first_non_reserved_i_node):
		self.n_blocks = int(n_blocks)
		self.n_inodes = int(n_inodes)
		self.block_size = int(block_size)
		self.inode_size = int(inode_size)
		self.bpg = int(bpg) # blocks per group
		self.ipg = int(ipg) # inodes per group
		self.first_non_reserved_i_node = int(first_non_reserved_i_node)
		
class Group:
	def __init__(self, group_num, n_blocks, n_inodes, n_free_blocks, n_free_inodes, free_block_bitmap_num, free_block_inode_num, first_inode_block):
		self.group_num = int(group_num)
		self.n_blocks = int(n_blocks)
		self.n_inodes = int(n_inodes)
		self.n_free_blocks = int(n_free_blocks)
		self.n_free_inodes = int(n_free_inodes)
		self.free_block_bitmap_num = int(free_block_bitmap_num)
		self.free_block_inode_num = int(free_block_inode_num)
		self.first_inode_block = int(first_inode_block)

class Free_block:
	def __init__(self, free_block_num):
		self.free_block_num = int(free_block_num)
	
class Free_inode:
	def __init__(self, free_inode_num):
		self.free_inode_num = int(free_inode_num)
		
class Inode:
	def __init__(self, inode_num, file_type, mode, owner, group, link_count, time_last_change, mod_time, time_of_last_access, file_size, n_blocks, blocks):
		blocks = map(int, blocks)
		self.inode_num = int(inode_num)
		self.file_type = file_type
		self.mode = int(mode)
		self.owner = int(owner)
		self.group = int(group)
		self.link_count = int(link_count)
		self.time_last_change = time_last_change
		self.mod_time = mod_time
		self.time_of_last_access  = time_of_last_access
		self.file_size = int(file_size)
		self.n_blocks = int(n_blocks)
		self.blocks = blocks[0:15] #blocks 1 to 12
		self.direct_block = blocks[12] #block 13
		self.double_block = blocks[13] # block 14
		self.triple_block = blocks[14] # block 15
		

class Directory:
	def __init__(self, parent_inode_num, byte_offset, inode_num, entry_len, name_len, name):
		self.parent_inode_num = int(parent_inode_num)
		self.byte_offset = int(byte_offset)
		self.inode_num = int(inode_num)
		self.entry_len = int(entry_len)
		self.name_len = int(name_len)
		self.name = name
		

class Indirect_block_references:
	def __init__(self, inode_num, level_of_indirection, block_offset, indirect_block_num, referenced_block_num):
		self.inode_num = int(inode_num)
		self.level_of_indirection = int(level_of_indirection)
		self.block_offset = int(block_offset)
		self.indirect_block_num = int(indirect_block_num)
		self.referenced_block_num = int(referenced_block_num)


class ProcessFile:
	def __init__(self, csv_file):
		self.superblocks = []
		self.groups = []
		self.free_blocks = []
		self.free_inodes = []
		self.inodes = []
		self.directories = []
		self.indirect_block_references = []
		self.EXIT_CODE = 0

		try:
			with open(csv_file) as csvfile:
				reader = csv.reader(csvfile, delimiter=' ', quotechar='|')
				for row in reader:
					row = " ".join(row)
					row = row.split(',')
					if row[0] == "SUPERBLOCK":
						self.superblocks.append(Superblock(row[1], row[2], row[3], row[4], row[5], row[6], row[7]))
						self.superblock = Superblock(row[1], row[2], row[3], row[4], row[5], row[6], row[7])
					elif row[0] == "GROUP":
						self.groups.append(Group(row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8]))
						self.group = self.groups[0]
					elif row[0] == "BFREE":
						self.free_blocks.append(Free_block(row[1]))
					elif row[0] == "IFREE":
						self.free_inodes.append(Free_inode(row[1]))
					elif row[0] == "INODE":
						self.inodes.append(Inode(row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8], row[9], row[10], row[11], row[12:]))
					elif row[0] == "DIRENT":
						self.directories.append(Directory(row[1], row[2], row[3], row[4], row[5], row[6]))
					elif row[0] == "INDIRECT":
						self.indirect_block_references.append(Indirect_block_references(row[1], row[2], row[3], row[4], row[5]))
		except:
			sys.stderr.write("Error: No such file exists\n")
			exit(1)
			
	
	# Create a set from 1 to number of blocks - 1
	# Remove a value in the set when we encounter it (either indirect, inode, or directory entry, or BFREE list)
	# All blocks that are left in the set are unreferenced blocks
	# Print associated errors
	def get_untaken_blocks(self):
		inode_table_blocks_size = self.superblock.inode_size * self.superblock.ipg / self.superblock.block_size	
		first_non_reserved_block = self.group.first_inode_block + inode_table_blocks_size
		unreferenced_blocks = set(list(range(1,self.superblock.n_blocks)))
		unreferenced_blocks -= set(range(0,first_non_reserved_block))
		free_blocks = [free_block.free_block_num for free_block in self.free_blocks]
		unreferenced_blocks -= set(free_blocks)
		inode_table_blocks = set(list(range(self.group.first_inode_block, self.group.first_inode_block + inode_table_blocks_size)))
		unreferenced_blocks -= inode_table_blocks
		for inode in self.inodes:
			unreferenced_blocks -= set(inode.blocks)
		for indirect_block in self.indirect_block_references:
			unreferenced_blocks -= set([indirect_block.referenced_block_num])
		for unreferenced_block in unreferenced_blocks:
			sys.stdout.write("UNREFERENCED BLOCK {0}\n".format(unreferenced_block))
			self.EXIT_CODE = 2

	
	def get_invalid_blocks(self):
		inode_table_blocks_size = self.superblock.inode_size * self.superblock.ipg / self.superblock.block_size	
		first_non_reserved_block = self.group.first_inode_block + inode_table_blocks_size
		for inode in self.inodes:
			i = 0
			for block in inode.blocks[0:12]:
				if block < 0 or block >= self.superblock.n_blocks:
					sys.stdout.write("INVALID BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block,inode.inode_num,i))
					self.EXIT_CODE = 2
				elif block > 0 and block < first_non_reserved_block:
					sys.stdout.write("RESERVED BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block,inode.inode_num,i))
					self.EXIT_CODE = 2
				i += 1
			if inode.direct_block < 0 or inode.direct_block >= self.superblock.n_blocks:
				sys.stdout.write("INVALID INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.direct_block,inode.inode_num,12))
				self.EXIT_CODE = 2
			if inode.direct_block > 0 and inode.direct_block < first_non_reserved_block:	
				sys.stdout.write("RESERVED INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.direct_block,inode.inode_num,12))
				self.EXIT_CODE = 2
			if inode.double_block < 0 or inode.double_block >= self.superblock.n_blocks:
				sys.stdout.write("INVALID DOUBLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.double_block,inode.inode_num,268))
				self.EXIT_CODE = 2
			if inode.double_block > 0 and inode.double_block < first_non_reserved_block:
				sys.stdout.write("RESERVED DOUBLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.double_block,inode.inode_num,268))
				self.EXIT_CODE = 2
			if inode.triple_block < 0 or inode.triple_block >= self.superblock.n_blocks:
				sys.stdout.write("INVALID TRIPPLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.triple_block,inode.inode_num,65804))
				self.EXIT_CODE = 2
			if inode.triple_block > 0 and inode.triple_block < first_non_reserved_block:
				sys.stdout.write("RESERVED TRIPPLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(inode.triple_block,inode.inode_num,65804))
				self.EXIT_CODE = 2
						
		
	def get_free_and_taken_blocks(self):
		inode_table_blocks_size = self.superblock.inode_size * self.superblock.ipg / self.superblock.block_size	
		first_non_reserved_block = self.group.first_inode_block + inode_table_blocks_size
		taken_blocks = []
		free_blocks = [free_block.free_block_num for free_block in self.free_blocks]
		taken_blocks += list(range(0, first_non_reserved_block))
		for inode in self.inodes:
			taken_blocks += inode.blocks
		for indirect_block in self.indirect_block_references:
			taken_blocks += [indirect_block.referenced_block_num]
		inode_table_blocks = list(range(self.group.first_inode_block, self.group.first_inode_block + inode_table_blocks_size))
		taken_blocks += inode_table_blocks
		intersection = set(taken_blocks) & set(free_blocks)
		for block in intersection:
			sys.stdout.write("ALLOCATED BLOCK {0} ON FREELIST\n".format(block))
			self.EXIT_CODE = 2
	

	def multiple_referenced_blocks(self):
		initial = {}
		duplicates = set()
		for inode in self.inodes:
			i = 0
			for block in inode.blocks:
				if block in duplicates and block != 0:
					at_most_two_inodes = []
					at_most_two_indices = [] 
					if block in initial:
						at_most_two_inodes.append(initial[block][0])
						at_most_two_indices.append(initial[block][1])
						del initial[block]
					at_most_two_inodes.append(inode)
					at_most_two_indices.append(i)
					for j in range(0, len(at_most_two_inodes)):
						if at_most_two_indices[j] == 12:
							sys.stdout.write("DUPLICATE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block, at_most_two_inodes[j].inode_num, 12))
							self.EXIT_CODE = 2
						elif at_most_two_indices[j] == 13:
							sys.stdout.write("DUPLICATE DOUBLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block, at_most_two_inodes[j].inode_num, 268))
							self.EXIT_CODE = 2
						elif at_most_two_indices[j] == 14:
							sys.stdout.write("DUPLICATE TRIPPLE INDIRECT BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block, at_most_two_inodes[j].inode_num, 65804))
							self.EXIT_CODE = 2
						else:
							sys.stdout.write("DUPLICATE BLOCK {0} IN INODE {1} AT OFFSET {2}\n".format(block, at_most_two_inodes[j].inode_num, at_most_two_indices[j]))
							self.EXIT_CODE = 2
				elif block != 0:
					duplicates.add(block)
					initial[block] = (inode, i)
				i += 1


	def mismatch_reference_count(self):
		link_counts = {}
		for directory in self.directories:
			if directory.inode_num in link_counts:
				link_counts[directory.inode_num] += 1
			else:
				link_counts[directory.inode_num] = 1
		
		for inode in self.inodes:
			if inode.inode_num not in link_counts:
				link_counts[inode.inode_num] = 0
			if inode.link_count != link_counts[inode.inode_num]:
				sys.stdout.write("INODE {0} HAS {1} LINKS BUT LINKCOUNT IS {2}\n".format(inode.inode_num, link_counts[inode.inode_num], inode.link_count))
				self.EXIT_CODE = 2


	# WHAT IS UNALLOCATED INODE??
	def bad_directory_to_inode_reference(self):
		
		# Allocated and valid inode nums
		current_allocated_inode_nums = []

		# Populating said explicit INODES with non zero type
		for inode in self.inodes:
			if inode.mode != 0:
				current_allocated_inode_nums.append(inode.inode_num)
				
		# Create inverse
		unallocated_inode_nums = []

		# VALID ENTAILS BETWEEN 0th INODE AND NUMBER OF INODES 
		all_valid_inode_nums = list(range(0, self.superblock.n_inodes + 1))

		for inode_num in all_valid_inode_nums:
			if not inode_num in current_allocated_inode_nums:
				unallocated_inode_nums.append(inode_num)
				
		for directory in self.directories:
			if directory.inode_num in unallocated_inode_nums:
				sys.stdout.write("DIRECTORY INODE {0} NAME {1} UNALLOCATED INODE {2}\n".format(directory.parent_inode_num, directory.name, directory.inode_num))
				self.EXIT_CODE = 2
			if not directory.inode_num in all_valid_inode_nums:
				sys.stdout.write("DIRECTORY INODE {0} NAME {1} INVALID INODE {2}\n".format(directory.parent_inode_num, directory.name, directory.inode_num))
				self.EXIT_CODE = 2

	
	def get_allocated_and_unallocated_inodes(self):
		# Free Inode List (everything in IFREE)
		free_inode_nums = [free_inode.free_inode_num for free_inode in self.free_inodes]
		
		# Everything explicitly stated by INODE,#,....
		current_inode_nums = []
		
		# Populating said explicit INODES with non zero type
		for inode in self.inodes:
			if inode.mode != 0:
				current_inode_nums.append(inode.inode_num)

		allocated_inode_nums = []
		unallocated_inode_nums = []
		
		# VALID ENTAILS BETWEEN 0th INODE AND NUMBER OF INODES 
		all_valid_inode_nums = list(range(self.superblock.first_non_reserved_i_node, self.superblock.n_inodes + 1))
		
		# GO FROM 0 TO 10
		for num in range(0, self.superblock.first_non_reserved_i_node):
			if num in current_inode_nums:
				allocated_inode_nums.append(num)
		
		# 11 -> 24
		for inode_num in all_valid_inode_nums:
			if not inode_num in current_inode_nums:
				unallocated_inode_nums.append(inode_num)
			else:
				allocated_inode_nums.append(inode_num)
		
		for inode_num in allocated_inode_nums:
			if inode_num in free_inode_nums:
				sys.stdout.write("ALLOCATED INODE {0} ON FREELIST\n".format(inode_num))
				self.EXIT_CODE = 2
		
		for inode_num in unallocated_inode_nums:
			if not inode_num in free_inode_nums:
				sys.stdout.write("UNALLOCATED INODE {0} NOT ON FREELIST\n".format(inode_num))
				self.EXIT_CODE = 2


	def get_directory_links(self):
		# Map inode num to its parent's inode num
		inodeToParent = {}
		
		for directory in self.directories:
			if directory.name != '\'.\'' and directory.name != '\'..\'':
				inodeToParent[directory.inode_num] = directory.parent_inode_num
			if directory.name == '\'.\'' or (directory.name == '\'..\'' and directory.parent_inode_num == 2):
				if directory.parent_inode_num != directory.inode_num:
					sys.stdout.write("DIRECTORY INODE {0} NAME {1} LINK TO INODE {2} SHOULD BE {3}\n".format(directory.parent_inode_num, directory.name, directory.inode_num, directory.parent_inode_num))
					self.EXIT_CODE = 2
		
		for directory in self.directories:
			if (directory.name == '\'..\''):
				if directory.parent_inode_num in inodeToParent and inodeToParent[directory.parent_inode_num] != directory.inode_num:
					sys.stdout.write("DIRECTORY INODE {0} NAME {1} LINK TO INODE {2} SHOULD BE {3}\n".format(directory.parent_inode_num, directory.name, directory.inode_num, inodeToParent[directory.parent_inode_num]))
					self.EXIT_CODE = 2
		

def main():
	ugh = ProcessFile(sys.argv[1])
	ugh.get_untaken_blocks()
	ugh.get_invalid_blocks()
	ugh.get_free_and_taken_blocks()
	ugh.multiple_referenced_blocks()
	ugh.mismatch_reference_count()
	ugh.get_allocated_and_unallocated_inodes()
	ugh.bad_directory_to_inode_reference()
	ugh.get_directory_links()
	exit(ugh.EXIT_CODE)


if __name__ == "__main__":
	if(len(sys.argv) == 2):
		main()
	else:
		sys.stderr.write("Error: Incorrect number of arguments\n")
		exit(1)