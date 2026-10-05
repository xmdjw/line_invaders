#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <ctype.h>
#include <ncurses.h>
#define LENGTH 50

char itoa_num(int number);//itoa,but only 0~9
void sel_msleep(int ms);
void *read_aim(void *arg);

char enemies[LENGTH];
int count_e = 0;
int taken_down = 0;
int hit = 0;
char input;

pthread_mutex_t f_lock;
int fd[2];	//fd[0]: read fd[1]: write

static float energy = 0.0;

int main()
{
	//init ncurses environment
	initscr();
	noecho();
	cbreak();
	nodelay(stdscr,true);
	curs_set(0);

	//init the game
	char you[] = "0123456789";
	char screen[LENGTH] = " 0 |||                                           ";
	char get ='\0';
	int wall = 5;
	int random;
	char enemy;
	int interval = 0;

	//init the screen
	int rows,cols;
	getmaxyx(stdscr,rows,cols);
	int x;
	int y = rows / 2;
	move(y,0);
	printw("%s",screen);
	printw(" #");
	refresh();

	int i = LENGTH;
	int shoot = 0;

	//init mutex
	if (pthread_mutex_init(&f_lock,NULL) != 0)
	{
		endwin();
		printf("init mutex error\n");
		exit(0);
	}

	//init pipe and child process
	pid_t aim;

	if (pipe(fd) < 0)
	{
		endwin();
		printf("pipe error\n");
		return -1;
	}

	aim = fork();

	if (aim < 0)
	{
		endwin();
		printw("fork error\n");
		return -1;
	}
	else if (aim == 0)	//child
	{
		close(fd[0]);

		const char C = 'C';
		const char X = 'X';
		const char EASTER_EGG= '~';

		while (hit == 0)
		{
			get = getch();

			//Normal
			if (get == ' ')
			{
				pthread_mutex_lock(&f_lock);
				
				shoot++;
				if (shoot > 9)
				{
					shoot = 0;
				}
				
				write(fd[1],&you[shoot],1);
				
				pthread_mutex_unlock(&f_lock);
			}
			
			//Skill
			if (get == 'c')
			{
				pthread_mutex_lock(&f_lock);

				write(fd[1],&C,1);

				pthread_mutex_unlock(&f_lock);
			}	
			if (get == 'x')
			{
				pthread_mutex_lock(&f_lock);

				write(fd[1],&X,1);

				pthread_mutex_unlock(&f_lock);
			}	
			if (get == '~')
			{
				pthread_mutex_lock(&f_lock);
		
				write(fd[1],&EASTER_EGG,1);

				pthread_mutex_unlock(&f_lock);
			}	
		}

		exit(0);
	}
	else	//father
	{
		close(fd[1]);		

		//create pthread for read player's output
		pthread_t reciever;
		pthread_create(&reciever,NULL,read_aim,&y);

		while (hit == 0)
		{
			//print energy
			pthread_mutex_lock(&f_lock);

			move(y -2,0);
			printw("Energy: %.1f",energy);

			pthread_mutex_unlock(&f_lock);

			for (int j = 0;j < count_e;j++)
			{
				//Old enemy go ahead

				pthread_mutex_lock(&f_lock);

				x = LENGTH - (count_e - j + 1);
				move(y,x);
				printw("%c",enemies[j]);
				refresh();

				pthread_mutex_unlock(&f_lock);
			}
			
			//Create New enemy

			if (interval % 2 == 0)	//interval means jian-ge  odd->spacebar even->create
			{
				srand((unsigned)time(NULL));
				random = rand() % 10;
				if (random == 0)
				{
					random = 1;
				}
				enemy = itoa_num(random);
				enemies[count_e] = enemy;
				count_e++;
				interval++;

				//New enemy show up

				pthread_mutex_lock(&f_lock);

				move(y,LENGTH -1);
				printw("%c",enemy);
				refresh();

				pthread_mutex_unlock(&f_lock);
			}
			else
			{
				enemy = ' ';
				enemies[count_e] = ' ';
				count_e++;
				interval++;

				pthread_mutex_lock(&f_lock);
				
				move(y,LENGTH -1);
				printw("%c",enemy);
				refresh();

				pthread_mutex_unlock(&f_lock);
			}

			if (enemies[0] == ' ')
			{
				pthread_mutex_lock(&f_lock);

				//Old enemy go ahead ONLY in array,NO PRINT
				for (int j = 0;j < count_e -1;j++)//-1:exclude the last enemy
				{
					enemies[j] = enemies[j+1];
				}

				count_e--;

				pthread_mutex_unlock(&f_lock);
			}

			if (input == enemies[0])
			{
				pthread_mutex_lock(&f_lock);

				for (int j = 0;j < count_e -1;j++)//-1:exclude the last enemy
				{
					enemies[j] = enemies[j+1];
				}
				count_e--;
				taken_down++;
				energy += 0.2;

				move(y,LENGTH - count_e -1);
				printw("*");
				refresh();

				pthread_mutex_unlock(&f_lock);
			}

			//enemies hitting wall test		
	
			if (wall > 0 && LENGTH - count_e == wall)
			{
				pthread_mutex_lock(&f_lock);

				for (int j = 0;j < count_e -1;j++)//-1:exclude the last enemy
				{
					enemies[j] = enemies[j+1];
				}
				count_e--;
				taken_down++;
				move(y,wall);
				printw("*");
				refresh();
				wall--;
				if (wall <= 2)
				{
					wall = -1;
				}

				pthread_mutex_unlock(&f_lock);
			}
			
			if (LENGTH - count_e == 1)
			{
				pthread_mutex_lock(&f_lock);

				for (int j = 0;j < count_e -1;j++)//-1:exclude the last enemy
				{
					enemies[j] = enemies[j+1];
				}
				count_e--;
				move(y,1);
				printw("Wryyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy");
				refresh();
				hit = 1;
				
				pthread_mutex_unlock(&f_lock);
			}

			sel_msleep(1000);	
		}
		
		const char QUIT = '!';
		write(fd[1],&QUIT,1);//send end sign to pthread 
		sleep(3);

		curs_set(1);
		endwin();
		printf("You have made %d points!\n",taken_down);
		printf("-=LINE_INVADERS=- By xmdjw\n");

		exit(0);
	}
}

char itoa_num(int number)
{
	if (number < 0 || number >= 10){
		return 'E';
	}
	switch (number)
	{
		case 0:
			return '0';
		case 1:
			return '1';
		case 2:
			return '2';
		case 3:
			return '3';
		case 4:
			return '4';
		case 5:
			return '5';
		case 6:
			return '6';
		case 7:
			return '7';
		case 8:
			return '8';
		case 9:
			return '9';
		default:
			return 'E';
	}
}

void sel_msleep(int ms)
{
	struct timeval tv;
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms % 1000) * 1000;
	select(0,NULL,NULL,NULL,&tv);
}

void *read_aim(void *arg)
{
	int *y = (int*)arg;
	while (true)
	{
		if (read(fd[0],&input,1) > 0)
		{
			if (input == '!')
			{
				exit(0);
			}

			//Normal
			pthread_mutex_lock(&f_lock);

			move(*y,1);
			printw("%c",input);
			refresh();

			pthread_mutex_unlock(&f_lock);

			if (input == 'C' && energy >= 5)	//Skill: Micro H.I.D.: kill all enemies
			{
				pthread_mutex_lock(&f_lock);

				energy -= 5.0;
				move(*y -2,0);
				printw("Energy: %.1f",energy);
				refresh();
				
				move(*y +2,0);
				printw("Micro H.I.D.: Charging... ");
				refresh();

				sel_msleep(3000);
				printw("FIRE! :-)");
				refresh();

				pthread_mutex_unlock(&f_lock);

				for (int j = 0;j < count_e;j++)
				{
					//Clear all enemies DATA

					pthread_mutex_lock(&f_lock);

					enemies[j] = ' ';
					taken_down++;

					count_e--;
					pthread_mutex_unlock(&f_lock);
				}

				//Clear all enemies SCREEN

				pthread_mutex_lock(&f_lock);

				move(*y,6);
				for (int k = 0;k <= 42;k++)
				{
					printw("=");
					refresh();
					sel_msleep(125);
				}

				move(*y,6);
				for (int k = 0;k <= 42;k++)
				{
					printw(" ");
					refresh();
					sel_msleep(100);
				}
				move(*y +2,0);
				printw("                                             ");
				refresh();

				pthread_mutex_unlock(&f_lock);
			}
			
			if (input == 'X' && energy >= 1.0)	//Skill: 3-X Particle Disintegrator: kill the first enemy whatever it is
			{
				pthread_mutex_lock(&f_lock);

				energy -= 1.0;
				move(*y -2,0);
				printw("Energy: %.1f",energy);
				refresh();
				
				pthread_mutex_unlock(&f_lock);

				srand((unsigned)time(NULL));
				int random = rand() % 10;

				if (random <= 4)
				{
					pthread_mutex_lock(&f_lock);

					move(*y +2,0);
					printw("3-X Particle Disintegrator: <=CRITICAL=>");
					refresh();

					sel_msleep(2000);

					for (int j = 0;j < count_e -1;j++)//-1:exclude the last enemy
					{
						enemies[j] = enemies[j+1];
					}
					count_e--;
					taken_down++;

					move(*y,LENGTH - count_e -1);
					printw("$");
					refresh();

					sel_msleep(800);

					move(*y +2,0);
					printw("                                               ");
					refresh();

					pthread_mutex_unlock(&f_lock);
				}
				else
				{
					pthread_mutex_lock(&f_lock);

					move(*y +2,0);
					printw("3-X Particle Disintegrator: MISSED");
					refresh();

					pthread_mutex_unlock(&f_lock);

					sel_msleep(1500);//let enemy keep moving(father process proceed)

					pthread_mutex_lock(&f_lock);

					move(*y +2,0);
					printw("                                       ");
					refresh();

					pthread_mutex_unlock(&f_lock);
				}
			}
			if (input == '~' && energy >= 10.0)//easter egg
			{
				pthread_mutex_lock(&f_lock);

				energy -= 10.0;
				move(*y -2,0);
				printw("Energy: %.1f",energy);
				refresh();

				move(*y +2,0);
				printw("Attack team,withdraw and regroup.");
				refresh();

				sel_msleep(3250);

				move(*y +3,0);
				printw("Alert conformed.");
				refresh();

				sel_msleep(2000);

				move(*y +4,0);
				printw("A back squad has been dispatch ASAP.");
				refresh();

				sel_msleep(3200);

				move(*y +5,0);
				printw("All unit exercise extreme caution.");
				refresh();

				sel_msleep(6000);

				move(*y +2,0);
				printw("                                               ");
				move(*y +3,0);
				printw("                                               ");
				move(*y +4,0);
				printw("                                               ");
				move(*y +5,0);
				printw("                                               ");

				pthread_mutex_unlock(&f_lock);
			}
		}
	}
}
