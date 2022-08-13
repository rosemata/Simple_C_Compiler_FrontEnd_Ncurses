#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ncurses.h>
#include <curses.h>
#include <dirent.h>

#include"dsh.h"

#define WIN_HEIGHT 40
#define WIN_WIDTH 65
#define WIN1_START_X 10
#define WIN2_START_X (WIN1_START_X + WIN_WIDTH + 20)
#define WIN_START_Y 10

// returns file name with file number and session folder
char* getFileName(int file_number, int session_folder, char str[], char err_out[], int flag) {

	str[0] = '\0';
	char filename1[100];
	snprintf(filename1,100,"%s/.dsh/", getenv("HOME"));
	char str1[5];
	sprintf(str1, "%d", session_folder);
	strcat(filename1,str1);
	char ch = '/';
	strncat(filename1, &ch, 1);
	sprintf(str1, "%d", file_number);
	strcat(filename1,str1);
	strcat(filename1,err_out);
	strcat(str,filename1);
	filename1[0] = '\0';
	str1[0] = '\0';
	wrefresh(stdscr);
	return str;
}

// print the result in the output screen
void result(int file_num, int session_folder, char str[100]) {

	int flag = 0;

    FILE *fptr;
	FILE *fptr2;

	int input_y  = 1;
	int input_x  = 1;
    char c, d;
    fptr = fopen(str, "r");
	c = fgetc(fptr);

	while (c != EOF) {
		if (input_x == WIN_WIDTH - 1){
			input_x = 1;
			input_y++;
		}	
		mvwprintw(stdscr, WIN_START_Y + input_y, WIN2_START_X + input_x, "%c", c);
		wrefresh(stdscr);
		c = fgetc(fptr);
		input_x++;
		flag++;
    }

    fclose(fptr);

	if (flag == 0) {
		input_y  = 1;
		input_x  = 1;

		char filename[100];
		getFileName(file_num, session_folder, filename, ".stderr", 0);

		fptr2 = fopen(filename, "r");
		d = fgetc(fptr2);
		while (d != EOF) {
		if (input_x == WIN_WIDTH - 1){
			input_x = 1;
			input_y++;
		}	
		mvwprintw(stdscr, WIN_START_Y + input_y, WIN2_START_X + input_x, "%c", d);
		wrefresh(stdscr);
		d = fgetc(fptr2);
		input_x++;
		flag++;
		}
		fclose(fptr2);
    }
}

// gievs the file number
int file_number(int session_number) {
    DIR *folder;
    struct dirent *entry;
    int num_files = 0;

	char folder2[100];
	snprintf(folder2,100,"%s/.dsh/", getenv("HOME"));

    // char filename[100] = "/AD-home/rmata3/.dsh/";
    char str[100];
    sprintf(str, "%d", session_number);
    strcat(folder2,str);

    // use session folder numebr here
    folder = opendir(folder2);

    if(folder == NULL)
    {
        perror("Unable to read directory");
        return(1);
    }
    while( (entry=readdir(folder)) )
    {
        num_files++;
    }
    closedir(folder);
    return ((num_files-2)/2)-1;
}


// gives the cur session folder number
int count_session_folder() {
    DIR *folder;
    struct dirent *entry;
    int num_files = 0;

	char folder2[100];
	snprintf(folder2,100,"%s/.dsh/", getenv("HOME"));

    folder = opendir(folder2);
    if(folder == NULL)
    {
        perror("Unable to read directory");
        return(1);
    }
    while( (entry=readdir(folder)) )
    {
        num_files++;
    }
    closedir(folder);
    return num_files;
}


// does the highlight whenever the arrow up is moved
int arrow_up(WINDOW* output, int array[50], int x, int file_num, int session_folder) {

	// makes it so we can use arrow keys
	keypad(output, true);
	int choice;
	int highlight = 0;
	int count = 0;

	while(1) {

		for(int i = 0; i < x-1; i++) {
			if(i == highlight) {
				wattron(output, A_REVERSE);
			}
			mvwprintw(output, i+1, 1, "%d.", i);
			
			wrefresh(stdscr);
			wattroff(output, A_REVERSE);
		}

		choice = wgetch(output);

		switch(choice) {
			case KEY_UP:
				highlight--;
				count--;
				
				if (highlight == -1) {
					highlight = 0;
					count = 0;
				}
				break;
			case KEY_DOWN:
				highlight++;
				count++;
				if (highlight == x-1) {
					return 0;
				}
				break;
			default:
				break;
		}

		// if user hit enter, runs the command of current file number
		if(choice == 10) { 
			char temp_name[100];
			getFileName(array[count], session_folder, temp_name, ".stdout", 1);
			result(array[count], session_folder, temp_name);
			wrefresh(stdscr);
			temp_name[0] = '\0';
			wrefresh(stdscr);
		}
	}
}


int main(int argc, char**argv){

    dsh_init();

	int session_folder = count_session_folder()-3;

	/* Initialize the screen */
	initscr();

	/* Make sure that Ctrl+C will exit the program */
	cbreak();

	/* Disable default key press echo */
	noecho();

	// create windor 1 for history
	WINDOW *history_commands = newwin(WIN_HEIGHT-13, WIN_WIDTH, WIN_START_Y, WIN1_START_X);
	// create window 2 where user types
	WINDOW *input = newwin(3, WIN_WIDTH, WIN_START_Y + WIN_HEIGHT - 13, WIN1_START_X);
	// create window 3 where we show list of commands
	WINDOW *output = newwin(WIN_HEIGHT-10, WIN_WIDTH, WIN_START_Y, WIN2_START_X);
	
	refresh();

	// create boxes for each window
	box(history_commands,0,0);
	wrefresh(history_commands);
	box(input,0,0);
	wrefresh(input);
	box(output,0,0);
	wrefresh(output);

	// store each input
	int inputChar = 0;
	int input_y  = 1;	// y var
	int input_x  = 6;	// x var
	
	int count = 0;
	char prompt[] = "dsh> ";

	int temp = -1;

	int array[50]; // store number of files
	int num_input_commands = 1;


	// loop every time user hit enter
	while(1) {
		// prompt
		mvwprintw(input, 1,1,"%s", prompt);
		
		// move the cursor to original position
		wmove(input, 1, 6);

		// refresh the screen, every time the cursor is moved
		wrefresh(input);
		
		// asking user for input
		inputChar = wgetch(input);

		// char array to store letters
		char str[100] = " ";
		char prev[100] = "";
		int num = 0;

		keypad(input, true);


		// loop until user hit enter
		while(inputChar != '\n') {

			// make sure input does not pass the line
			if (input_x == WIN_WIDTH - 1){
				input_x = 1;
				input_y++;
			}

			if(inputChar == 127) {

				if (input_x == 6) {
					// continue;
					wmove(input, 1, 6);
					// mvwprintw(input,input_y,input_x," ");
					wrefresh(input);
				}

				else {
					input_x--;
					num--;
					mvwprintw(input,input_y,input_x," ");
					str[num] = '\0';
					wrefresh(input);
				}
			}

			else if (inputChar == KEY_DOWN) {
				wrefresh(input); 
			}
			
			else if (inputChar == KEY_LEFT) {
				input_x--;
				wmove(input, input_y, input_x);
				wrefresh(input);
			}
			
			else if (inputChar == KEY_RIGHT) {
				str[num] = ' ';
				input_x++;
				num++;
				
				wmove(input, input_y, input_x);
				wrefresh(input); 
			}

			else if (inputChar == KEY_UP) {
				// mvwprintw(stdscr, WIN_START_Y + 2 + count , WIN2_START_X + 1, "File Name %d", 1);
				if(count != 0) {				
					arrow_up(history_commands, array, num_input_commands, temp, session_folder);
				// wrefresh(stdscr);
					wrefresh(input); 
				}
			}
			
			else if (inputChar == KEY_BACKSPACE) {
				// mvwprintw(stdscr, WIN_START_Y + 2 + count , WIN2_START_X + 1, "File Name %d", 1);
				
				// wrefresh(stdscr);
				input_x--;
				num--;
				str[num] = '\0';
				mvwprintw(input,input_y,input_x," ");
				wmove(input, input_y, input_x);
				wrefresh(input);
				// wrefresh(input); 
			}
			
			else {	
				// store each letter in an array, we will output this later
				str[num] = inputChar;
				num++;

				mvwprintw(input,input_y,input_x,"%c",inputChar);
				wrefresh(input);
				input_x++;
			}

			inputChar = wgetch(input);
			
		}

		// number of input commands
		num_input_commands++;

		wrefresh(input);

		// prints in the command history
		mvwprintw(stdscr, WIN_START_Y + count + 1, WIN1_START_X + 1, "%d. %s", count, str);
		
		str[num] = 'x';
		wrefresh(stdscr);

		// reset screen, position and box layout
		wmove(input, 1, 6);
		wclear(input);
		input_y  = 1;
		input_x  = 6;
		box(input,0,0);

		// run in the .dsh file
        dsh_run(str);

		// returns file number
		int file_num = file_number(session_folder);
		// keep track file number
		temp = file_num;
		
		// returns the full directory name
		// ex. 
		char filename[100];
		getFileName(file_num, session_folder, filename, ".stdout", 3);

		// run the directory
		// /AD-home/rmata3/.dsh/2/0.stdout
		result(file_num, session_folder, filename);
		array[count] = file_num;
		wrefresh(stdscr);
		filename[0] = '\0';
		count++;

		// mvwprintw(stdscr, WIN_START_Y + 2 + count, WIN2_START_X + 1, "File Name %d", array[count]);
		// mvwprintw(stdscr, WIN_START_Y + 2 + count, WIN2_START_X + 15, "File Name %d", file_num);
		// files[0] = filename;

		// mvwprintw(stdscr, WIN_START_Y + 1 + count, WIN2_START_X + 1, "input %s", array[0]);
		// mvwprintw(stdscr, WIN_START_Y + 2 + count , WIN2_START_X + 1, "File Name %d", array[count]);
		// mvwprintw(stdscr, WIN_START_Y + count, WIN2_START_X + 1, ">%s<", &str); 
	}

	endwin();

	return 0;
}
