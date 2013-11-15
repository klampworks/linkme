#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <alloca.h>
#include "uint128/uint128.h"
#include <errno.h>

struct node {

	struct uint128 *value;
	char *filepath;
	struct node *left,
		    *right;
};

struct node *nodes[16];

int is_hex(const char input) {

	//30 - 39 0-9
	for (char i = 0x30; i < 0x40; i++)
		if (input == i) return 1;

	//61 - 66 a-f
	for (char i = 0x61; i < 0x67; i++)
		if (input == i) return 1;

	//41 - 46 A-F
	for (char i = 0x41; i < 0x47; i++)
		if (input == i) return 1;

	return 0;
}

int is_md5(const char *input) {

	int i = 0;
	for (; input[i]; i++) {

		//Check that this digit is [0-9a-fA-F]
		if (is_hex(input[i]))
			continue;

		//Exclude the dot extension from the equation.
		if (input[i] == '.')
			break;

		//This character was not a hex digit or a dot, not an md5.
		return 0;
	}

	//The individual string characters are legit but are there the right number of them?
	return i == 32;	
}

void do_insert(struct node **current, struct node *new_node) {

	if (!*current) {
		//printf("Inserting %s\n", new_node->filepath);
		*current = new_node;
		return;
	}
	

	if (!cmp_uint128((*current)->value, new_node->value)) {
		
		//Collision!
		printf("Duplicate! %s and %s\n", (*current)->filepath, new_node->filepath);

		char *filename = strrchr(new_node->filepath, '/');
		char *filepath = new_node->filepath; 
		char *existing_file = (*current)->filepath;

		if (!filename)
			filename = filepath;
		else
			filename++;

		int filename_len = strlen(filename);

		char *tmp_filepath = alloca(filename_len + 1 + 5);
		strcpy(tmp_filepath, "/tmp/");
		strncpy(tmp_filepath + 5, filename, filename_len);
		tmp_filepath[filename_len + 5] = '\0';

		//printf("Moving %s to %s\n", filepath, tmp_filepath);
		rename(filepath, tmp_filepath);

		//printf("Linking %s to %s\n", existing_file, filepath);
		if (link(existing_file, filepath)) {
			
			//error
			printf("Link error %s, restoring original file.\n", strerror(errno));
			rename(tmp_filepath, filepath);
		}

		unlink(tmp_filepath);
		
		free(new_node->value);
		free(new_node->filepath);
		free(new_node);
		return;
	}

	if (cmp_uint128((*current)->value, new_node->value) > 0) {

		do_insert(&(*current)->left, new_node);

	} else {

		do_insert(&(*current)->right, new_node);
	}
}

void insert(struct uint128 *value, const char *filepath, const char *filename) {

	//Create a new node.
	struct node *new_node = malloc(sizeof *new_node);
	new_node->value = value;

	//Filepath will be clobbered by the next file in the directory traversal proceedure.
	int size = strlen(filepath) + 1;
	char *filepath_n = malloc(size);
	memcpy(filepath_n, filepath, size);

	new_node->filepath = filepath_n;
	new_node->left = NULL;
	new_node->right = NULL;

	//Find the first digit of the hash and use it as an
	//index for the nodes array.
	int index;

	if (*filename >= 'a')
		index = *filename - 'a' + 10;
	else if (*filepath >= 'A') 
		index = *filename - 'A' + 10;
	else
		index = *filename - '0';

	struct node **tmp = &nodes[index];
	do_insert(tmp, new_node);
}

void process_file(const char *input) {

	struct stat stat_buf;
	stat(input, &stat_buf);

	if (S_ISDIR(stat_buf.st_mode)) {
		//directory

		DIR *dir;
		struct dirent *ent;

		int path_size = strlen(input) + 2;
		int size = path_size + 20;
		char *path = malloc(size);
		
		strcpy(path, input);
		path[path_size - 2] = '/';
		path[path_size - 1] = '\0';

		if ((dir = opendir(path))) {

			//For each file ent.
			while ((ent = readdir(dir)) != NULL) {

				//Skipping . .. and hidden files.
				if (ent->d_name[0] == '.')
					continue;

				if (strlen(ent->d_name) + path_size >= size) {
					
					size = strlen(ent->d_name) + path_size;
					path = realloc(path, size);
				}

				memcpy(path+path_size - 1, ent->d_name, strlen(ent->d_name) + 1);
				process_file(path);
			}
		} else {
			printf("Could not open file %s\n", path);
		}

		closedir(dir);
		//free(path);

	} else if (S_ISREG(stat_buf.st_mode)) {
		//normal file
		
		const char *filename = strrchr(input, '/');

		if (!filename)
			filename = input;
		else
			filename++;

		if (!is_md5(filename))
			return;

		char hash[33];
		memcpy(hash, filename, 32);
		hash[32] = '\0';

		struct uint128 *hash_u = malloc(sizeof *hash_u);
		create_uint128_d(hash, hash_u);

		insert(hash_u, input, filename);
		
		//printf("%s\n", filename);
		//Add it to the tree
	}
}

void main(int argc, char **argv) {

	//Input directory
	
	for (int i = 0; i < 16; i++) {
		nodes[i] = NULL;
	}
	
	for (int i = 0; i < argc; i++) {
		process_file(argv[i]);
	}


}

