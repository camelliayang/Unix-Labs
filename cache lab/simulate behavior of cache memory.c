//Name:Lu Yang
//Andrew Id:luyang
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include "cachelab.h"

/*
	Structures:
*/
//Contains cache summary for printout 
struct Summary {
	int hits;
	int misses;
	int evictions;
};
typedef struct Summary Summary;

//Flag container to know how to execute the program.
struct OptionFlags {
	int help;
	int verbose;
	int nsets;
	int nsetf;
	int assoc;
	int bbits;
	int bbitf;
	char * tracefile;
};
typedef struct OptionFlags OptionFlags;

//During TraceInput parsing TraceLine Container will maintain data per line.
//Input Format: [space]operation address,size
//Output Format: [space]operation address,size Hit* Miss* Eviction* (*-optional)
struct TraceLine {
	char operation;
	char address[256];
	int size;
	int hit;
	int miss;
	int eviction;
	int ignore;
};
typedef struct TraceLine TraceLine;

//TraceInputEnvironment container: will contain all runtime variables so we can manage memory if we need to exit.
struct TraceInputEnv { 
	FILE * tracefilehandle;
	TraceLine * tracelines;
	OptionFlags * flags;
	int ** cache;
	int ** _cache;
	int currentID;
	int tracelinescount;

};
typedef struct TraceInputEnv TraceInputEnv;


/*
	Function Prototypes:
*/

int parseOptions ( int , char ** , OptionFlags* );
int validateOptions ( OptionFlags* );
void printOptions ( OptionFlags* );
void resetOptions ( OptionFlags* );

void printHelp ( );

void resetSummary ( Summary* );
void L_printSummary ( Summary* );

void parseTraceInput ( OptionFlags* , Summary* );
void resetTraceInputEnvironment ( TraceInputEnv* );

int main ( int argc , char ** argv ) {

	//Initialize variables
	OptionFlags flags; 
	Summary summary;

	//Reset Summary information
	resetSummary ( &summary );

	//Parse Options
	if ( parseOptions ( argc , argv , &flags ) < 0 ) {
		printf ( "Unable to parse options\n" );
		printHelp ( );
		return 0;
	} 

	//Print Options
	//printOptions ( &flags );

	//Check for help flag
	if ( flags.help ) {
		printHelp ( );
		return 0;
	} 

	//Validate Options
	if ( validateOptions ( &flags ) < 0 ) {
		printf ( "Unable to validate option values\n" );
		printHelp ( );
		return 0;
	}

	//manageAction
	parseTraceInput ( &flags , &summary );

	//Print out hits, misses, evictions
	L_printSummary ( &summary );

	//Clean up
	resetOptions ( &flags );

    return 0;
}

/*
	Function Declarations:
*/

/*
	resetSummary:
		Reset summary values using this function
*/
void resetSummary ( Summary * summ ) {
	summ->hits = 0;
	summ->misses = 0;
	summ->evictions = 0;
}

/*
	L_printSummary:
		Local representation of the printSummary so it will print out the summary object, which contains the total amount of hits, misses, and evictions.
*/

void L_printSummary ( Summary * summ ) {
    printSummary ( summ->hits, summ->misses, summ->evictions );
}

/*
	parseOptions:
		Parse incoming command line options which are provided by main argc, argv
	
	Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
		-h: Optional help flag that prints usage info
		-v: Optional verbose flag that displays trace info
		-s <s>: Number of set index bits (S = 2s is the number of sets)
		-E <E>: Associativity (number of lines per set)
		-b <b>: Number of block bits (B = 2b is the block size)
		-t <tracefile>: Name of the valgrind trace to replay
*/
int parseOptions ( int argc , char ** argv , OptionFlags * flags ) {
	char c, i;

	//Reset all values in flags
	resetOptions ( flags );

	//Get all options from command line
	//hvsEbt defines options for getopt
	while ((c = getopt (argc, argv, "hvs:E:b:t:")) != -1) {
		switch (c)
		{
			case 'h':
				flags->help = 1;
			break;
			case 'v':
				flags->verbose = 1;
			break;
			case 's':
				flags->nsets = atoi ( optarg );
				for ( i = 1 ; i < flags->nsets ; i ++ ) {
					flags->nsetf = ( flags->nsetf << 1 ) | 1;
				}
			break;
			case 'E':
				flags->assoc = atoi ( optarg );
			break;
			case 'b':
				flags->bbits = atoi ( optarg );
				for ( i = 1 ; i < flags->bbits ; i ++ ) {
					flags->bbitf = ( flags->bbitf << 1 ) | 1;
				}
			break;
			case 't':
				//Check string not empty
				if ( strlen ( optarg ) == 0 ) {
					printf ( "Error: File path not provided\n" );
					return -1;
				}

				//Allocate memory to save file path
				flags->tracefile = (char*)malloc ( strlen ( optarg ) + 1 );

				//Check malloc for NULL pointer
				if ( !flags->tracefile ) {
					return -1;
				}

				//Copy string from optarg
				strcpy ( flags->tracefile , optarg );

			break;
		}
	}
	return 0;
}

/*
	validateOptions: 
		Validate options when help flag isn't turned on to make sure file exists and values are greater than zero.
*/
int validateOptions ( OptionFlags * flags ) {
	//Check file first no purpose checking the other values if there's not file input.
	//Attempt an fopen to tracefile
	FILE * handle = fopen ( flags->tracefile , "r" );

	//Check if handle is not NULL
	if ( !handle ) {
		printf ( "%s not found\n" , flags->tracefile );
		return 0;
	}

	//If opened succesfully close it.
	fclose ( handle );

	//Check s option
	if ( flags->nsets <= 0 ) {
		printf ( "-s needs a value larger than 0\n" );
		return -1;
	}

	//Check E option
	if ( flags->assoc <= 0 ) {
		printf ( "-E needs a value larger than 0\n" );
		return -1;
	}

	//Check b option
	if ( flags->bbits <= 0 ) {
		printf ( "-b needs a value larger than 0\n" );
		return -1;
	}

	//EVERYTHING'S OK!
	return 0;
}

/*
	printOptions:
		Print out each flag.
*/

void printOptions ( OptionFlags * flags ) {
	printf ( "Printing Options:\n" );
	printf ( "\thelp: %d\n" , flags->help );
	printf ( "\tverbose: %d\n" , flags->verbose );
	printf ( "\ts: %d\n" , flags->nsets );
	printf ( "\tE: %d\n" , flags->assoc );
	printf ( "\tb: %d\n" , flags->bbits );
	printf ( "\tt: %s\n" , flags->tracefile );
}

/*
	resetOptions:
		Reset all options and free if tracefile has allocated memory.
*/
void resetOptions ( OptionFlags * flags ) {
	//Set all integer values to zero
	flags->help = 0;
	flags->verbose = 0;
	flags->nsets = 0;
	flags->nsetf = 1;
	flags->assoc = 0;
	flags->bbits = 0;
	flags->bbitf = 1;

	//Check for allocated memory
	//free ( flags->tracefile );
}

/*
	printHelp:
		Print out usage information for this command line program, so someone knows how to use it.
	Complete Help:
	Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
		-h: Optional help flag that prints usage info
		-v: Optional verbose flag that displays trace info
		-s <s>: Number of set index bits (S = 2^s is the number of sets)
		-E <E>: Associativity (number of lines per set)
		-b <b>: Number of block bits (B = 2^b is the block size)
		-t <tracefile>: Name of the valgrind trace to replay
*/
void printHelp ( ) {
	printf ( "Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n" );
	printf ( "\t-h: Optional help flag that prints usage info\n" );
	printf ( "\t-v: Optional verbose flag that displays trace info\n" );
	printf ( "\t-s <s>: Number of set index bits (S = 2s is the number of sets)\n" );
	printf ( "\t-E <E>: Associativity (number of lines per set)\n" );
	printf ( "\t-b <b>: Number of block bits (B = 2b is the block size)\n" );
	printf ( "\t-t <tracefile>: Name of the valgrind trace to replay\n" );
}

/*
	parseTraceLineCount:
		Read in how many lines are inside trace file.
*/
int parseTraceLineCount ( TraceInputEnv * env ) {
	int c , lines = 0;
	while ( ( c = fgetc ( env->tracefilehandle ) ) != EOF ) {
		if ( c == '\n' ) lines++;
	}	 
	rewind ( env->tracefilehandle );
	return lines;
}


/*
	parseTraceLine:
		Reads handle to locate current line type.
		Input Format: [space]operation address,size
*/

int parseTraceLine ( TraceInputEnv * env , TraceLine * line , Summary * summary ) {
	int c = 0, i = 0;
	char num[256];

	//Set ignore to default 0
	line->ignore = 0;

	//Read in operation
	while ( ( c = fgetc ( env->tracefilehandle ) ) != EOF ) {
		if ( c == ' ' ) continue;
		line->operation = c;
		break;
	}

	//Read in address
	//i - counter
	//zero out address 
	memset ( line->address , 0x0 , sizeof ( line->address ) );
	while ( ( c = fgetc ( env->tracefilehandle ) ) != EOF ) {
		if ( c == ' ' ) continue;
		if ( c == ',' ) break;
		line->address[i++] = c;
	}

	//Read in size
	i = 0; //reset counter
	memset ( num , 0x0 , sizeof (num) ); //reset num
	while ( ( c = fgetc ( env->tracefilehandle ) ) != EOF ) {
		if ( c == '\n' ) break;
		num[i++] = c;
	}
	//Convert char num[] to int
	line->size = atoi ( num );

	//Check for "I" and set ignore flag
	if ( line->operation == 'I' ) {
		line->ignore = 1;
		return 0;
	}

	//Convert address to decimal integer
	//hex -> dec
	long dec_address = strtol ( line->address , NULL , 16 );

 	//Mask out block bits
	//int bits = dec_address & env->flags->bbitf;

	//Mask out set 
	int index = ((int)( dec_address >> env->flags->bbits )) & env->flags->nsetf;

	//Mask out tag
	int tag = dec_address >> (env->flags->nsets + env->flags->bbits);

	//Check for M which loads and reads

	line->hit = 0;
	line->miss = 0;
	line->eviction = 0;

	/*
	 *	| tag | index | offset |
	 *	|     |   0   |  data  | 
	 *	------------------------
	 **/

	for ( i = 0 ; i < env->flags->assoc ; i ++ ) {
	 	if ( env->_cache[index][i] == tag ) {

			//  index - same for any address
			//  i - position based off associativity
			//  tag - check if it has the same value or not. //might need to check if bits are the same 

	 		line->hit += 1;
	 		summary->hits += 1;

	 		env->cache[index][i] = env->currentID++;

	 		if ( line->operation == 'M' ) {
	 			line->hit += 1;
	 			summary->hits += 1;
	 		}
	 		break;
		} else
		if ( env->_cache[index][i] == -1 ) {
			line->miss += 1;
			summary->misses += 1;


	 		env->_cache[index][i] = tag;
	 		env->cache[index][i] = env->currentID++;//sudo-timer
	 		break;
		} else 
		if ( i == (env->flags->assoc - 1) ) {
			int smallest = i;
		 	for ( c = 0 ; c < env->flags->assoc ; c++ ) {
		 		if ( env->cache[index][c] < env->cache[index][smallest] ) {
		 			smallest = c;
		 		}
		 	}

		 	line->miss += 1;
		 	summary->misses += 1;

		 	line->eviction += 1;
		 	summary->evictions += 1;

		 	if ( line->operation == 'M' ) {
		 		line->hit += 1;
		 		summary->hits += 1;
		 	}

		 	env->_cache[index][smallest] = tag;
		 	env->cache[index][smallest] = env->currentID++;
			break;
		}
	}

	return 0;
}
/*
	printTraceLines:
		Print out information for a TraceLine
		Output Format: [space]operation address,size Hit* Miss* Eviction* (*-optional)
*/

void printTraceLine ( TraceLine * line ) {
	int i;

	printf ( "%c %s,%d", 
				line->operation, 
				line->address, 
				line->size );

	for ( i = 0 ; i < line->miss ; i ++ ) {
		printf ( "%s", line->miss? " miss":"" );
	}

	for ( i = 0 ; i < line->eviction ; i ++ ) {
		printf ( "%s", line->eviction? " eviction":"" );
	}

	for ( i = 0 ; i < line->hit ; i ++ ) {
		printf ( "%s", line->hit? " hit":"" ); 
	}

	printf ( "\n" );
}
//hits:257595 misses:29369 evictions:29365

/*
	parseTraceInput: 
		Takes in flag input and parses a tracefile based off given flags and input file.
*/	
void parseTraceInput ( OptionFlags * flags , Summary * summary ) {
	//Envrionment variables container
	TraceInputEnv env;
	int i, k;

	//Set env flags
	env.flags = flags;
	env.currentID = 0;
	//Create cache sim matrix
	env.cache = (int**)malloc ( sizeof(int*) *  pow ( 2 , env.flags->nsets ) );
	for ( i = 0 ; i <  pow ( 2 , env.flags->nsets ) ; i++ ) {
		env.cache[i] = (int*)malloc ( sizeof(int) * pow ( 2 , env.flags->assoc) );
		for ( k = 0 ; k < env.flags->assoc ; k ++ ) {
			env.cache[i][k] = -1;
		}
	}

	//Create cache sim matrix
	env._cache = (int**)malloc ( sizeof(int*) * pow ( 2 , env.flags->nsets ) );
	for ( i = 0 ; i < pow ( 2, env.flags->nsets ) ; i++ ) {
		env._cache[i] = (int*)malloc ( sizeof(int) * env.flags->assoc );
		for ( k = 0 ; k < env.flags->assoc ; k ++ ) {
			env._cache[i][k] = -1;
		}
	}

	//Open tracefile
	env.tracefilehandle = fopen ( flags->tracefile , "r+" );

	//Count lines
	env.tracelinescount = parseTraceLineCount ( &env );

	//Allocate memory to contain perline data
	env.tracelines = (TraceLine*)malloc ( sizeof(TraceLine) * env.tracelinescount );

	//Parse each line
	for ( i = 0 ; i < env.tracelinescount ; i ++ ) {
		//Parse line
		parseTraceLine ( &env , &env.tracelines[i] , summary );

		//Print line if verbose flag is set
		if ( flags->verbose && !env.tracelines[i].ignore ) {
			printTraceLine ( &env.tracelines[i] );
		}
	}

	//Print Line Count
	//printf ( "Lines: %d\n" , env.tracelinescount );

	//Free, close, zero-out environment variables
	resetTraceInputEnvironment ( &env );
}

/*
	resetTraceInputEnvironment: (THIS IS AN IDEA ON HOW TO MANAGE MEMORY IN C BETTER)
		Clears all allocated, opened, used variables during parseTraceInput function.
*/
void resetTraceInputEnvironment ( TraceInputEnv * env ) {
	//Close if open.
	if ( env->tracefilehandle ) {
		fclose ( env->tracefilehandle );
	}

	//Free traceline data if available
	if ( env->tracelines ) {
		free ( env->tracelines );
	}

	if ( env->cache ) {
		//free ( env->cache );
	}

}
