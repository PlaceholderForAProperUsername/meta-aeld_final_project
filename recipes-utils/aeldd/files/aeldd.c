#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <string.h>
#include <stdint.h>

static const char *LCD_DEV = "/dev/aeldhd44780";
static const char *BME280_DEV = "/dev/aeldbme280";

static const uint8_t row_start[] = {0x00, 0x40, 0x14, 0x54};

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
  FILE *bme280_fd;
  int32_t measurement_data[3];
  char temperature[20];
  char humidity[20];
  char pressure[20];
  
  start_daemon();
  syslog(LOG_NOTICE, "aeldd: Daemon started.");
  
  sleep(5); // let system start
  
  lcd_fd = fopen(LCD_DEV, "w");
  syslog(LOG_NOTICE, "aeldd: LCD Device opened.");
  
  
  
  while (1)
  {
    bme280_fd = fopen(BME280_DEV, "r");
    syslog(LOG_NOTICE, "aeldd: BME280 Device opened.");
    fread(measurement_data, sizeof(measurement_data), 1, bme280_fd);
    close(bme280_fd);
    snprintf(temperature, 19, "Temp.: %.2f C ", measurement_data[0] / 100.0f);
    snprintf(pressure, 19, "Press.: %.2f kPa ", measurement_data[1] / 1000.0f);
    snprintf(humidity, 19, "Hum.: %d %c ", measurement_data[2], '%');  
    fseek(lcd_fd, row_start[0], SEEK_SET);
    fwrite(temperature, strlen(temperature), 1, lcd_fd);
    fseek(lcd_fd, row_start[1], SEEK_SET);
    fwrite(pressure, strlen(pressure), 1, lcd_fd);
    fseek(lcd_fd, row_start[2], SEEK_SET);
    fwrite(humidity, strlen(humidity), 1, lcd_fd);
    sleep(5);
  }
  
  closelog();
  
  return EXIT_SUCCESS;
}
