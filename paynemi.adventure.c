#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>

#define SROOM 1 // Want to associate a number with the starting room
#define MROOM 2 // Want to associate a number with the middle rooms
#define EROOM 3 // Want to associate a number with the ending room

pthread_mutex_t aMutex = PTHREAD_MUTEX_INITIALIZER; // my mutex; so I can lock and unlock threads

// This struct represents an individual room
typedef struct room
{
    char name[20];      // name of room
    char cRooms[7][20]; // 2d array of rooms that are connected to this room
    int connections;    // number of connections that this room has
    char roomType[10];  // room type

} ROOM;

// the game function is the central method of the process starting and running the game.
void game();
// this is a testing function that makes sure the structs have all requisite data
void garboTestingFunc(ROOM rooms[]);
// This prints a description of the room that the player is currently in
void printRoom(ROOM room);
// this funtion takes input from the player
void userCommand(char uR[], int num);
// checkRoomCon checks to make sure the player's inputted room choice is acceptable
int checkRoomCon(ROOM room, char uR[]);
// readRooms checks the files that the program buildrooms has generated and puts their contents into
void readRooms(ROOM rooms[]);
// wTime writes a timestamp to a file called currentTime.txt
void *wTime();
// rTime reads the contents of a file called currentTime (and prints them)
void rTime();

int main(void)
{
    // calling game function to begin and play game
    game();
    //

    // ending program
    return 0;
}
// game is the primary method of this program.  It finds the newest rooms directory and switches to it.  It then calls readRooms, which reads the
// room files in the aforementioned directory and copies the contents to an array of structs. After that, it runs the code that allows the player to, well play the
// game.  The player can move from room to room (assuming they choose a legal connection).  When they make it to the end room, the game ends, and the player is congratulated
void game()
{
    // locking the  mutex from the main therad
    pthread_mutex_lock(&aMutex);
    // setting up my second thread
    pthread_t threadId = pthread_self();
    // Creating the second thread and making its start routine the wTime function.  Since it tries to get a lock, it is blocked immediately
    pthread_create(&threadId, NULL, *wTime, NULL);
    // Creating the array to store my room structs
    ROOM gameRooms[7];
    // history is used to keep track of the number of steps the player has taken and the rooms that they have visited
    char history[100][9];
    // userResponse stores the player's response to the WHERE TO question
    char userResponse[50];
    // some thread nonsense.
    //pthread_t threadId = pthread_self();
    // i, j, and k are used for a huge variety of purposes (mostly as counters and such) through the program.
    // sRoom and eRoom are used to store the indices of the starting and ending rooms
    int i = 0, j, k, sRoom, eRoom;

    // this block of code (line 73 to 101) comes from Instructor Brewster's 2.4 Manipulating Directories reading material
    int newestDirTime = -1;                      // Modified timestamp of newest subdir examined
    char targetDirPrefix[32] = "paynemi.rooms."; // Prefix we're looking for
    char newestDirName[256];                     // Holds the name of the newest dir that contains prefix
    memset(newestDirName, '\0', sizeof(newestDirName));

    DIR *dirToCheck;           // Holds the directory we're starting in
    struct dirent *fileInDir;  // Holds the current subdir of the starting dir
    struct stat dirAttributes; // Holds information we've gained about subdir

    dirToCheck = opendir("."); // Open up the directory this program was run in

    if (dirToCheck > 0) // Make sure the current directory could be opened
    {
        while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
        {
            if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) // If entry has prefix
            {
                stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

                if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
                {
                    newestDirTime = (int)dirAttributes.st_mtime;
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }
    closedir(dirToCheck); // Close the directory we opened

    chdir(newestDirName); // Changing directories to the newest room folder
    readRooms(gameRooms); // calling readRooms to start reading the files in the room folder
    chdir("..");          // swapping back to directory that this file is in
    // This for-loop finds the starting room, sets sRoom equal to its index and then calls printRoom in order to print the description
    // of the room that the player starts in.  It then locates the end room and sets eRoom to its index
    for (i = 0; i < 7; i++)
    {
        if (strcmp(gameRooms[i].roomType, "START_ROOM") == 0) // using strcmp to locate starting room
        {
            sRoom = i;
            printRoom(gameRooms[i]);
        }
        if (strcmp(gameRooms[i].roomType, "END_ROOM") == 0) // using strcmp to locate ending room
        {
            eRoom = i;
        }
    }

    // i, j, and k are used as counters for the subsequent while loop
    i = -5;
    j = 0;
    k = 1;
    // This while loop essentially runs the entire game for the player.
    while (1)
    {
        // This logic might see strange, but it keeps the loop from putting the starting room into the history (although travelling to the starting room later will add it)
        // array.
        if (i == -5)
        {
            i = sRoom;
        }
        // If the user response is time, we need to print a WHERE TO? > prompt, since the the time functions don't do that themselves.
        else if (strcmp(userResponse, "time") == 0)
        {
            printf("WHERE TO? >");
        }
        // We store the name of the newly entered room into the hisory array, and increment j by 1 (which is keeping track where we are in the array)
        else
        {

            strcpy(history[j], gameRooms[i].name);
            j++;
        }
        // if i is equal to the index of the ending room, the game ends.  A congratulatory message is printed, and the number of steps and each room is printed after that
        if (i == eRoom)
        {
            printf("YOU HAVE FOUND THE END ROOM.  CONGRATULATIONS!\n");
            printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", j);
            for (i = 0; i < j; i++)
            {
                printf("%s\n", history[i]);
            }
            break;
            // end game
        }
        //This do-while loop gets the user's response.  It ends if the user responds with time or if the name of a valid connecting room.
        do
        {
            userCommand(userResponse, 50);
            // strcmp is used to check to see if time is given as an answer.
            // The function checkRoomCon (which is essentially a boolean function) is called to see if a valid connecting room has been typed in.
        } while ((k = strcmp(userResponse, "time")) != 0 && checkRoomCon(gameRooms[i], userResponse) == 0);
        // k is set to 0 when the user types in time.  We need to preserve the i value (which stores the index of the room that we're currently in) when the user opts
        // to have the current time printed.  Otherwise, i needs to be reset to 0 so that the subsequent do-while loop can run properly and move the user to the next room
        if (k != 0)
        {
            i = 0;
        }
        // k is reset to 1 so that the previous do-while loop can properly look for "time" inputs from the user
        k = 1;
        // this do-while loop either goes through the obnoxious process of writing and then reading and printing out a time stamp or it finds the index of the room
        // that the user opted to travel to and then prints its information out
        do
        {
            if (strcmp(userResponse, "time") == 0)
            {
                // unlocking mutex in main thread, which causes second thread to unblock and start running
                pthread_mutex_unlock(&aMutex);
                // We immediately call pthread_join, which blocks the main thread until the second thread finishes its job.
                pthread_join(threadId, NULL);
                // The second thread ends after wTime is finished.  This unlocks the main thread, which we use to execute rTime (which reads (and prints) from the file
                // that wTime and the second thread created)
                // We again call for a lock from the main thread
                rTime();
                // We again call for a lock from the main thread
                // And recreate the second thread, which is immediately locked because it tries to call for a lock.
                // exiting from the do-while loop
                pthread_mutex_lock(&aMutex);
                pthread_create(&threadId, NULL, *wTime, NULL);
                break;
            }
            // When we get to the index of the room that the user selected, we call printRoom
            else if (strcmp(gameRooms[i].name, userResponse) == 0)
            {
                // printing a newline for formatting reasons
                printf("\n");
                // We don't want to print the information for the ending room.  We just want to exit the game.
                if (i != eRoom)
                {
                    printRoom(gameRooms[i]);
                }
                break;
            }
            // incrementing i by 1, so we can check the next room
            i++;
        } while (1);
    }
    //pthread_join(threadId, NULL);
}
// Originally, I exclusively used the standard input/output library to handle reading and writing the rooms.  But I wanted to get practice using system calls (read and write),
// so I rewrote this function to, well, do that.
void readRooms(ROOM rooms[])
{
    // roomFile is a character array that stores the names of the room files that are accessed and read
    // temp and cName are used to temporarily hold strings that we read from the room files
    char roomFile[50], temp[30], cName[10];
    // i and j are used as counters in the subsequent do-while loop
    // fileD is used to store file descriptor returned from the open() function
    int i = 0, j, fileD;

    do
    {
        // clearing out both the roomFile and temp arrays and filling them with terminating zeroes with memset
        memset(roomFile, '\0', sizeof(roomFile));
        memset(temp, '\0', sizeof(temp));
        // putting the filename that we want to access in roomFile with sprintf.  Each room file is numbered, so we're able to hit all of them by incrementing i
        // with each loop of the do-while loop
        sprintf(roomFile, "room%d", i);
        // fileD is given the value of the file descriptor returned from open
        fileD = open(roomFile, O_RDONLY);
        // setting j to 0 for the upcoming while loop
        j = 0;
        // This while loop reads an individual character from the current room file and stores it in the specified location in the temp array.  When it hits a newline
        // or the end of a line, it sets the next character to a terminating 0 and then ends the loop.  Each iteration of the loop increments j by 1.
        // I got the idea to do this from this stackoverflow thread:
        // https://stackoverflow.com/questions/33262499/reading-files-using-system-calls-and-printing-lines
        while ((read(fileD, &temp[j], 1)) == 1)
        {
            if (temp[j] == '\n' || temp[j] == 0x0)
            {
                temp[j] = '\0';
                break;
            }
            j++;
        }
        //sscanf is used to parse through the contents of temp and pull out the room name, which is then stored in the name field of the room that we're currently on in the room struct
        // array.
        sscanf(temp, "ROOM NAME: %s\n", &rooms[i].name);
        // Clearing out temp with memset
        memset(temp, '\0', sizeof(temp));
        // This next while loop essentially moves through lines character by character until it hits a newline.  As it is doing that, it is populating the temp array
        // with the characters that it is reading.  After the first nested loop ends, it has two conditionals that it goes through.  If it is a line with
        // Connection information, sscanf is parse through the line and pull out the data that we need to populate the connections and cRooms fields of our room structs array.
        //  If it is the final line with the room type info, sscanf is used to grab the desired room type info, which is then stored in the roomtype field in the rooms structs
        // array
        while (1)
        {
            // Setting j to 0 everytime we restart the entire loop, as it is used to store each read character into the appropriate spot in the temp character array
            j = 0;
            // reading each character from a line and storing it until we hit a newline
            // I got the idea to do this from this stackoverflow thread:
            // https://stackoverflow.com/questions/33262499/reading-files-using-system-calls-and-printing-lines
            while ((read(fileD, &temp[j], 1)) == 1)
            {
                if (temp[j] == '\n' || temp[j] == 0x0)
                {
                    temp[j] = '\0';
                    break;
                }
                j++;
            }
            //We look at the first character in the temp array.  If it is C, it holds a line with connecting room info.  Consequently, we pull the appropriate information from it
            // and use it to populate the connections and cRooms fields of the rooms structs array
            if (temp[0] == 'C')
            {
                sscanf(temp, "CONNECTION %d: %s", &rooms[i].connections, &cName);
                strcpy(rooms[i].cRooms[rooms[i].connections - 1], cName);
                memset(temp, '\0', sizeof(temp));
                memset(cName, '\0', sizeof(cName));
            }
            // If the first character is an R, we have the room type line, so we use sscanf to get the room type and store it in the appropriate field in the rooms array
            if (temp[0] == 'R')
            {
                sscanf(temp, "ROOM TYPE: %s", &rooms[i].roomType);
                memset(temp, '\0', sizeof(temp));
                break;
            }
        }
        i++;
        // setting lseek to point to the beginning of the the file at the end of every run through the main while loop.  I don't think this is necessary (it worked without it),
        // but I'm just being careful
        lseek(fileD, 0, SEEK_SET);
    } while (i < 7);
}

// This function/method/whatever prints descriptive text that tells the player which room they are currently in.
void printRoom(ROOM room)
{
    int i = 0;
    // This is all pretty simple.  We're printing out the text that the assignment requires and then printing out the specific room's
    // connections ( so that the user knows which rooms to pick ).
    printf("CURRENT LOCATION: %s\n", room.name);
    printf("POSSIBLE CONNECTIONS: ");
    for (i = 0; i < room.connections; i++)
    {
        if (i < room.connections - 1)
        {
            printf("%s, ", room.cRooms[i]);
        }
        else
        {
            printf("%s.\n", room.cRooms[i]);
        }
    }
    printf("WHERE TO? >");
}
// userCommand takes input from the user via the getchar() function.
// This function is borrowed from K.N. King's "C Programming: A Modern Approach"
void userCommand(char uR[], int num)
{
    int i = 0, j;
    // I should also check against EoF, but this function works as is
    while ((j = getchar()) != '\n')
    {
        // While I is lower than the buffer's length, we continue to add characters
        if (i < num)
        {
            uR[i] = j;
            i++;
        }
    }
    // Adding terminating 0 to end of string
    uR[i] = '\0';
}
// This function checks to make sure that the user has something that matches up with one of the valid connecting rooms of the room that they are currently in
int checkRoomCon(ROOM room, char uR[])
{
    int i;
    // This for-loop goes through the connecting rooms of the room in question and checks it against the user's answer.
    // If a match is found, a value of 1 (true) is returned
    for (i = 0; i < room.connections; i++)
    {
        if (strcmp(uR, room.cRooms[i]) == 0)
        {
            return 1;
        }
    }
    // if no match is found, an error message is printed, a description of the player's current location is printed again, and a value of 0 is returned
    printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
    printRoom(room);
    return 0;
}
// This function generates a time stamp and writes it to a file called currentTime.txt
void *wTime()
{
    // Right when the second thread is created and this function is called, the a lock is called from the second thread.  This blocks the second thread
    // and puts wTime on hold
    pthread_mutex_lock(&aMutex);
    // timeFile is used to store the name of the file that we're going to create (well, for the first call) and write to
    // theTime is where the time stamp is stored
    char timeFile[20], theTime[50];
    // t will be used to store system time values
    time_t t;
    // This puts the time in seconds into t
    time(&t);
    // We need to set up a struct, so that we can populate it with data from local time
    struct tm *temp;
    // putting the name of the file that we want to write to into timeFile
    sprintf(timeFile, "currentTime.txt");
    // Opening up the file for writing
    FILE *fTime = fopen(timeFile, "w");
    // localtime takes t and uses it to populate the struct temp with values tht represent the corresponding local time
    temp = localtime(&t);
    // strftime then takes values from the temp struct and uses them to build up a string that represents a good timestamp.  This is put
    // into theTime
    strftime(theTime, sizeof(theTime), "%l:%M%p, %A, %B %d, %Y", temp);
    // The time stamp is written to the the file currentTime.txt
    fprintf(fTime, "%s\n", theTime);
    // closing the file
    fclose(fTime);
    // unlocking the main thread
    pthread_mutex_unlock(&aMutex);
}
// rTime reads the currentTime.txt file and pulls the timestamp written in it.  It then prints it out (in a nice formatted way)
void rTime()
{
    // We need len for getline.  The length can be 0 since getline will resize it dynamically as necessary.
    size_t len = 0;
    // timeFile will hold the file name
    // timePrint is going to hold the line grabbed with getline
    char timeFile[20], *timePrint = NULL;
    // writing file name currentTime.txt to timeFile
    sprintf(timeFile, "currentTime.txt");
    // Opening the currentTime.txt file for reading
    FILE *fTime = fopen(timeFile, "r");
    // Using getline to copy the time stamp and put it into timePrint
    getline(&timePrint, &len, fTime);
    // printing the timestamp
    printf("\n%s\n", timePrint);
    // closing the file
    fclose(fTime);
    // getline mallocs (I BELIEVE) the space that it needs , so we need to get rid of it
    free(timePrint);
}
// worthless testing function
void garboTestingFunc(ROOM rooms[])
{
    int i, j;
    for (i = 0; i < 7; i++)
    {
        printf("Name = %s  Type = %s\n", rooms[i].name, rooms[i].roomType);
    }
    for (i = 0; i < 7; i++)
    {
        printf("Connections: %d\n", rooms[i].connections);
    }
    for (i = 0; i < 7; i++)
    {
        printf("Room %s's connections:\n", rooms[i].name);
        for (j = 0; j < rooms[i].connections; j++)
        {
            printf("Connection %d: %s", j, rooms[i].cRooms[j]);
            if (j < rooms[i].connections - 1)
            {
                printf(", ");
            }
        }
        printf("\n");
    }
}
