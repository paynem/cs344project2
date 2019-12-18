#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define SROOM "START_ROOM"     // Associating an integer with the starting room helps with later operations
#define MROOM "MID_ROOM"   // Ditto with middle rooms
#define EROOM "END_ROOM"     // And ditto with the end room

// This is the pool of names that my program picks from when choosing names for the rooms.
// These are the names of pets that I've had over the course of my life (Popo, Mr. Chicken, Baba Yaga, and Fancy Lala are all still alive)
const char *NAMES[] = {"popo", "chicken", "kong", "baba", "oswald", "fancy", "ami", "bogdon", "luthor", "emma"};

// This struck represents an individual room
typedef struct room
{
    char name[20];  // It's the name of the room, obviously
    int connections; // number of connections that this room has
    int adjRooms[7]; // an array that contains the indices attached to the rooms that are connected to this room
    char cRooms[7][20]; // 2d array of rooms that are connected to this room
    char roomType[15];       // and the room type (1 is starting room, 2 is middle room, and 3 is ending room)

} ROOM;

// buildAndWriteRooms is the central file of the process of building each room and then writing it to a file
void buildAndWriteRooms();
// isGraphFull checks to see if every room has at required 3 connections
int isGraphFull(ROOM rooms[]);
// addRandomConnection randomly picks a room from the room array and then calls connectRoom to add a connection (it does the same for the destination room)
void addRandomConnection(ROOM rooms[]);
// connectionAlreadyExists checks to see if there is already a connection between the two rooms in question
int connectionAlreadyExists(ROOM rooms[], int i, int j);
// connectRoom generates a connection between the two rooms passed to it
void connectRoom(ROOM rooms[], int i, int j);
// This testing function prints out the contents of all of the structs that have been generated
void garboTestingFunc(ROOM rooms[]);
// writeRooms writes all of the room structs to textfiles
void writeRooms(ROOM rooms[]);

int main(void)
{
    // Main exists just to call this buildAndWriteRooms, which does just that.
    buildAndWriteRooms();
    // ending program
    return 0;
}

// This function originally had a lot more functionality until I broke it up (It actually had all of the code to write the rooms to files)
// 
void buildAndWriteRooms()
{
    // Seeding random num generator
    srand((unsigned)time(NULL));
    // Need i, j, and k to hold random int values
    // sR and eR help designate start and end room
    int i, j, k, sR = 0, eR = 0, pid = getpid();

    // setting up array of room structs
    ROOM rooms[7];
    // this array prevents names from being used more than once.  Initializing all values to 0
    int usedNames[10] = {0};
    int usedRooms[7] = {0};

    char rDirectory[50];
    char roomFile[20];
    // This for-loop randomly assigns roomtypes and room names to all of the rooms
    for (i = 0; i < 7;)
    {
        // j is used to pick rooms from the room array.
        j = rand() % 7;
        // checking to see if we've picked a starting room
        if (sR == 0)
        {
            // if we have, we start the process of picking a random name.
            // k is used to choose a random name
            k = rand() % 10;
            // assigning the room the START_ROOM type
            strcpy(rooms[j].roomType, SROOM);
            // giving random name to room
            strcpy(rooms[j].name, NAMES[k]);
            // marking the name as used so it isn't used in the future
            usedNames[k] = 1;
            // marking the room as processed
            usedRooms[j] = 1;
            sR = 1;
            i++;
        }
        // Need to check to see if we've chosen an end room.  If we haven't, and the randomly determined index isn't already taken,
        // we start the process of generating an end room
        if (eR == 0 && usedRooms[j] != 1)
        {
            // Assinging room the END_ROOM type
            strcpy(rooms[j].roomType, EROOM);
            // Generate names until we get one that we haven't used (meaning, we check the generated index until we get one that isn't used)
            do
            {
                // randomly generate number from 0 to 9
                k = rand() % 10;
            } while (usedNames[k] == 1);
            // copying name from name REPOSITORY to the chosen room's name field
            strcpy(rooms[j].name, NAMES[k]);
            // marking name and room as processed
            usedNames[k] = 1;
            usedRooms[j] = 1;
            // marking end room as processed
            eR = 1;
            // incrementing i so we can move onto next room in array
            i++;
        }
        // This if statement processes all of the mid rooms.  All of the logic is very similar to what is used for start and end room generation, so
        // I won't repeat it here
        if (usedRooms[j] != 1)
        {
            strcpy(rooms[j].roomType, MROOM);
            do
            {
                k = rand() % 10;
            } while (usedNames[k] == 1);
            strcpy(rooms[j].name, NAMES[k]);
            usedNames[k] = 1;
            usedRooms[j] = 1;
            i++;
        }
    }

    // starting process of generating connections
    // this for-loop sets connection numbers to 0 and the values of the adjRoom arrays for each room to -1
    // This is for tests that need to be run later on in the program
    for (i = 0; i < 7; i++)
    {
        rooms[i].connections = 0;
        for (j = 0; j < 7; j++)
        {
            rooms[i].adjRooms[j] = -1;
        }
    }
    // As long as the graph isn't full, we'll continue to add connections
    while (isGraphFull(rooms) == 0)
    {
        // calling addRandomConnection since we need to add connections
        addRandomConnection(rooms);
    }
    // We've added all our connections, so we're going to start writing the rooms to files.  SO, we call writeRooms();
    writeRooms(rooms);
}

// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
int isGraphFull(ROOM rooms[])
{
    int i;
    for (i = 0; i < 7; i++)
    {
        // if we find a room with less than 3 connections, we immediately return a 0 to signify that we need to add more connections
        if (rooms[i].connections < 3)
        {
            return 0;
        }
    }
    // we have all the connections we need, so we're returning a 1
    return 1;
}

// Adds a random, valid outbound connection from a Room to another Room
void addRandomConnection(ROOM rooms[])
{
    // i and j are used as counters in this function
    int i, j;
    // While a room has less than 6 connections, we can continue to add connections to it
    do
    {
        i = rand() % 7;
    } while (rooms[i].connections > 6);
    // Now we're looking for a room that we can connect our first room to.  We'll search for rooms with less than 6 connections
    do
    {
        j = rand() % 7;
    } while (rooms[j].connections > 6 || i == j);
    // We check to make sure a that the two rooms we have chosen aren't already connected.  If they aren't, we call connectRooms twice to connect them
    // (a connect to the destination room, and a connection back to the source room)
    if (connectionAlreadyExists(rooms, i, j) == 0)
    {
        connectRoom(rooms, i, j);
        connectRoom(rooms, j, i);
    }
}

// Returns true if a connection from Room x to Room y already exists, false otherwise
int connectionAlreadyExists(ROOM rooms[], int i, int j)
{
    // a is used as a counter in this function
    int a;
    // Running through each room's connections to see if the two rooms are connected
    for (a = 0; a < 7; a++)
    {
        // Is the first room connected to second room?
        if (rooms[i].adjRooms[a] == j)
        {
            // if it is, return 1
            return 1;
        }
        // Making sure second room doesn't have a connection to first room (JUST TO BE SAFE)
        if (rooms[j].adjRooms[a] == i)
        {
            // if so, return 1
            return 1;
        }
    }
    // Rooms aren't connected, so return 0
    return 0;
}

// Connects Rooms x and y together, does not check if this connection is valid
void connectRoom(ROOM rooms[], int i, int j)
{
    rooms[i].adjRooms[rooms[i].connections] = j;
    strcpy(rooms[i].cRooms[rooms[i].connections], rooms[j].name);
    rooms[i].connections++;
}
//  This is some worthess testing function that I wrote to make sure the rooms structs were being constructed properly
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

// I originally wrote this function to use the standard input/output libary stuff almost exclusively.  I wanted to practice use system calls like read and write, though,
// so I rewrote it to do that.  
void writeRooms(ROOM rooms[])
{
    // rDirectory is an array that will hold the directory name that we want to make and swap to
    // roomFile is going to hold the name of the specific file that we want to write to
    // temp is going to hold strings values that we want to write to the room files
    char rDirectory[50], roomFile[50], temp[30];
    // i and j are used as counters
    // pid is set equal to getpid() because we need to attach a process id to the file directory name
    // fileD is used to store the file descriptor returned from the open system call
    int i = 0, j, pid = getpid(), fileD;
    // Probably don't need this!
    ssize_t nwritten;
    // attaching the desired directory name to rdirectory
    sprintf(rDirectory, "paynemi.rooms.%d", pid);
    // making the directory (with the desired file permissions)
    mkdir(rDirectory, 0755);
    // changing to the desire directory
    chdir(rDirectory);

    do
    {
        // Using memset to clear out temp and populate it with terminating 0s
        memset(temp, '\0', sizeof(temp));
        // Attaching desired room file name to roomFile (we increment i, so we can tell room files apart (room1, room2, etc.))
        sprintf(roomFile, "room%d", i);
        // fileD is set equal to the file descriptor returned from the open system call (also setting file permissions so we can read and write)
        fileD = open(roomFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        // Building string (in this case, "ROOM NAME: <WHATEVERROOMNAMEIS>") and putting it into temp, so we can later write it to the file
        sprintf(temp, "ROOM NAME: %s\n", rooms[i].name);
        // Not sure if the nwritten is necessary ...
        // We're using write to write the string stored in temp to the file that fileD points to
        nwritten = write(fileD, temp, strlen(temp) * sizeof(char));
        // Using memset to clear out temp
        memset(temp, '\0', sizeof(temp));
        // This for-loop writes all of the CONNECTION lines to the room file
        for (j = 0; j < rooms[i].connections; j++)
        {
            // Storing the CONNECTION line in temp
            sprintf(temp, "CONNECTION %d: %s\n", j + 1, rooms[i].cRooms[j]);
            // Again, I don't think nwritten is necessary.  Need to test
            // Writing contents of temp to room file that fileD points to
            nwritten = write(fileD, temp, strlen(temp) * sizeof(char));
            // Using memset to clear out temp after each written CONNECTION line
            memset(temp, '\0', sizeof(temp));
        }
        // Now we need to write down the ROOM TYPE line.
            sprintf(temp, "ROOM TYPE: %s\n", rooms[i].roomType);
            nwritten = write(fileD, temp, strlen(temp) * sizeof(char));
        
        i++;
        // Just to be safe, we're setting the lseek to point to beginning of file
        lseek(fileD, 0, SEEK_SET);
    } while (i < 7);
    // changing back to parent directory
    chdir("..");
}
