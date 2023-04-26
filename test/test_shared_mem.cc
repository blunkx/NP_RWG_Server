#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

const int ARRAY_SIZE = 100;
const int SHM_SIZE = sizeof(int) * ARRAY_SIZE;

int main()
{
    int shmid, *shm, *s, sum = 0;
    key_t key = 1234;

    // Create shared memory segment
    if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory segment
    if ((shm = (int *)shmat(shmid, NULL, 0)) == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // Initialize the shared memory with an array of integers
    for (s = shm; s < shm + ARRAY_SIZE; s++)
    {
        *s = rand() % 100 + 1;
    }

    // Fork a child process
    pid_t pid = fork();

    if (pid == 0)
    { // Child process
        // Compute the sum of the first half of the array
        for (s = shm; s < shm + ARRAY_SIZE / 2; s++)
        {
            sum += *s;
        }
        cout << "Child process: Sum of first half of array = " << sum << endl;
        // Detach shared memory segment
        shmdt(shm);
        exit(0);
    }
    else if (pid > 0)
    { // Parent process
        // Wait for child process to finish
        wait(NULL);
        // Compute the sum of the second half of the array
        for (s = shm + ARRAY_SIZE / 2; s < shm + ARRAY_SIZE; s++)
        {
            sum += *s;
        }
        cout << "Parent process: Sum of second half of array = " << sum << endl;
        // Detach and remove shared memory segment
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        exit(0);
    }
    else
    { // Error occurred during fork
        perror("fork");
        exit(1);
    }

    return 0;
}