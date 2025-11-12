#include "ProcessManagement.hpp"
#include "Task.hpp"
#include <queue>
#include <memory>
#include <iostream>
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
// Windows does not provide <sys/wait.h>; the POSIX wait APIs are not available.
// Include <sys/wait.h> only on POSIX systems.
#else
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#endif
#include "../encryptDecrypt/Cryption.hpp"
#include <atomic>
#include<semaphore.h>
//k

ProcessManagement::ProcessManagement()
{
   itemsSemaphore = sem_open("/item_semaphore",O_CREAT,0666,0);
   emptySlotsSemaphore = sem_open("/empty_slots_semaphore",O_CREAT,0666,1000);

  shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(shmFd, sizeof(SharedMemory));
  sharedMem = static_cast<SharedMemory *>(mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0));
  sharedMem->front = 0;
  sharedMem->rear = 0;
  sharedMem->size.store(0);
}
bool ProcessManagement::submitToQueue(std::unique_ptr<Task> &task)
{
  sem_wait(emptySlotsSemaphore);
  std::unique_lock<std::mutex> lock(queueLock);
  if (sharedMem->size.load() >= 1000)
  {
    return false;
  }
  strcpy(sharedMem->tasks[sharedMem->rear], task->toString().c_str());
  sharedMem->rear = (sharedMem->rear + 1) % 1000;
  sharedMem->size.fetch_add(1);
  lock.unlock();
  sem_post(itemsSemaphore);
  int pid = fork();
  if (pid < 0)
  {
    return false;
  }
  else if (pid > 0)
  {
    std::cout << "Entering Parent Process" << std::endl;
  }
  else
  {
    std::cout << "Entering Child Process" << std::endl;
    executeTasks();
    std::cout << "Exiting Child Process" << std::endl;
    exit(0);
  }
  return true;
}
void ProcessManagement::executeTasks()
{
  sem_wait(itemsSemaphore);
  std::unique_lock<std::mutex>lock(queueLock);
  char taskStr[256];
  strcpy(taskStr,sharedMem->tasks[sharedMem->front]);
  sharedMem->front = (sharedMem->front+1)%1000;
  sharedMem->size.fetch_sub(1);
  lock.unlock();
  sem_post(emptySlotsSemaphore);
  std::cout << "Excuting task: " << taskStr << std::endl;
  excecuteCryption(taskStr);
}

ProcessManagement::~ProcessManagement(){
  munmap(sharedMem,sizeof(SharedMemory));
  shm_unlink(SHM_NAME);


  
}
