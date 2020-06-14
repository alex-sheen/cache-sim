
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <getopt.h>

#include "cachelab.h"

#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>

bool verbose = false;

typedef struct _lru_t {
    long tag;
    struct _lru_t *next;
} lru_node;

typedef struct _lru_list {
    lru_node *head;
    lru_node *tail;
} lru_list;

lru_node *new_node(long tag)
{
    lru_node *temp = (lru_node*)malloc(sizeof(lru_node));
    temp->tag = tag;
    temp->next = NULL;
    return temp;
}

lru_list *create_lru()
{
    lru_list *l = (lru_list*)malloc(sizeof(lru_list));
    l->head = NULL;
    l->tail = NULL;
    return l;
}

void enqueue(lru_list *l, long tag)
{
    lru_node *temp = new_node(tag);
	if(l == NULL)
	{
		fprintf(stderr, "ERROR, enqueue given null list\n");
	}
	if(temp == NULL)
	{
		fprintf(stderr, "ERROR, enqueue couldn't allocate new lru node\n");
	}
    if (l->head == NULL) {
        l->head = temp;
        l->tail = temp;
        return;
    }
    l->tail->next = temp;
	l->tail = temp;
}

void dequeue(lru_list *l)
{
    if (l->head == NULL)
        return;

    lru_node *temp = l->head;

	if(l->tail == NULL)
	{
		return;
	}
    l->head = l->head->next;

    if (l->head == NULL)
        l->tail = NULL;

    free(temp);
}

void delete_node(lru_list *l, long tag)
{
    lru_node *temp = l->head;
    lru_node *prev = NULL;

    if (temp != NULL && temp->tag == tag)
    {
        l->head = temp->next;
		if(l->head == NULL)
		{
			l->tail = NULL;
		}
		free(temp);
        return;
    }

    while (temp != NULL && temp->tag != tag)
    {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return;

    prev->next = temp->next;
    if(prev->next == NULL)
	{
		l->tail = prev;
    }
    free(temp);
}

void print_lru(lru_list *l)
{
if(verbose == true){
    if(l == NULL)
    {
        printf("lru null\n");
        return;
    }
    if(l->head == NULL)
    {
        printf("lru empty\n");
        return;
    }
    lru_node *temp = l->head;
	int count = 0;
    while(temp!=NULL)
    {
        printf("lru[%d] tag : %ld\n",count, temp->tag);
        temp = temp->next;
		count++;
    }
}
}

unsigned get_mask(unsigned start, unsigned end)
{
   unsigned mask = 0;
   for (unsigned i = start; i < end; i++)
       mask = mask | (1 << i);

   return mask;
}

typedef struct {
    int valid;
    long tag;
} line_t;

typedef struct {
    lru_list *lru;
    line_t *linearray;
} set_t;

typedef struct {
    set_t *setarray;
    int num_sets;
    int num_lines;
} cache_t;

cache_t *cache;
int hits = 0;
int misses = 0;
int evictions = 0;


void print_cache(cache_t *c)
{
	printf("\n");
	printf("Cache | sets: %d, lines: %d\n",c->num_sets,c->num_lines);
    for(int s = 0; s < c->num_sets; s++)
	{
		printf("set[%d]\n",s);
		printf("    LRU:\n");
		set_t *set = &(c->setarray[s]);
        print_lru(set->lru);
		for(int l = 0; l < c->num_lines; l++)
		{
			line_t *line = &(set->linearray[l]);
            printf("    line[%d] | valid: %d, tag: %ld\n",l,line->valid,line->tag);
		}
	}
	printf("\n");
}
void cache_load(long set_num, long tag)
{
	printf("cache_load | set: %ld tag: %ld\n",set_num,tag);
    bool hit = false;
    set_t *cur_set = &(cache->setarray[set_num]);
    for(int i = 0; i < cache->num_lines; i++)
    {
        line_t *cur_line = &(cur_set->linearray[i]);
        printf("searching for tag %ld at set[%ld], line[%d]\n", tag, set_num, i);
        if(cur_line->valid == 1 && cur_line->tag == tag)
        {
            if(verbose == true) {fprintf(stderr, "    hit\n");}
			hits++;
            hit = true;
            delete_node(cur_set->lru, tag);
            enqueue(cur_set->lru, tag);
            printf("    deleted %ld, added %ld\n",tag,tag);
            print_cache(cache);
            }
    }
    if(hit == false)
    {
        bool put = false;
        misses++;
        if(verbose == true) {fprintf(stderr, "    miss\n");}
        for(int i = 0; i < cache->num_lines; i++)
        {
			printf("searching for valid 0 at set[%ld], line[%d]\n",set_num,i);
            line_t *cur_line = &(cur_set->linearray[i]);
            if(cur_line->valid == 0 && put == false)
            {
                cur_line->valid = 1;
                cur_line->tag = tag;
                put = true;
                enqueue(cur_set->lru, tag);
				printf("    set set[%ld], line[%d] to tag %ld\n",set_num,i,tag);
				print_cache(cache);
            }
        }
        if(put == false)
        {
            bool evicted = false;
            evictions++;
            if(verbose == true) {fprintf(stderr, "    evicting ");}
            for(int i = 0; i < cache->num_lines; i++)
            {
                line_t *cur_line = &(cur_set->linearray[i]);
                //print_lru(cur_set->lru);
                //printf("\n tag: %ld\n",cur_set->lru->head->tag);
                if(cur_line->tag == cur_set->lru->head->tag && evicted == false)
                {
                    printf("set[%ld], line[%d]",set_num, i);
                    evicted = true;
                    cur_line->valid = 1;
                    cur_line->tag = tag;
                    dequeue(cur_set->lru);
                    enqueue(cur_set->lru, tag);
                }
            }
        }
    }
printf("\n");
}

line_t *new_linearray(int lines)
{
	line_t *new_array = (line_t*)malloc(sizeof(line_t) * lines);
	for(int l = 0; l < lines; l++)
	{
		line_t *line = &(new_array[l]);
		line->tag = 0;
		line->valid = 0;		
	}
	return new_array;
}

set_t *new_setarray(int sets, int lines)
{
    set_t *new_array = (set_t*)malloc(sizeof(set_t) * sets);
    for(int s = 0; s < sets; s++)
	{
        set_t *set = &(new_array[s]);
        set->lru = create_lru();
        set->linearray = new_linearray(lines);
	}
	return new_array;
}

cache_t *new_cache(int sets, int lines)
{
	cache_t *c = malloc(sizeof(cache_t));
	c->num_sets = sets;
	c->num_lines = lines;
	c->setarray = new_setarray(sets, lines);
	return c;
}

int main(int argc, char *argv[])
{
    char *schar = NULL;
    char *echar = NULL;
    char *bchar = NULL;
    char *tchar = NULL;

    int svalue = 0;
    int evalue = 0;
    int bvalue = 0;

    char ch;
    while ((ch = getopt(argc, argv, "vs:E:b:t:")) != -1)
    {
        switch (ch) {
            case 's':
                schar = optarg;
                break;
            case 'E':
                echar = optarg;
                break;
            case 'b':
                bchar = optarg;
                break;
            case 't':
                tchar = optarg;
                break;
            case 'v':
                verbose = true;
        	    break;
            default:
                fprintf(stderr, "Unrecognized character, %c\n", optopt);
                break;
        }
    }

    svalue = atoi(schar);
    evalue = atoi(echar);
    bvalue = atoi(bchar);
   
 
	cache = new_cache(pow(2, svalue), evalue);
	print_cache(cache);


    FILE* file = fopen(tchar, "r"); /* should check the result */
    char line[256];
    char instr;
    char div_char[20];
    while (fgets(line, sizeof(line), file)) {
         sscanf(line, " %c %s", &instr, div_char);

        char* addr_char = strtok(div_char, ",");
        long addr_long = strtol(addr_char, NULL, 16);
        long set_num = addr_long & get_mask(bvalue, bvalue + svalue);
        set_num = set_num >> bvalue;
        long tag = addr_long & (~(get_mask(0,bvalue + svalue)));
        tag = tag >> (bvalue + svalue);

        printf("%s",line);
        switch(instr){
            case 'L':
                cache_load(set_num, tag);
                break;
            case 'M':
                cache_load(set_num, tag);
                cache_load(set_num, tag);
                break;
            case 'S':
                cache_load(set_num, tag);
                break;
            default:
                break;
        }
    }
    fclose(file);

    printSummary(hits, misses, evictions);
    return 0;
}
