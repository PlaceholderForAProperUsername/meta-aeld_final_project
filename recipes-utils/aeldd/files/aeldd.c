#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <string.h>

static const char *LCD_DEV = "/dev/aeldhd44780";
static const char *BME280_DEV = "/dev/aeldbme280"

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
  double measurement_data[3];
  char *temperature[20];
  char *humidity[20];
  char *pressure[20];
  
  start_daemon();
  syslog(LOG_NOTICE, "aeldd: Daemon started.");
  
  sleep(5); // let system start
  
  lcd_fd = fopen(LCD_DEV, "w");
  syslog(LOG_NOTICE, "aeldd: LCD Device opened.");
  
  bme280_fd = fopen(BME280_DEV, "R");
  syslog(LOG_NOTICE, "aeldd: BME280 Device opened.")
  
  while (1)
  {
    fread(measurement_data, sizeof(measurement_data), 1, bme280_fd);
    temperature = "Temp.: %.2lf °C ", measurement_data[0];
    pressure = "Press.: %.2lf Pa ", measurement_data[1];
    humidity = "Hum.: %.2lf % ", measurement_data[2];
    fseek(lcd_fd, row_start[0], SEEK_SET);
    fwrite(temperature, strlen(temperature), 1, lcd_fd);
    fseek(lcd_fd, row_start[1], SEEK_SET);
    fwrite(pressure, strlen(pressure), 1, lcd_fd);
    fseek(lcd_fd, row_start[2], SEEK_SET);
    fwrite(humidity, strlen(humidity), 1, lcd_fd);
    syslog(LOG_NOTICE, "aeldd: hello send to display.");
    sleep(1);
  }
  
  closelog();
  
  return EXIT_SUCCESS;
}
