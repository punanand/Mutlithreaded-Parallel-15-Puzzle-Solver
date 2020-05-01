#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#define HASHTABLE_SIZE 1000099
#define LOCALBUFFER_SIZE 20
#define MAX_THREADS 1000

/* node structure for heap */
typedef struct node {
	/* square orientation */
	char str[17];
	/* stores the distance from the expected orientation */
	int dist;
	/* if this node is used in a linked list, it points to the next node in the list */
	struct node * next;
	/* pointer to the moves after which this orientation is obtained */
	char * moves;
	/* maximum number of moves that can fit in moves string */
	int max_moves;
} node;

/* stores heap for the work queue */
node heap[1000005];

/* it shores the hash table of orientation in chained format */
node * hash_table[HASHTABLE_SIZE];

/* the number of nodes currently in work queue */
int heap_size = 0;

/* the number of threads running our algorithm */
int nthreads;

/* mutex lock for accessing the working queue */
pthread_mutex_t working_queue_lock;

/* flag that will be set if any of the threads find the solution */
int found_solution;

/* the threads defined globally */
pthread_t thread_id[MAX_THREADS];

/* arguments are thread numbers starting from 0(for easy recognition of threads) */
int argument[MAX_THREADS];


/**
 * @param a 	orientation of the 15 square in string format.
 * finds the manhattan distance from the expected orientation 
 */
int get_manhattan_distance(char *a) {
	int dist = 0, i;
	for(i = 0; i < strlen(a); i++) {
		int exp_r;
		int exp_c;
		if(a[i] == 'a') {
			exp_r = 3;
			exp_c = 3;
		}
		else {
			exp_r = (a[i] - 'a' - 1) / 4;
			exp_c = (a[i] - 'a' - 1) % 4;
		}
		int tmp = abs(exp_c - (i % 4)) + abs(exp_r - i / 4);
		dist += tmp;
	}
	return dist;
}

/**
 * @param s 	string representation of the square 
 * makes a new node with the fields populated accordingly 
 */
node get_heap_node(char * s, char * moves, int max_moves) {
	node new;
	strcpy(new.str, s);
	new.dist = get_manhattan_distance(s);
	new.moves = moves;
	new.max_moves = max_moves;
	return new;
}

/**
 * @param s 	The orientation of the square in string format that needs to be inserted in the heap
 * Inserts a node corresponding to s in the heap
 */
void insert_heap_node(node new, node heap[], int * heap_size) {
	int idx;
	if(*heap_size >= 1000000)
		idx = 999999;
	else
		idx = *heap_size;
	*heap_size = idx + 1;
	heap[idx] = new;
	while(1) {
		if(idx == 0)
			break;
		int pid = (idx - 1) / 2;
		if(heap[pid].dist > new.dist) {
			heap[idx] = heap[pid];
			heap[pid] = new;
			idx = pid;
		}
		else
			break;
	}
}

/**
 * returns the node corresponding to minimum distance orientation in the heap 
 */
node extract_heap_min(node heap[], int * heap_size) {
	node tmp = heap[0];
	heap[0] = heap[*heap_size - 1];
	heap[*heap_size - 1] = tmp;
	*heap_size = *heap_size - 1;
	int idx = 0;
	while(1) {
		int lid = idx * 2 + 1;
		int rid = idx * 2 + 2;
		if(lid >= *heap_size && rid >= *heap_size)
			break;
		node left = heap[lid];
		node right = heap[rid];
		if(lid < *heap_size && (left.dist < right.dist || rid >= *heap_size)) {
			if(heap[idx].dist > left.dist) {
				node tmp = heap[idx];
				heap[idx] = left;
				heap[lid] = tmp;
				idx = lid;
			}
			else
				break;
		}
		else if(rid < *heap_size) {
			if(heap[idx].dist > right.dist) {
				node tmp = heap[idx];
				heap[idx] = right;
				heap[rid] = tmp;
				idx = rid;
			}
			else
				break;
		}
	}
	return heap[*heap_size];
}

/**
 * @param s 	string for which hash is to be calculated
 * returns the hashed value for the string 
 */
long long int compute_hash(char *s, long long int sz) {
    const int p = 31;
    const int m = sz;
    long long int hash_value = 0;
    long long int p_pow = 1;
    for (int i = 0; i < strlen(s); i++) {
    	char c = s[i];
        hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash_value;
}

/**
 * @param s 	The orientation in string format that we want to hash
 * inserts a node structure corresponding to s into the hash table
 */
void insert_in_ht(node * hash_table[], char * s, long long int sz) {
	int idx = compute_hash(s, sz);
	node * new = (node *)malloc(sizeof(node));
	strcpy(new -> str, s);
	new -> dist = get_manhattan_distance(s);
	new -> next = NULL;
	node * tmp = hash_table[idx];
	if(tmp == NULL) {
		hash_table[idx] = new;
		return;
	}
	node * prev = NULL;
	while(tmp != NULL) {
		prev = tmp;
		tmp = tmp -> next;
	}
	prev -> next = new;
	new -> next = tmp;
}

/**
 * @param s 	The orientation in string format that we want to hash
 * finds a node structure corresponding to s into the hash table
 * 0: not found 1: found
 */
int find_in_ht(node * hash_table[], char * s, long long int sz) {
	int idx = compute_hash(s, sz);
	node * tmp = hash_table[idx];
	while(tmp != NULL) {
		if(strcmp(tmp -> str, s) == 0)
			return 1;
		tmp = tmp -> next;
	}
	return 0;
}

/**
 * s 	starting orientation of the puzzle in string format
 * checks whether the given start orientation is solvable
 * 0: unsolvable 1: solvable
 */
int is_solvable(char * s) {
	int inv = 0;
	int row;
	for(int i = 0; i < strlen(s); i++) {
		if(s[i] == 'a') {
			row = i / 4;
			continue;
		}
		for(int j = i + 1; j < strlen(s); j++) {
			if(s[i] > s[j] && s[j] != 'a')
				inv++;
		}
	}
	if(row % 2 == 0 && inv % 2)
		return 1;
	if(row % 2 == 1 && inv % 2 == 0)
		return 1;
	return 0;
}

/* reads the input state of the puzzle and returns its string equivalent */
char * read_input() {
	FILE * fp = fopen("input.txt", "r");
	int i, j;
	char * ret = (char *)malloc(20);
	ret[16] = '\0';
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++) {
			int tmp;
			fscanf(fp, "%d", &tmp);
			ret[i*4 + j] = (char)('a' + tmp);
		}
	return ret;
}

/* the tree search function where each thread begins its execution */
void * tree_search(void * i) {

	// pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	int tmp = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(tmp != 0)
		printf("Error in setting state\n");

	int * id = (int *) i;
	int num_threads = nthreads;
	node local_heap[LOCALBUFFER_SIZE];
	node * local_ht[LOCALBUFFER_SIZE];
	for(int i = 0; i < LOCALBUFFER_SIZE; i++)
		local_ht[i] = NULL;
	int local_heap_size = 0;
	node min_dist_node;

	while(1) {
		/* getting lock on working queue and after taking he minimum distance orientation, instantaneously release the lock */
		pthread_testcancel();
		if(local_heap_size == 0) {
			/* no local heap, so we have to get the lock and get the minimum element */
			pthread_mutex_lock(&working_queue_lock);
			if(heap_size == 0) {
				if(found_solution) {
					pthread_mutex_unlock(&working_queue_lock);
					return NULL;
				}
				pthread_mutex_unlock(&working_queue_lock);
				continue;
			}
			if(found_solution) {
				pthread_mutex_unlock(&working_queue_lock);
				return NULL;
			}
			min_dist_node = extract_heap_min(heap, &heap_size);
			pthread_mutex_unlock(&working_queue_lock);
		}
		else if(local_heap_size == LOCALBUFFER_SIZE) {
			pthread_mutex_lock(&working_queue_lock);
			for(int i = 0; i < LOCALBUFFER_SIZE; i++) {
				if(!find_in_ht(hash_table, local_heap[i].str, HASHTABLE_SIZE)) {
					insert_heap_node(local_heap[i], heap, &heap_size);
					insert_in_ht(hash_table, local_heap[i].str, HASHTABLE_SIZE);
				}
			}
			min_dist_node = extract_heap_min(heap, &heap_size);
			pthread_mutex_unlock(&working_queue_lock);
			local_heap_size = 0;
		}
		else {
			int lock_status = pthread_mutex_trylock(&working_queue_lock);
			if(lock_status != 0) {
				min_dist_node = extract_heap_min(local_heap, &local_heap_size);
			}
			else {
				for(int i = 0; i < local_heap_size; i++) {
					if(!find_in_ht(hash_table, local_heap[i].str, HASHTABLE_SIZE)) {
						insert_heap_node(local_heap[i], heap, &heap_size);
						insert_in_ht(hash_table, local_heap[i].str, HASHTABLE_SIZE);
					}
				}
				min_dist_node = extract_heap_min(heap, &heap_size);
				pthread_mutex_unlock(&working_queue_lock);
				local_heap_size = 0;
			}
		}

		pthread_testcancel();

		printf("Thread:%d orientation: %s\n", *id, min_dist_node.str);

		if(min_dist_node.dist == 0) {
			pthread_mutex_lock(&working_queue_lock);
			if(found_solution) {
				pthread_mutex_unlock(&working_queue_lock);
				break;
			}
			found_solution = 1;
			int j;
			for(j = 0; j < num_threads; j++) {
				if(j != *id) {
					pthread_cancel(thread_id[j]);
				}
			}
			pthread_mutex_unlock(&working_queue_lock);
			printf("The solution is: %s\n", min_dist_node.moves);
			break;
		}

		/* spotting the blank position */
		char pos[17];
		strcpy(pos, min_dist_node.str);
		insert_in_ht(local_ht, pos, LOCALBUFFER_SIZE);
		int i;
		for(i = 0; i < 16; i++)
			if(pos[i] == 'a')
				break;
		int row, col;
		row = i / 4;
		col = i % 4;

		/* reallocating memory if moves are greater than memory given */
		int moves_done = strlen(min_dist_node.moves);
		if(moves_done + 1 > min_dist_node.max_moves) {
			min_dist_node.max_moves *= 2;
		}

		/* moving appropriately */
		if(row > 0) {
			char tmp[17];
			strcpy(tmp, pos);
			tmp[(row - 1) * 4 + col] = 'a';
			tmp[row * 4 + col] = pos[(row - 1) * 4 + col];
			char * moves = (char *)malloc(min_dist_node.max_moves);
			strcpy(moves, min_dist_node.moves);
			moves[strlen(min_dist_node.moves) + 1] = '\0';
			moves[strlen(min_dist_node.moves)] = 'U';
			node new = get_heap_node(tmp, moves, min_dist_node.max_moves);
			if(local_heap_size < LOCALBUFFER_SIZE - 1 && !find_in_ht(local_ht, new.str, LOCALBUFFER_SIZE)) {
				insert_heap_node(new, local_heap, &local_heap_size);
				insert_in_ht(local_ht, new.str, LOCALBUFFER_SIZE);
			}
		}
		if(row < 3) {
			char tmp[17];
			strcpy(tmp, pos);
			tmp[(row + 1) * 4 + col] = 'a';
			tmp[row * 4 + col] = pos[(row + 1) * 4 + col];
			char * moves = (char *)malloc(min_dist_node.max_moves);
			strcpy(moves, min_dist_node.moves);
			moves[strlen(min_dist_node.moves) + 1] = '\0';
			moves[strlen(min_dist_node.moves)] = 'D';
			node new = get_heap_node(tmp, moves, min_dist_node.max_moves);
			if(local_heap_size < LOCALBUFFER_SIZE - 1 && !find_in_ht(local_ht, new.str, LOCALBUFFER_SIZE)) {
				insert_heap_node(new, local_heap, &local_heap_size);
				insert_in_ht(local_ht, new.str, LOCALBUFFER_SIZE);
			}
		}
		if(col > 0) {
			char tmp[17];
			strcpy(tmp, pos);
			tmp[row * 4 + col - 1] = 'a';
			tmp[row * 4 + col] = pos[row * 4 + col - 1];
			char * moves = (char *)malloc(min_dist_node.max_moves);
			strcpy(moves, min_dist_node.moves);
			moves[strlen(min_dist_node.moves) + 1] = '\0';
			moves[strlen(min_dist_node.moves)] = 'L';
			node new = get_heap_node(tmp, moves, min_dist_node.max_moves);
			if(local_heap_size < LOCALBUFFER_SIZE - 1 && !find_in_ht(local_ht, new.str, LOCALBUFFER_SIZE)) {
				insert_heap_node(new, local_heap, &local_heap_size);
				insert_in_ht(local_ht, new.str, LOCALBUFFER_SIZE);
			}	
		}
		if(col < 3) {
			char tmp[17];
			strcpy(tmp, pos);
			tmp[row * 4 + col + 1] = 'a';
			tmp[row * 4 + col] = pos[row * 4 + col + 1];
			char * moves = (char *)malloc(min_dist_node.max_moves);
			strcpy(moves, min_dist_node.moves);
			moves[strlen(min_dist_node.moves) + 1] = '\0';
			moves[strlen(min_dist_node.moves)] = 'R';
			node new = get_heap_node(tmp, moves, min_dist_node.max_moves);
			if(local_heap_size < LOCALBUFFER_SIZE - 1 && !find_in_ht(local_ht, new.str, LOCALBUFFER_SIZE)) {
				insert_heap_node(new, local_heap, &local_heap_size);
				insert_in_ht(local_ht, new.str, LOCALBUFFER_SIZE);
			}
		}
	}
}

int main(int argc, char ** argv) {

	/* initializing the number of threads to be spawned */
	nthreads = atoi(argv[1]);
	found_solution = 0;
	struct timeval t1, t2;
	gettimeofday(&t1, 0);

	/* taking the input orientation of the puzzle and inserting it into the work queue and hash table */
	char start[17];
	strcpy(start, read_input());
	if(!is_solvable(start)) {
		printf("The given puzzle is unsolvable\n");
		return 0;
	}

	insert_in_ht(hash_table, start, HASHTABLE_SIZE);
	node new = get_heap_node(start, "", 256);
	insert_heap_node(new, heap, &heap_size);

	pthread_mutex_init(&working_queue_lock, NULL);

	/* creating the threads */
	int i;
	int lim = nthreads;
	for(i = 0; i < lim; i++) {
		argument[i] = i;
		pthread_create(&thread_id[i], NULL, tree_search, (void *) &argument[i]);
	}

	/* waiting for the threads to join */
	for(i = 0; i < lim; i++) {
		pthread_join(thread_id[i], NULL);
	}
	gettimeofday(&t2, 0);
	double time_taken = t2.tv_sec+t2.tv_usec/1e6-(t1.tv_sec+t1.tv_usec/1e6); // in seconds 
	printf("Program took %lf seconds to execute \n", time_taken);
	return 0;
}
