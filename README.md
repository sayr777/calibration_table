# calibration_table
 Integrated calibration table for capacitive fuel level sensor


Implementation Features:

- FlashStorage library is used to work with non-volatile memory;
- Data is stored in a structure with a maximum size of 20 points;
- Automatic sorting of calibration points by ADC value;
- Linear interpolation between calibration points.

Control commands via Serial:

- **CALIBRATE** - start calibration
- **SAVE** - save calibration
- **LOAD** - load calibration
- **SHOW** - show current calibration
- **CONVERT** [value] - manual value conversion

CALIBRATE - Начать калибровку
SAVE      - Сохранить калибровку
LOAD      - Загрузить калибровку
SHOW      - Показать калибровку
DELETE    - Удалить калибровку
SIM ON    - Включить симуляцию
SIM OFF   - Выключить симуляцию
CONVERT X - Ручной расчёт значения


Recommendations for use:

- Connect the sensor to analog input A0;
- Load the program into the Arduino;
- Use the serial port monitor (115200 baud) for control;
- For calibration, enter pairs of values separated by a space (for example: “500 10.5”);
- After the calibration is complete, save the data with the SAVE command.

The program automatically uses the optimal address in flash memory provided by the FlashStorage library, which eliminates the possibility of address conflicts with the main program.


can be entered:

Whole numbers: 500 10

Fractional numbers: 700 15.5

Zero: 0 0 0

To work with fractional numbers, use a period as a separator (not a comma).

Example of correct input:


```bash
500 10.0
750 25.5
1000 50

```


*зависимость от температуры (диэлектрическая и тепловая зависимости), геометрии бака, скорости движения, типа топлива. 





