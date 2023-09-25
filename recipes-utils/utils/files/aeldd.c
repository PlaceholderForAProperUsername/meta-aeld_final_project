#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

static const char *LCD_DEV = "/dev/aeldhd44780";

static void start_daemon();

static void start_daemon()
{
  pid_t pid;
  
  pid = fork();
  
  if (pid < 0)
  {
    exit(EXIT_FAILURE);
  }
  
  if (pid > 0)
  {
    exit(EXIT_SUCCESS);
  }
  
  if (setsid() < 0)
  {
    exit(EXIT_FAILURE);
  }
  
  pid = fork();
  
  if (pid < 0)
  {
    exit(EXIT_FAILURE);
  }
  
  if (pid > 0)
  {
    exit(EXIT_SUCCESS);
  }
  
  umask(0);
  
  chdir("/");
  
  for (int x = sysconf(_SC_OPEN_MAX); x >= 0; --x)
  {
    close(x);
  }
  
  openlog("aelddaemon", LOG_PID, LOG_DAEMON);
}

int main()
{
  FILE *lcd_fd;
  
  start_daemon();
  syslog(LOG_NOTICE, "aeldd: Daemon started.");
  
  lcd_fd = fopen(LCD_DEV, "w");
  syslog(LOG_NOTICE, "aeldd: Device opened.");
  
  while (1)
  {
    fseek(lcd_fd, 0x00, SEEK_SET);
    fwrite("Hello", sizeof(char), 5, lcd_fd);
    syslog(LOG_NOTICE, "aeldd: hello send to display.");
    sleep(5);
    fseek(lcd_fd, 0x00, SEEK_SET);
    fwrite("world", sizeof(char), 5, lcd_fd);
    syslog(LOG_NOTICE, "aeldd: world send to display.");
    sleep(5);
  }
  
  closelog();
  
  return EXIT_SUCCESS;
}
