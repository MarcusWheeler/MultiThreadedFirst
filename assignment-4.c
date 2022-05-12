/******************************************************************************

Online C Compiler.
Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#define MAX_LINE_AMOUNT 50
#define MAX_BUFFER_SIZE 1000

char first_buffer[MAX_LINE_AMOUNT][MAX_BUFFER_SIZE];	//This is going to be my first buffer. The input thread will produce for it and the newline thread will consume it
int first_unprocessed_count = 0;
int first_producer_index = 0;
int first_consumer_index = 0;
pthread_mutex_t first_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t first_full = PTHREAD_COND_INITIALIZER;


char second_buffer[MAX_LINE_AMOUNT][MAX_BUFFER_SIZE];	//This is going to be my second buffer. Newline thread produces for it and plus sign thread consumes it
int second_buffer_count = 0;
int second_producer_index = 0;
int second_consumer_index = 0;
pthread_mutex_t second_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t second_full = PTHREAD_COND_INITIALIZER;


char third_buffer[MAX_LINE_AMOUNT][MAX_BUFFER_SIZE];	//Plus sign thread produces for it and the output thread consumes and posts it.
int third_buffer_count = 0;
int third_buffer_character_count = 0;
int third_producer_index = 0;
int third_consumer_index = 0;
pthread_mutex_t third_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t third_full = PTHREAD_COND_INITIALIZER;


//Now I can create the actual bones - I'll have four main functions and then I'll follow the helper function example given

void *get_input (void *args);
void put_buffer_one (char *temp);
void get_user_input (char *temp);


void *process_newlines (void *args);
void put_buffer_two (char *temp);
void get_buffer_one (char *temp);


void *process_plus (void *args);
void put_buffer_three ();
void get_buffer_two ();
char *replace_plus_pairs (char *temp);

void *write_output (void *args);
void get_buffer_three ();






int
main ()
{
  /*for(int i = 0; i < 5 && !stopped; ++i){
    char *temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
    get_user_input (temp);
    put_buffer_one (temp);
    if(strcmp(temp, "STOP\n") == 0) stopped = true;
    printf("buffer_one: %s\n", first_buffer[i]);
    get_buffer_one(temp);
    temp[strlen(temp) - 1] = '!';
    put_buffer_two(temp);
    printf("buffer_two: %s\n", second_buffer[i]);
    get_buffer_two(temp);
    
    temp = replace_plus_pairs(temp);
    printf("temp: %s\n", temp);
    put_buffer_three(temp);
    printf("third_buffer: %s\n",  third_buffer[i]);
    
    get_buffer_three(temp);
    deal_with_output(temp);
    free (temp);
  } */
  
  //Attempt at the actual thread stuff now
  pthread_t input_thread, newline_thread, plus_thread, output_thread;
  // Create the threads
  pthread_create (&input_thread, NULL, get_input, NULL);
  pthread_create (&newline_thread, NULL, process_newlines, NULL);
  pthread_create (&plus_thread, NULL, process_plus, NULL);
  pthread_create (&output_thread, NULL, write_output, NULL);
  // Wait for the threads to terminate
  pthread_join (input_thread, NULL);
  pthread_join (newline_thread, NULL);
  pthread_join (plus_thread, NULL);
  pthread_join (output_thread, NULL);
  return 0;
}

void *
get_input (void *args)
{
  bool input_stop = false;
  for (int i = 0; i < MAX_LINE_AMOUNT && !input_stop; ++i)
  {
    char *temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
    get_user_input (temp);
    if (strcmp (temp, "STOP\n") == 0) input_stop = true;
    put_buffer_one (temp);
    free (temp);
  }
  return NULL;
}

void
put_buffer_one (char *temp)
{
  pthread_mutex_lock (&first_mutex);	//Lock the first mutex so I can put something in in peace.
  strcpy (first_buffer[first_producer_index], temp);	//This is simply copying over my string to the buffer
  first_unprocessed_count++;
  first_producer_index++;	//And incrementing where I'm going to do it next time.
  pthread_cond_signal (&first_full);	//There's something in the buffer
  pthread_mutex_unlock (&first_mutex);	//Unlock the buffer for the next thread
}

void
get_user_input (char *temp)
{
  bool cleared = false;
  size_t size = sizeof (char) * MAX_BUFFER_SIZE;
  while(!cleared){
    getline (&temp, &size, stdin);
    for(int i = 0; i < strlen(temp); ++i){
      if(isspace(temp[i]) == 0 && temp[i] != '\0'){
        cleared = true;
      }
    }
    
  }
}

void *
process_newlines (void *args)
{
  bool newline_stop = false;
  for (int i = 0; i < MAX_LINE_AMOUNT && !newline_stop; ++i)
  {
    char *temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
    get_buffer_one (temp);	//Allocate and grab my current line to work with
    //There should be no situations where I have newlines in the middle of my string - each line is triggered by the newline
    //That means I can just get the last character that isn't null. If I need to or think of any reason to, 
    //I can make this system more robust very easily.
    temp[strlen (temp) - 1] = ' ';
    if(strcmp(temp, "STOP ") == 0) newline_stop = true;
    put_buffer_two (temp);	//Now put it all away and free up my resources
    free (temp);
  }
  return NULL;
}

void
put_buffer_two (char *temp)
{
  pthread_mutex_lock (&second_mutex);	//Lock the second so I can put something in in peace.
  strcpy (second_buffer[second_producer_index], temp);	//This is simply copying over my string to the buffer
  second_producer_index++;	//And incrementing where I'm going to do it next time.
  second_buffer_count++;
  pthread_cond_signal (&second_full);	//There's something in the buffer
  pthread_mutex_unlock (&second_mutex);	//Unlock the buffer for the next thread
}

void
get_buffer_one (char *temp)
{
  pthread_mutex_lock (&first_mutex);
  while (first_unprocessed_count == 0)
  pthread_cond_wait (&first_full, &first_mutex);
  
  //CRITICAL SECTION
  strcpy (temp, first_buffer[first_consumer_index]);
  
  first_consumer_index++;
  first_unprocessed_count--;
  pthread_mutex_unlock (&first_mutex);
}



void *
process_plus (void *args)
{
  bool plus_stop = false;
  for (int i = 0; i < MAX_LINE_AMOUNT && !plus_stop; ++i)
  {
    char *temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
    get_buffer_two (temp);
    if(strcmp(temp, "STOP ") == 0) plus_stop = true;
    temp = replace_plus_pairs (temp);
    put_buffer_three (temp);
    free (temp);
  }
  
}

char *
replace_plus_pairs (char *temp)
{
  char *processed_temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
  int processed_index = 0;
  bool plus_found = false;
  for (int i = 0; i <= strlen (temp); ++i)
  {
    if (temp[i] == '+')
    {			//Found a plus sign - have I found another beforehand?
      if (plus_found)
      {
        processed_temp[processed_index - 1] = '^';
        plus_found = false;
      }
      else
      {
        processed_temp[processed_index] = temp[i];
        plus_found = true;
        processed_index++;
      }
    }
    else
    {			//Else I haven't found anything worth mentioning
      processed_temp[processed_index] = temp[i];
      plus_found = false;
      processed_index++;
    }
  }
  free (temp);
  return processed_temp;
}

void
put_buffer_three (char *temp)
{
  pthread_mutex_lock (&third_mutex);	//Lock the second so I can put something in in peace.
  strcpy (third_buffer[third_producer_index], temp);	//This is simply copying over my string to the buffer
  third_producer_index++;	//And incrementing where I'm going to do it next time.
  third_buffer_count++;
  pthread_cond_signal (&third_full);	//There's something in the buffer
  pthread_mutex_unlock (&third_mutex);	//Unlock the buffer for the next thread
}

void
get_buffer_two (char *temp)
{
  pthread_mutex_lock (&second_mutex);
  while (second_buffer_count == 0)
  pthread_cond_wait (&second_full, &second_mutex);
  
  //CRITICAL SECTION
  strcpy (temp, second_buffer[second_consumer_index]);
  
  second_consumer_index++;
  second_buffer_count--;
  pthread_mutex_unlock (&second_mutex);
}




void *
write_output (void *args)
{
  char internal_buffer[MAX_LINE_AMOUNT][MAX_BUFFER_SIZE];
  int usable_characters = 0;
  int last_used_line = 0;
  int last_used_index = 0;
  int processed_index = 0;
  int number_of_repeats = 0;
  bool output_stop = false;
  char *processed_temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);	//Allocate a string of the correct size
  for (int i = 0; i < MAX_LINE_AMOUNT && !output_stop; ++i)
  {
    char *temp = malloc (sizeof (char) * MAX_BUFFER_SIZE);
    get_buffer_three (temp);
    if(strcmp(temp, "STOP ") != 0){
      usable_characters += strlen (temp);
      
    } 
    else{
      output_stop = true;
    }
    if(!output_stop){
      strcpy (internal_buffer[i], temp);
    }
    number_of_repeats = usable_characters / 80;
    if (number_of_repeats >= 1)
    {
      for (int i = 0; i < number_of_repeats; ++i)
      {
        
        for (int j = 0; j < 80; ++j)
        {
          if (internal_buffer[last_used_line][last_used_index] ==
          '\0')
          {		//If I've reached the end of my current line, reset me
            last_used_line++;
            last_used_index = 0;
          }
          processed_temp[processed_index] =
          internal_buffer[last_used_line][last_used_index];
          processed_index++;
          last_used_index++;
          usable_characters--;
          
        }
        
        processed_temp[processed_index++] = '\n';
        write(1 , processed_temp, sizeof(char) * 81);
        
        processed_index = 0;
      }
    }
    free (temp);
  }
  free (processed_temp);
  return NULL;
}

void
get_buffer_three (char *temp)
{
  pthread_mutex_lock (&third_mutex);
  while (third_buffer_count == 0)
  pthread_cond_wait (&third_full, &third_mutex);
  
  //CRITICAL SECTION
  strcpy (temp, third_buffer[third_consumer_index]);
  third_consumer_index++;
  third_buffer_count--;
  pthread_mutex_unlock (&third_mutex);
}

