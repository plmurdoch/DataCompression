/* uvcompress.c
   CSC 485B/CSC 578B
   
   Placeholder starter code for UVCompress 
   Provided by: B. Bird - 2023-05-01
   
   Student Name: Payton Murdoch
   V#:V00904677
   Due Date: 2023-05-23
*/
#include <stdio.h>
#include <math.h>
#include <string.h>

#define INITIAL_ENTRY = 256
#define FIRST_ENTRY = 257
#define BASE = 2
#define INITIAL_BIT = 9
#define MAX_BIT = 16
#define INPUT_SIZE = 10

typedef struct{
	char* entry;
	int index;
}ITEntry;

void InitIndexTable(ITEntry* I_T,int size);
void DeallocIndexTable(ITEntry* I_T, int size);
void GrowTable(ITEntry* I_T,int bits);
void NewEntry(ITEntry* I_T, char* new_entry, int indx, int size);
int Comparison(ITEntry* I_T, char* cmpstr,int size);
void Encode(int next_symbol, int bits, char* str, ITEntry* I_T);
void ResetStr(char* str);



/*
If combination of char and char +1 in the dictionary output index number
else add combination to the dictionary with the next available index number
*/

//what to do, create index structure so we know how to add elements to it
int main(){
	int init_size = INITIAL_ENTRY;
	int curr_bits = INITIAL_BIT;
	int add_index = FIRST_ENTRY;
	ITEntry* Index_Table = NULL;
	InitIndexTable(Index_Table, init_size);
	char next_character = '';
	char input_string[INPUT_SIZE];
	
    while((next_character = fgetc(stdin)) != EOF){
		strncat(input_string,next_character, 1);
		if (strlen(input_string) == INPUT_SIZE){
			Encode(add_index, curr_bits, input_string, Index_Table);
			ResetStr(input_string);
		}
	}
	if (strlen(input_string) != 0){
		Encode(add_index, curr_bits, input_string, Index_Table);
	}
}
//Step 3: construct LZW algorithm (Utilize Sudocode)
//Step 4: piping contents out to command line (Reverse char bit order, package all 8 bits, reverse 8 bits, output) Note: IF EOF found last bits packed with 0s till 8 bits)
void Encode(int next_symbol, int bits, char* str, ITEntry* I_T){
	char* working = "";
	
}


void InitIndexTable(ITEntry* I_T, int size){
	I_T = (ITEntry*)malloc(INITIAL_MAX *sizeof(ITEntry));
	for(int i = 0; i <size; i++){
		I_T[i].entry = (char*)malloc(2*sizeof(char));
		I_T[i].entry[0] = i;
		I_T[i].entry[1] = '\0';
		I_T[i].index = i;
	}
	I_T[size].index = size;
	I_T[size].entry =(char*)malloc(sizeof(char));
	I_T[size].entry = '\0';
}


void DeallocIndexTable(ITEntry* I_T, int size){
	for(int i = 0; i < size; i++){
		free(I_T[i].entry);
	}
	free(I_T);
}


void GrowTable(ITEntry* I_T, int bits){
	I_T = (ITEntry*)realloc(I_T, pow(BASE,bits)*sizeof(ITEntry));
}


void NewEntry(ITEntry* I_T, char* new_entry, int indx, int size){
	I_T[indx].entry = (char*)malloc(strlen(new_entry)*sizeof(char));
	I_T[indx].entry = new_entry;
	I_T[indx].indx = indx;
	indx++;
	size++;
}


int Comparison(ITEntry* I_T, char* cmpstr, int size){
	for (int i = 0; i<size; i++){
		if(i == 256){
			//Pass entry
		}
		else if (strcmp(I_T[i].entry, cmpstr) ==0){
			return i;
		}
	}
	return 0
}


void ResetStr(char *str){
	char none_string[INPUT_SIZE] = "";
	strncpy(str, none_string,INPUT_SIZE)	
}


