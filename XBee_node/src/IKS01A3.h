#include <LIS2MDLSensor.h>
#include <LPS22HHSensor.h>
#include <STTS751Sensor.h>
#include <HTS221Sensor.h>
#define DEV_I2C Wire
#define SerialPort Serial

LIS2MDLSensor Mag(&DEV_I2C);
LPS22HHSensor PressTemp(&DEV_I2C);
HTS221Sensor HumTemp(&DEV_I2C);
STTS751Sensor Temp3(&DEV_I2C);


void setup_sensors()
{
  // Initialize I2C bus.
  DEV_I2C.begin();
  
  Mag.begin();
  Mag.Enable();
  PressTemp.begin();
  PressTemp.Enable();
  HumTemp.begin();
  HumTemp.Enable();
  Temp3.begin();
  Temp3.Enable();
}

int read_sensors(char *buffer)
{
    // Read humidity and temperature.
  float humidity = 0, temperature = 0;
  HumTemp.GetHumidity(&humidity);
  HumTemp.GetTemperature(&temperature);

  // Read pressure and temperature.
  float pressure = 0, temperature2 = 0;
  PressTemp.GetPressure(&pressure);
  PressTemp.GetTemperature(&temperature2);

  //Read temperature
  float temperature3 = 0;
  Temp3.GetTemperature(&temperature3);

  //Read magnetometer
  int32_t magnetometer[3];
  Mag.GetAxes(magnetometer);

  float real_mag[3];
  real_mag[0] = magnetometer[0] / 10.0;
  real_mag[1] = magnetometer[1] / 10.0;
  real_mag[2] = magnetometer[2] / 10.0;

  // Write data to buffer.
  int length = sprintf(&buffer[1], "%.2f,%.2f,%.1f,%.1f,%.1f,%.1f", temperature, humidity, pressure, real_mag[0], real_mag[1], real_mag[2]);

  // Output data.
  // SerialPort.print("| Hum[%]: ");
  // SerialPort.print(humidity, 2);
  // SerialPort.print(" | Temp[C]: ");
  // SerialPort.print(temperature, 2);
  // SerialPort.print(" | Pres[hPa]: ");
  // SerialPort.print(pressure, 2);
  // SerialPort.print(" | Temp2[C]: ");
  // SerialPort.print(temperature2, 2);
  // SerialPort.print(" | Temp3[C]: ");
  // SerialPort.print(temperature3, 2);
  // SerialPort.print(" | Mag[mGauss]: ");
  // SerialPort.print(magnetometer[0]);
  // SerialPort.print(" ");
  // SerialPort.print(magnetometer[1]);
  // SerialPort.print(" ");
  // SerialPort.print(magnetometer[2]);
  // SerialPort.println(" |");

  return length;
}