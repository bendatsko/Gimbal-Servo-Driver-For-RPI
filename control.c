#include <unistd.h> // required for I2C device access and usleep()

#include <fcntl.h> // required for I2C device configuration

#include <sys/ioctl.h> // required for I2C device usage

#include <linux/i2c-dev.h> // required for constant definitions

#include <stdio.h> // required for printf statements

#include <stdlib.h> // required for input parsing

using namespace std;

int main(int argc, char *argv[])
{

  // ------ Error checking ------ //

  // Validate argument count
  if (argc != 3)
  {
    printf("Usage: %s <channel> <angle>\n", argv[0]);
    printf("channel: The servo channel number to control.\n");
    printf("angle: The desired angle to set for the servo, within 0 to 180 degrees.\n");
    return -1;
  }

  // Validate arguments
  int angleMeasure = atoi(argv[2]);
  int channelNumber = atoi(argv[1]);
  if (channelNumber < 0 || channelNumber > 15)
  {
    printf("Invalid channel number. Please specify a channel number between 0 and 15.\n");
    return -1;
  }
  if (angleMeasure < 0 || angleMeasure > 203)
  {
    printf("Invalid angle. Please specify an angle between 0 and 180 degrees.\n");
    return -1;
  }

  // Open I2C device file for read/write and check for errors
  char *filename = (char *)"/dev/i2c-1"; // Define filename
  int file_i2c = open(filename, O_RDWR); // open file for R/W
  if (file_i2c < 0)
  {
    printf("Failed to open file!");
    return -1;
  }

  // ------ PCA9685 PWM controller ------ //
  int addr = 0x40;                  // PCA9685 address
  ioctl(file_i2c, I2C_SLAVE, addr); // Set I2C address for upcoming transactions

  char buffer[5]; // Create a buffer for transferring data to I2C device

  // Enable the chip. We do this by writing 0x20 to register 0. buffer[0] is always the register
  // address, and subsequent bytes are written out in order.
  buffer[0] = 0;                   // target register
  buffer[1] = 0x20;                // desired value
  int length = 2;                  // number of bytes, including address
  write(file_i2c, buffer, length); // initiate write

  // Enable multi-byte writing.
  buffer[0] = 0xfe; // pre-scaling register
  buffer[1] = 0x78; // set PWM frequency to 50 Hz
  write(file_i2c, buffer, length);

  // ------ Servo Control ------ //

  // Write the start time out to the chip. This is the time when the chip will generate a high output.
  buffer[0] = 0x06; // "start time" reg for channel 0
  if (channelNumber == 2)
  {
    buffer[0] = 0x0E; // "start time" reg for channel 2
  }
  buffer[1] = 0; // We want the pulse to start at time t=0
  buffer[2] = 0;
  length = 3;                      // 3 bytes total written
  write(file_i2c, buffer, length); // initiate the write

  // Write the stop time out to the chip. This is the time when the chip will
  // generate a low output. We change servo angle by writing low based on the PWM width the
  // servo is spec'd for per angle. This is typically linear. In our case, The value is in
  // units of 1.2us. 1.5ms corresponds to "neutral" position. This is where the value 1250 below comes from.
  int pulseWidth = 836 + (angleMeasure * (1664 - 836) / 180);

  buffer[0] = 0x08; // "stop time" reg for channel 0
  if (channelNumber == 2)
  {
    buffer[0] = 0x10; // "stop time" reg for channel 2
  }
  buffer[1] = pulseWidth & 0xff;        // The "low" byte comes first...
  buffer[2] = (pulseWidth >> 8) & 0xff; // followed by the high byte.
  write(file_i2c, buffer, length);      // Initiate the write.

  return 0;
}