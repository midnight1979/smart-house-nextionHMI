#include <DS3231.h>
#include <PCF8574.h>      // Для PCF8574
#include <U8glib.h>       // Для OLED-дисплея
#include <EEPROM.h>       // Для работы с EEPROM
#include <Wire.h>         // Для I2C
#include <Nextion.h>

/* DS3231 */
DS3231 clock;
RTCDateTime dt;

/* Переменные работы кнопок */
#define NextPageButtonPin 9               // Вход к которому подключена кнопка "Далее"

int LongTapNext = 0;                      // Флаг для длительного нажатия кнопки - чтобы при удержании "Далее"/"Назад" не было цикличного перехода по страницам

// Инициализация LCD12864 (SPI)
U8GLIB_ST7920_128X64_4X u8g(13, 5, 4);
#define BLK 3                               // PIN(3) в Arduino Nano с PWM - используется для регулировки подсветки LCD12864
const int WorkBrightness = 170;             // Значение яркости LCD12864 в рабочем режиме
const int SleepBrightness = 10;             // Значение яркости LCD12864 в спящем режиме
bool SleepMode = false;                     // Текщий режим - "Режим сна"
const int BrightnessInterval = 30000;       // 30 секунд интервал для перехода в режим сна
const int temperature_change_timer = 5000;  // Таймер смены отображения показаний температуры на странице 2 (баня) - температура воздуха/температура воды
bool tmpChange = false;

// Инициализация расширителя портов PCF8574
PCF8574 pcf;

/* ----------------------- Иконки и значки ------------------------------ */
const uint8_t ico_err_status[] U8G_PROGMEM =
{
  // @0 'V' (11 pixels wide)
  0x1F, 0x00, //    #####
  0x20, 0x80, //   #     #
  0x40, 0x40, //  #       #
  0x91, 0x20, // #  #   #  #
  0x8A, 0x20, // #   # #   #
  0x84, 0x20, // #    #    #
  0x8A, 0x20, // #   # #   #
  0x91, 0x20, // #  #   #  #
  0x40, 0x40, //  #       #
  0x20, 0x80, //   #     #
  0x1F, 0x00, //    #####
};

const uint8_t ico_ok_status[] U8G_PROGMEM =
{
  // @0 'a' (8 pixels wide)
  0x01, //        #
  0x03, //       ##
  0x87, // #    ###
  0xCF, // ##  ####
  0xFF, // ########
  0xFE, // #######
  0xFC, // ######
  0x78, //  ####
  0x30, //   ##
};

const uint8_t ico_hotwater_blocked[] U8G_PROGMEM =
{
  0x44, 0x44, 0x40, //  #   #   #   #   #
  0x22, 0x22, 0x20, //   #   #   #   #   #
  0x11, 0x11, 0x10, //    #   #   #   #   #
  0x51, 0x11, 0x18, //  # #   #   #   #   ##
  0xD1, 0x11, 0x1C, // ## #   #   #   #   ###
  0xE2, 0x22, 0x2C, // ###   #   #   #   # ##
  0xE2, 0x22, 0x2C, // ###   #   #   #   # ##
  0xC4, 0x44, 0x4C, // ##   #   #   #   #  ##
  0xC4, 0x44, 0x4C, // ##   #   #   #   #  ##
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xFF, 0xFF, 0xFC, // ######################
};

const uint8_t ico_water_temp[] U8G_PROGMEM =
{
  0x03, 0xE6, //       #####  ##
  0x02, 0x49, //       #  #  #  #
  0x02, 0xE9, //       # ### #  #
  0x02, 0x46, //       #  #   ##
  0x02, 0xE0, //       # ###
  0x02, 0x40, //       #  #
  0x7A, 0xE0, //  #### # ###
  0x86, 0x40, // #    ##  #
  0x02, 0x40, //       #  #
  0x04, 0x21, //      #    #    #
  0x08, 0x1E, //     #      ####
  0x08, 0x10, //     #      #
  0x7F, 0xF0, //  ###########
  0x8F, 0xF9, // #   #########  #
  0x07, 0xE6, //      ######  ##
  0x03, 0xC0, //       ####
};

const uint8_t ico_air_temp[] U8G_PROGMEM =
{
  0x18, 0x06, //    ##        ##
  0x24, 0x09, //   #  #      #  #
  0x34, 0x09, //   ## #      #  #
  0x24, 0xE6, //   #  #  ###  ##
  0x35, 0x10, //   ## # #   #
  0x25, 0x00, //   #  # #
  0x35, 0x00, //   ## # #
  0x25, 0x10, //   #  # #   #
  0x34, 0xE0, //   ## #  ###
  0x66, 0x00, //  ##  ##
  0xC3, 0x00, // ##    ##
  0x81, 0x00, // #      #
  0xFF, 0x00, // ########
  0xFF, 0x00, // ########
  0x7E, 0x00, //  ######
  0x3C, 0x00, //   ####
};


const uint8_t ico_stones_temp[] U8G_PROGMEM =
{
  0x03, 0xE6, //       #####  ##
  0x02, 0x49, //       #  #  #  #
  0x02, 0xE9, //       # ### #  #
  0x02, 0x46, //       #  #   ##
  0x02, 0xE0, //       # ###
  0x02, 0x40, //       #  #
  0xFE, 0xFE, // ####### #######
  0xD6, 0x56, // ## # ##  # # ##
  0xAA, 0x6A, // # # # #  ## # #
  0xFF, 0xFF, // ################
  0x28, 0x11, //   # #      #   #
  0x28, 0x11, //   # #      #   #
  0x3C, 0x3F, //   ####    ######
  0xFE, 0x74, // #######  ### #
  0x8F, 0xE4, // #   #######  #
  0xFF, 0xFC, // ##############
};

//const uint8_t left_arrow[] U8G_PROGMEM = {
//  // @0 '3' (5 pixels wide)
//  0x08, //     #
//  0x18, //    ##
//  0x38, //   ###
//  0x78, //  ####
//  0xF8, // #####
//  0xF8, // #####
//  0x78, //  ####
//  0x38, //   ###
//  0x18, //    ##
//  0x08, //     #
//};

const uint8_t right_arrow[] U8G_PROGMEM = {
  // @10 '4' (5 pixels wide)
  0x80, // #
  0xC0, // ##
  0xE0, // ###
  0xF0, // ####
  0xF8, // #####
  0xF8, // #####
  0xF0, // ####
  0xE0, // ###
  0xC0, // ##
  0x80, // #
};

const uint8_t ico_eye[] U8G_PROGMEM = {
  // @0 'N' (15 pixels wide)
  0x07, 0xC0, //      #####
  0x1B, 0xF0, //    ## ######
  0x20, 0x48, //   #      #  #
  0x41, 0xA4, //  #     ## #  #
  0x82, 0xA2, // #     # # #   #
  0x8B, 0xA2, // #   # ### #   #
  0x49, 0x24, //  #  #  #  #  #
  0x24, 0x48, //   #  #   #  #
  0x17, 0xD0, //    # ##### #
  0x0F, 0xE0, //     #######
};
/* END ------------------- Иконки и значки -------------------------- END */

int temp_cm1, cm1 = 0;                     // Расстояние от ультразвукового датчика до поверхности воды в домашней емкости (в см.)
int temp_cm2, cm2 = 0;                     // Расстояние от ультразвукового датчика до поверхности воды в уличной емкости (в см.)

// Качество воды
int wq_raw              = 0;               // Показания датчика прозрачности воды в уличной емкости, полученные со Slave-контроллера
const int averageFactor = 4;               // коэффициент сглаживания показаний (0 = не сглаживать) - чем выше, тем больше "инерционность" - это видно на индикаторах

//-------------------------------------------------------- Параметры емкостей ----------------------------------------------------------------------
//--------- Домашняя емкость --------
int MainTank_Height       = 92;             // Высота домашней емкости в см.
int MainTank_Max          = 7;              // Максимальный уровень воды домашней емкости в см. (от датчика до поверхности воды)
int MainTank_MinHotWater  = 60;             // Минимальное количество воды в домашней емкости в % чтобы заблокировать пополнение бака горячей воды
int MainTank_FillIn       = 80;             // Уровень воды в домашней емкости в % при котором включается режим наполнения
int MainTank_MinBlock     = 10;             // Минимальное количество воды в домашней емкости в % чтобы заблокировать работу насосной станции
int HotWater_Unblock      = 70;             // Количество воды в домашней емкости в % при котором разблокируется пополнение бака горячей воды

//-------- Уличная емкость ----------
int StreetTank_Height         = 110;         // Высота домашней емкости в см.
int StreetTank_Max            = 10;         // Максимальный уровень воды домашней емкости в см. (от датчика до поверхности воды)
int StreetTank_MinBlock       = 15;         // Минимальное количество воды уличной емкости в % чтобы заблокировать работу насоса

//-------- Статусы емкостей ---------
int MainTankStatus = 1;                     // Статус домашней емкости
int StreetTankStatus = 1;                   // Статус уличной емкости
int HotWaterTankStatus = 1;                 // Статус бака с горячей водой

//------ Уровни емкостей в % и объем в литрах ------
int MainTankPercent       = 0;              // Объем воды в % для домашней емкости
int StreetTankPercent     = 0;              // Объем воды в % для уличной емкости
int HotWaterTankPercent   = 0;              // Объем воды в % для бака с горячей водой

//------ Переменные для работы с таймерами ---------
int progress                      = 0;      // Переменная для цикличного индикатора процесса работы (анимирования символов)
unsigned long previousMillisBLK   = 0;      // Счетчик для регулировки подсветки LCD12864 (режим сна через BrightnessInterval)
unsigned long previousMillisTMP   = 0;      // Счетчик смены показаний температуры
unsigned long previousMillisINIT  = 0;      // Таймаут инициализации при включении

//unsigned long currentMillis       = 0;

// Блокировки емкостей и насосов
bool HotWaterTankIsBlocked = false;         // Заблокировано ли пополнение бака горячей воды
bool MainPumpIsBlocked = false;             // Насосная станция заблокирована

//-------------------- Переменные для работы со страницами -------------------------
int CurrentPage = 1;                       // Текущая страница: 1 - данные главной емкости, 2 - данные бака горячей воды, 3 - данные уличной емкости, 4 - скважина, 5 - Настройки

//-------------------- Номера реле в расширителе портов 8574 -----------------------
const int MainTankRelay      = 0;          // Насосная станция (0-й порт)
const int StreetTankRelay    = 1;          // Насос уличных емкостей (1-й порт)
const int HotWaterTankRelay  = 2;          // Электроклапан емкости с горячей водой (2-порт)

const int DirtyWaterRelay      = 4;          // Реле клапана грязной воды
const int CleanWaterRelay      = 3;          // Реле клапана чистой воды

// Резерв портов на блоке реле - в последующем дать переменным нормальные имена
const int port4              = 5;   //Реле пока не работает (неисправность реле)
const int port7              = 6;
const int port8              = 7;

// --------------------- Скважина -------------------------------------------
// Для Nextion
uint32_t numberProgressBar      = 0;          // Значение передаваемое в Nextion для визуализации наполнения скважины
unsigned long previousMillisHMI = 0;          // Переменная дла организации задержки при цикле визуализации наполнения
const long intervalHMI          = 500;        // Интервал цикла визуализации HMI Nextion
bool ImpulseUpdated             = false;      // Флаг обновления импульса со входа прерывания (interupt)

/* Переменные аппаратных входов */
#define impulsePin 2             // Цифровой вход для подачи импульсов от счетчика для защиты насоса от сухого хода
#define SkvajinaPin 10           // Выход включения насоса - подключается к автоматике скважины (HIGH-включено/LOW-выключено)
#define TurbiditySensorPin A0    // Аналоговый вход с датчика прозрачности

/* Переменные значений входов и выходов */
int Turbidity   = 0;                   // Переменная для получения значений датчика прозрачности

/* Служебные переменные и константы */
const long intervalDryWork        = 15000;   // Интервал за который проверяется количество импульсов счетчика (15 сек.) - защита насоса скважины от сухого хода
unsigned long previousMillisSKV   = 0;       // Переменная для хранения интервала для подсчета импульсов
const int minTurbidity            = 250;     // Минимальное значение прозрачности (всё что меньше будет сливаться в дренаж)

int impulses              = 0;          // Переменная для подсчета импульсов внутри интервала (10 сек).
int oldimpulses           = 0;          // Переменная для временного хранения импульсов с целью задержки переключения с грязной воды на чистую
int CommulativeImpulses   = 0;          // Накопительный счетчик импульсов - собирается за сеанс "откачки" (от старта до стопа!)
int CommulativeImpulsesD  = 0;          // Накопительный счетчик импульсов грязной воды
int CommulativeImpulsesC  = 0;          // Накопительный счетчик импульсов чистой воды
bool SkvajNasos         = false;        // Скважинный насос ВКЛ/ВЫКЛ

/* Служебные компоненты ProgressBar, Timer, Button */
NexTimer        tm0       = NexTimer(0, 14, "tm0");
NexProgressBar  j0        = NexProgressBar(0, 2, "j0");
NexButton       btnStart  = NexButton(0, 13, "b0");

/* Поля вывод числовых значений */
NexNumber n0 = NexNumber(0, 1, "n0");                 // поле проценты емкости
NexNumber n1 = NexNumber(0, 3, "n1");                 // поле температура улицы
NexNumber n2 = NexNumber(0, 4, "n2");                 // поле температура в доме
NexNumber n3 = NexNumber(0, 5, "n3");                 // поле импульсы
NexNumber n4 = NexNumber(0, 9, "n4");                 // поле литры чистой воды
NexNumber n5 = NexNumber(0, 10, "n5");                // поле литры грязной воды

/* Индикаторы */
NexCrop q0 = NexCrop(0, 8, "q2");      // Индикатор импульсов
NexCrop q1 = NexCrop(0, 6, "q0");      // Чистая вода
NexCrop q2 = NexCrop(0, 7, "q1");      // Грязная вода
NexCrop q3 = NexCrop(0, 11, "q3");     // Наполнение в дом

bool updateHMI = false;
const int litresKoeff = 70;             // Коэффициент пересчета ипульсов в литры

NexTouch *nex_listen_list[] =
{
  &btnStart,
  NULL
};

void setMaxBrightnessNex()
{
  nexSerial.print("dim=100");
  nexSerial.write(0XFF);
  nexSerial.write(0XFF);
  nexSerial.write(0XFF);
}

void buttonStartCallback(void *ptr)
{

  // При нажатии на кнопку устанавливаем яркость экрана на максимум
  setMaxBrightnessNex();

  // Включаем внутренний таймер сна (снижение яркости до 40% через 15 сек.)
  tm0.enable();

  // Если скважина выключена - включаем, изменяем картинки кнопки на вкл.
  if (SkvajNasos == false)
  {
    btnStart.Set_background_crop_picc(1);
    btnStart.Set_press_background_crop_picc2(0);
    SkvajNasos = true;
    impulses = 0;
    CommulativeImpulses = 0;
    CommulativeImpulsesD = 0;
    CommulativeImpulsesC = 0;
  }
  else
  {
    btnStart.Set_background_crop_picc(0);
    btnStart.Set_press_background_crop_picc2(0);
    SkvajNasos = false;
    updateHMI = true;
  }
}

// ----------------------------- Температуры --------------------------------
int sauna_stones_temp_Raw     = 0;    // Температура термопары из каменки, полученная со Slave-контроллера
int sauna_stones_Temp         = 0;    // Итоговая температура термопары из каменки - используется для усреднения значений
int dataA                     = 0;    // Upper byte температуры
int dataB                     = 0;    // Lower Byte температуры

// Температура воздуха с модуля RTC
//int home_air_Temp  = 0;

// Температуры с терморезисторов
int sauna_air_Temp    = 0;        // Температура воздуха в парилке
int sauna_water_Temp  = 0;        // Температура воды в парилке
int street_Temp       = 0;        // Температура воздуха на улице

// Процедура "посаженная" на прерывание 2-го цифрового входа для подсчета импульсов с датчика
void flow()
{
  if (SkvajNasos == true)
  {
    impulses++;
    if (Turbidity < minTurbidity)
    {
      CommulativeImpulsesD++;
    }
    else
    {
      CommulativeImpulsesC++;
    }
    CommulativeImpulses++;
    ImpulseUpdated = true;
  }
}

void setup()
{
  /* Инициализация i2c */
  Wire.begin();

  /* Перевернем экран на 180 градусов */
  u8g.setRot180();

  /* Устанавливаем PIN(3) на выход для регулировки яркости подсветки LCD12864 */
  pinMode(BLK, OUTPUT);
  LED12864_Brightness(WorkBrightness);

  /* Даем время на инициализацию Slave-контроллеров */
  for (int i = 0; i < 6; i++)
  {
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_unifont_0_8);
      u8g.setPrintPos(10, 22);
      u8g.print("Initialize...");
      u8g.setPrintPos(54, 44);
      u8g.print(map(i, 0, 5, 1, 100));
      u8g.print("%");
      u8g.drawBox(0, 52, map(i, 0, 5, 1, 128), 5);
    } while ( u8g.nextPage() );
    delay(500);
  }
  delay(500);
  /* Таймаут инициализации завершен */

  /* Инициализация I2C для расширителя портов PCF8574
    Адрес согласно установленных перемычек A0-A1-A2 (0x27 - без перемычек) */
  pcf.begin(0x20);

  /* Установка всех 8 портов PCF8574 на выход */
  pcf.pinMode(0, OUTPUT);
  pcf.pinMode(1, OUTPUT);
  pcf.pinMode(2, OUTPUT);
  pcf.pinMode(3, OUTPUT);
  pcf.pinMode(4, OUTPUT);
  pcf.pinMode(5, OUTPUT);
  pcf.pinMode(6, OUTPUT);
  pcf.pinMode(7, OUTPUT);
  pcf.set();

  // Проводим сброс выходов на PCF8574 - изначально после включения на всех выходах HIGH
  PCFReset();

  HotWaterTankIsBlocked   = EEPROM.read(0);
  MainTankStatus          = EEPROM.read(1);
  StreetTankStatus        = EEPROM.read(2);
  HotWaterTankStatus      = EEPROM.read(3);
  MainPumpIsBlocked       = EEPROM.read(5);

  if (MainTankStatus == 255)
  {
    MainTankStatus = 1;
    EEPROM.write(1, MainTankStatus);
  }

  if (StreetTankStatus == 255)
  {
    StreetTankStatus = 1;
    EEPROM.write(2, StreetTankStatus);
  }

  if (HotWaterTankStatus == 255)
  {
    HotWaterTankStatus = 1;
    EEPROM.write(3, HotWaterTankStatus);
  }

  if (MainPumpIsBlocked == 255)
  {
    MainPumpIsBlocked = 0;
    EEPROM.write(5, MainPumpIsBlocked);
  }

  if (MainPumpIsBlocked == true)
  {
    TurnOn(MainTankRelay);
  }

  if (StreetTankStatus = 3)
  {
    TurnOn(StreetTankRelay);
  }

  CurrentPage = 1;                // Инициализируем всегда 1-ю страницу при включении

  // Инициализируем PIN для кнопок
  pinMode(NextPageButtonPin, INPUT_PULLUP);

  // Инициализация входов/выходов (блок скважины)
  pinMode(SkvajinaPin, OUTPUT);
  pinMode(impulsePin, INPUT);
  attachInterrupt(0, flow, RISING);
  sei();

  // Инициализация дисплея HMI Nextion - установка по умолчанию скорости 115200
  // Serial.print("bauds=115200");
  // Serial.write( nex_msg_end, 3);
  nexInit();
  btnStart.attachPop(buttonStartCallback, &btnStart);

  /* Initialize DS3231 */
  clock.begin();
  // Set sketch compiling time
  //clock.setDateTime(__DATE__, __TIME__); // - занимает большой объем в памяти - исключил

}

void loop()
{
  /* Пример получения значений из консольного COM-порта - иногда необходим при отладке */
  //  if (Serial.available() > 0) {  //если есть доступные данные
  //    // считываем байт
  //    //StreetTank_WaterQuality = Serial.parseInt();
  //    HotWaterTankPercent = Serial.parseInt();
  //    // отсылаем то, что получили
  //    Serial.print("I received: ");
  //    //Serial.println(StreetTank_WaterQuality, DEC);
  //    Serial.println(HotWaterTankPercent, DEC);
  //  }

  // Считываем текущую дату и время
  dt = clock.getDateTime();

  // Опрос событий с HMI Nextion
  nexLoop(nex_listen_list);

  // Запрос к Slave-контроллеру для получения показаний датчиков
  i2cReadSlave();

  //Изменяем счетчик для анимации иконок, курсоров на OLED
  SystemCounters();

  // Обработка кнопок
  ProcessButtons();

  // Отображение уровня воды в емкостях
  MainTankLevel();
  StreetTankLevel();

  // Усредненный расчет температуры с термопары на MAX6675 (температура в баке с горячей водой) и с DS3231 (температура в доме)
  SaunaStonesTemperature();

  // Получение показаний температуры в доме с RTC
  //HomeAirTemp();

  /*-------------------- BEGIN Блок работы скважины/HMI Nextion ----------------*/
  TurbidityCheck();                   // Проверка прозрачности
  ScanImpulse();                      // Защита от сухого хода
  ProcessHMI();                       // Обновление вывода на экран Nextion
  /*--------------------- END Блок работы скважины/HMI Nextion -----------------*/

  /* Вывод страниц на LCD */
  ProcessPages();

  /* Обработка логики емкостей */
  Logic_StreetTank();
  Logic_MainTank();
  Logic_HotWaterTank();

  /* Запись значений в EEPROM */
  WriteStates();
}   /* END LOOP */

void ProcessHMI() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisHMI >= intervalHMI) {
    previousMillisHMI = currentMillis;

    n1.setValue(street_Temp);
    //n2.setValue(home_air_Temp);
    n2.setValue(clock.readTemperature());

    if (MainTankStatus == 2)
    {
      if (progress == 3)
      {
        q3.setPic(1);
      }
      else
      {
        q3.setPic(0);
      }
    }
    else
    {
      q3.setPic(0);
    }

    if (SkvajNasos == true) {
      if (numberProgressBar >= 100) {
        if (StreetTankPercent > 100) {
          numberProgressBar = 90;
        }
        else
        {
          numberProgressBar = StreetTankPercent;
        }
      }
      else
      {
        numberProgressBar = numberProgressBar + 2;
      }
      j0.setValue(numberProgressBar);
      n0.setValue(StreetTankPercent);
    }
    else
    {
      if (StreetTankPercent > 100) {
        numberProgressBar = 100;
      }
      else
      {
        numberProgressBar = StreetTankPercent;
      }
      n0.setValue(StreetTankPercent);
      j0.setValue(numberProgressBar);
    }
  }
}

/* Процедура проверки прозрачности воды
  Если насос работает, считываем показания датчика прозрачности.
  В случае если прозрачность меньше пороговой minTurbidity - вызываем процедуру "Грязной воды",
  иначе вызываем процедуру "Чистой воды".
*/
void TurbidityCheck()
{
  if (SkvajNasos == true)
  {
    Turbidity = analogRead(TurbiditySensorPin);
    if (Turbidity < minTurbidity)
    {
      oldimpulses = CommulativeImpulses;
      DirtyWater();
      q1.setPic(0);
      q2.setPic(1);
    }
    else
    {
      if (CommulativeImpulses - oldimpulses >= (litresKoeff * 2))
      {
        CleanWater();
        q1.setPic(1);
        q2.setPic(0);
      }
    }
    //Serial.println(Turbidity);
  }
  else
  {
    CloseWaterValves();
    if (updateHMI == true)
    {
      q0.setPic(0);
      q1.setPic(0);
      q2.setPic(0);
      btnStart.Set_background_crop_picc(0);
      btnStart.Set_press_background_crop_picc2(1);
      updateHMI = false;
    }
  }
}

/* Процедура защиты от сухого хода и непосредственного управления насосом скважины */
void ScanImpulse()
{
  unsigned long currentMillis = millis();

  if (ImpulseUpdated == true) {

    // Выводим текущие показания импульсов
    n3.setValue(CommulativeImpulses);

    // Выводим текущие показания литров
    n4.setValue(CommulativeImpulsesC / litresKoeff);
    n5.setValue(CommulativeImpulsesD / litresKoeff);

    if (progress == 1) {
      q0.setPic(1); //digitalWrite(ledPin, HIGH);
    }
    else
    {
      q0.setPic(0);
    }
    ImpulseUpdated = false;
  }

  if (currentMillis - previousMillisSKV >= intervalDryWork)
  {
    previousMillisSKV = currentMillis;
    if (SkvajNasos == true)
    {
      if ((impulses < 5) || (StreetTankPercent > 101))
      {
        SkvajNasos = false;
        updateHMI = true;
      }
      else
      {
        impulses = 0;
      }
    }
  }

  if (SkvajNasos == true)
  {
    TurnSkvajNasosOn();
  }
  else
  {
    TurnSkvajNasosOff();
  }
}

/* Процедуры управления силовой частью - РЕЛЕ - BEGIN */
void TurnSkvajNasosOn() {
  digitalWrite(SkvajinaPin, HIGH);
}

void TurnSkvajNasosOff() {
  digitalWrite(SkvajinaPin, LOW);
}

void DirtyWater() {
  TurnOff(CleanWaterRelay);
  TurnOn(DirtyWaterRelay);
}

void CleanWater() {
  TurnOff(DirtyWaterRelay);
  TurnOn(CleanWaterRelay);
}

void CloseWaterValves() {
  TurnOff(CleanWaterRelay);
  TurnOff(DirtyWaterRelay);
}
/* Процедуры управления силовой частью - РЕЛЕ - END */

// Получение уровня домашней емкости в %
void MainTankLevel()
{
  int oldsensorValue = cm1;
  cm1 = temp_cm1;
  //  Serial.print("CM1 before: ");
  //  Serial.print(cm1);
  //  Serial.println();

  if ((cm1 > MainTank_Height) || (cm1 < 0))
  {
    cm1 = MainTank_Height;
  }

  cm1 = (oldsensorValue * (averageFactor - 1) + cm1) / averageFactor;

  MainTankPercent = 100 - ((cm1 - MainTank_Max) * 100 / (MainTank_Height - MainTank_Max));             //Текущий уровень в %
  //  Serial.print("CM1 after: ");
  //  Serial.print(cm1);
  //  Serial.println();
}

// Получение уровня уличной емкости в %
void StreetTankLevel()
{
  int oldsensorValue = cm2;
  cm2 = temp_cm2;
  //Serial.print("CM2 before: ");
  //Serial.print(cm2);
  //Serial.println();

  if ((cm2 > StreetTank_Height) || (cm2 < 0))
  {
    cm2 = StreetTank_Height;
  }
  cm2 = (oldsensorValue * (averageFactor - 1) + cm2) / averageFactor;

  StreetTankPercent = 100 - ((cm2 - StreetTank_Max) * 100 / (StreetTank_Height - StreetTank_Max));       //Текущий уровень в %
  //  Serial.print("CM2 after: ");
  //  Serial.print(cm2);
  //  Serial.println();
}

// Обнуление (выключение) всех портов PCF8574
void PCFReset()
{
  pcf.digitalWrite(0, HIGH);
  pcf.digitalWrite(1, HIGH);
  pcf.digitalWrite(2, HIGH);
  pcf.digitalWrite(3, HIGH);
  pcf.digitalWrite(4, HIGH);
  pcf.digitalWrite(5, HIGH);
  pcf.digitalWrite(6, HIGH);
  pcf.digitalWrite(7, HIGH);
}

// Включение порта на PCF8574
void TurnOn(int val)
{
  pcf.digitalWrite(val, LOW);
}

// Выключение порта на PCF8574
void TurnOff(int val)
{
  pcf.digitalWrite(val, HIGH);
}

// Запись статусов емкостей и значений портов вывода (реле) в EEPROM
void WriteStates()
{
  EEPROM.write(0, HotWaterTankIsBlocked);
  EEPROM.write(1, MainTankStatus);
  EEPROM.write(2, StreetTankStatus);
  EEPROM.write(3, HotWaterTankStatus);
  EEPROM.write(5, MainPumpIsBlocked);
}

// Вывод страницы №1
void page1()
{
  int level1;
  char dom[] = {char(180), char(222), char(220), '\0'};
  String StrDT;

  //graphic commands to redraw the complete screen should be placed here
  u8g.drawRFrame(0, 0, 128, 16, 3);

  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(50, 12);
  u8g.print(dom);

  u8g.drawVLine(61, 16, 48);
  u8g.setFont(u8g_font_profont22);

  //Уровень домашней емкости (граф.)
  u8g.drawFrame(48, 18, 12, 46);
  level1 = 44 * MainTankPercent / 100;
  if (level1 == 0) {
    level1 = 1;
  }
  u8g.drawBox(50, 20 + (44 - level1), 8, level1);
  u8g.setPrintPos(6, 34);
  u8g.print(MainTankPercent);

  // Вывод на экран текущего времени
  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(64, 29);
  //StrDT = clock.dateFormat("H:i:s", dt);
  u8g.print(dt.hour); u8g.print(":");
  u8g.print(dt.minute); u8g.print(":");
  u8g.print(dt.second);

  u8g.setPrintPos(64, 43);
  u8g.print(dt.day); u8g.print("-");
  u8g.print(dt.month); u8g.print("-");
  u8g.print(dt.year - 2000);

  /* -----------------------------  Иконка блокировки горячей воды --------------------------------- */
  if ((HotWaterTankIsBlocked == true) && (progress == 1))
  {
    u8g.drawBitmapP(6, 36, 3, 16, ico_hotwater_blocked);
  }
  /* END --------------------------  Иконка блокировки горячей воды ---------------------------- END */

  /* -------------------- Иконка и значение прозрачности воды в уличной емкости ---------------------*/
  if ((wq_raw < 90) && (progress == 1))
  {
    u8g.drawBitmapP(80, 53, 2, 10, ico_eye);
  }
  /* END ---------------- Иконка и значение прозрачности воды в уличной емкости ---------------- END */

  //------------------ Статусы емкостей ------------------------------------------
  if (MainTankStatus == 1) {                                      // При нормальном статусе емкости - ОК
    u8g.drawBitmapP(6, 55, 1, 9, ico_ok_status);
  }

  if (MainTankStatus == 2) {                                      // Анимация при наполнении
    u8g.drawBitmapP(14 + (8 * progress), 54, 1, 10, right_arrow);
  }

  if ((MainTankStatus == 4) && (progress == 1)) {                 // Мигающая иконка при ошибке
    u8g.drawBitmapP(6, 53, 2, 11, ico_err_status);
  }

  if ((StreetTankStatus == 4) && (progress == 1)) {              // При ошибке анимация
    u8g.drawBitmapP(64, 53, 2, 11, ico_err_status);
  }

  // Отображение стрелок перехода по страницам
  //DrawMenuArrows();
}

// Вывод страницы №2
void page2()
{
  char sauna[] = {char(177), char(208), char(221), char(239), '\0'};

  // Оформление страницы
  u8g.drawRFrame(0, 0, 128, 16, 3);
  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(50, 12);
  u8g.print(sauna);
  //u8g.drawVLine(64, 16, 48);

  u8g.setPrintPos(20, 28);
  u8g.print("100");
  u8g.setPrintPos(20, 46);
  u8g.print(" 50");
  u8g.setPrintPos(20, 64);
  u8g.print("  0");

  if (tmpChange == false)
  {
    // Иконка термометра №1 - тем.воздуха
    u8g.drawBitmapP(62, 18, 2, 16, ico_air_temp);

    // Значение температуры воздуха в парилке
    u8g.setFont(u8g_font_profont22);
    u8g.setPrintPos(82, 34);
    u8g.print(sauna_air_Temp);
    u8g.print(char(176));

  }
  else
  {
    // Иконка термометра №2 - температура воды в баке с горячей водой
    u8g.drawBitmapP(62, 18, 2, 16, ico_water_temp);

    // Значение температуры воды в парилке
    u8g.setFont(u8g_font_profont22);
    u8g.setPrintPos(82, 34);
    u8g.print(sauna_water_Temp);
    u8g.print(char(176));
  }

  // Иконка термометра №3 - температура камней в каменке
  u8g.drawBitmapP(62, 45, 2, 16, ico_stones_temp);

  // Значение температуры камней
  u8g.setFont(u8g_font_profont22);
  u8g.setPrintPos(82, 61);
  u8g.print(sauna_stones_Temp);
  u8g.print(char(176));

  // Уровень емкости с горячей водой (градация 0-50-100%) - всего три уровня
  u8g.drawFrame(48, 18, 12, 46);
  u8g.drawHLine(45, 18, 15);
  u8g.drawHLine(45, 41, 15);
  u8g.drawHLine(45, 63, 15);

  // Прорисовываем уровень емкости
  if (HotWaterTankPercent == 1)
  {
    u8g.drawBox(48, 52, 12, 11);
  }
  if (HotWaterTankPercent == 2)
  {
    u8g.drawBox(48, 30, 12, 33);
  }
  if (HotWaterTankPercent == 3)
  {
    u8g.drawBox(48, 18, 12, 45);
  }

  //--------------------------------- Статус емкости ------------------------------------------
  if (HotWaterTankStatus == 1) {                         // При нормальном статусе емкости - ОК
    u8g.drawBitmapP(6, 55, 1, 9, ico_ok_status);
  }

  if (HotWaterTankStatus == 2) {                         // Анимация при наполнении
    u8g.drawBitmapP(8 * progress, 54, 1, 10, right_arrow);
  }

  if ((HotWaterTankStatus == 4) && (progress == 1)) {    // Мигающая иконка при ошибке
    u8g.drawBitmapP(0, 53, 2, 11, ico_err_status);
  }

  /* -----------------------------  Иконка блокировки горячей воды --------------------------------- */
  if ((HotWaterTankIsBlocked == true) && (progress == 1))
  {
    u8g.drawBitmapP(0, 35, 3, 16, ico_hotwater_blocked);
  }
  /* END --------------------------  Иконка блокировки горячей воды ---------------------------- END */

  /* Пример работы с форматной строкой - возможно понадобится в будущем */
  /* char text_buffer[16]; // Массив для вывода
    //  u8g.setFont(u8g_font_profont22);
    //  u8g.setPrintPos(26, 36);
    //  sprintf(text_buffer, "#1 %u %", MainTankPercent); //запись в буфер текста и значений
    //  u8g.print(text_buffer);
    //
    //  u8g.setPrintPos(26, 56);
    //  sprintf(text_buffer, "#2 %u %", StreetTankPercent); //запись в буфер текста и значений
    //  u8g.print(text_buffer); */

  // Отображение стрелок перехода по страницам
  //DrawMenuArrows();
}

// Вывод страницы №3
//void page3()
//{
//  char sauna_air_Temp[] = {char(177), char(208), char(221), char(239), ' ', 't', '1',  '\0'};
//
//  // Оформление страницы
//  u8g.drawRFrame(0, 0, 128, 16, 3);
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(40, 12);
//  u8g.print(sauna_air_Temp);
//
//  // Иконка термометра №1 - тем.воздуха
//  u8g.drawBitmapP(2, 20, 2, 16, ico_air_temp);
//
//  // Значение температуры воздуха в парилке
//  //u8g.setFont(u8g_font_fub14n);
//  u8g.setFont(u8g_font_profont22);
//  u8g.setPrintPos(2, 63);
//  u8g.print(sauna_stones_Temp + 53);
//
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(20, 28);
//  u8g.print("120");
//  u8g.setPrintPos(20, 46);
//  u8g.print(" 50");
//  u8g.setPrintPos(20, 64);
//  u8g.print("  0");
//
//  // Разметка графика температуры
//  u8g.drawFrame(48, 18, 79, 46);
//  u8g.drawHLine(45, 18, 82);
//  u8g.drawHLine(45, 41, 82);
//  u8g.drawHLine(45, 63, 82);
//
//  u8g.drawLine(50 + x1, 63, 50 + x1, y1);
//
//  // Отображение стрелок перехода по страницам
//  //u8g.drawBitmapP( 6 - progress, 3, 1, 10, left_arrow);
//  //u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
//  DrawMenuArrows();
//}

// Вывод страницы SleepPage
//void SleepPage()
//{
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(36, 36);
//  u8g.print(clock.dateFormat("d F Y H:i:s",  dt));
//}

// Счетчик для анимации графических элементов на страницах дисплея
void SystemCounters()
{
  unsigned long currentMillis       =  millis();

  /* Управление уровнем яркости LCD дисплея - вход в "спящий режим" */
  if ((currentMillis - previousMillisBLK >= BrightnessInterval) && (SleepMode == false)) {
    previousMillisBLK = currentMillis;
    LED12864_Brightness(SleepBrightness);
    SleepMode = true;
  }

  /* Переключение отображения температуры воздуха/воды на странице "БАНЯ" */
  if (currentMillis - previousMillisTMP >= temperature_change_timer)  {
    previousMillisTMP = currentMillis;
    if (tmpChange == false) {
      tmpChange = true;
    }
    else
    {
      tmpChange = false;
    }
  }

  /* Счетчик progress - для анимации процессов на LCD-экране */
  if (progress < 3 ) {
    progress++;
  }
  else
  {
    progress = 1;
  }
}

// Обработка нажатия кнопок
void ProcessButtons()
{
  int NextPageButtonState = digitalRead(NextPageButtonPin);
  if ((NextPageButtonState == LOW) && (LongTapNext == 0))       // Кнопка нажата (первое нажатие выводит экран из режима сна, последующие переключают страницы)
  {
    if (SleepMode == false)
    {
      //LongTapNext = 1;
      CurrentPage++;

      if (CurrentPage > 2) {
        CurrentPage = 1;
      }
      //previousMillisBLK = millis();
    }
    else
    {
      //LongTapNext = 1;
      SleepMode = false;
      LED12864_Brightness(WorkBrightness);
      setMaxBrightnessNex();
      tm0.enable();
      //previousMillisBLK = millis();
    }
    LongTapNext = 1;
    previousMillisBLK = millis();
  }

  if ((NextPageButtonState == HIGH) && (LongTapNext == 1)) {        // Кнопка отжата
    LongTapNext = 0;
  }
}

// Условия для вывода нужной страницы на LCD
void ProcessPages()
{
  if (CurrentPage == 1)
  {
    u8g.sleepOff();
    u8g.firstPage();
    do {
      page1();                    // Отображение страницы №1 на LCD
    } while ( u8g.nextPage() );
  }

  if (CurrentPage == 2)
  {
    u8g.sleepOff();
    u8g.firstPage();
    do {
      page2();                    // Отображение страницы №2 на LCD
    } while ( u8g.nextPage() );
  }
}

// Отображение на страницах стрелок меню "Влево"/"Вправо"
//void DrawMenuArrows()
//{
//  // Отображение стрелок перехода по страницам
//  u8g.drawBitmapP( 6 - progress, 3, 1, 10, left_arrow);
//  u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
//}

/* ----------------------- БЛОК ЛОГИКИ ЕМКОСТЕЙ ---------------------------- */
// Логика уровней/событий домашней емкости
void Logic_MainTank()
{
  // Наполнение
  if (MainTankPercent < MainTank_FillIn)
  {
    if (StreetTankStatus != 4)
    {
      TurnOn(StreetTankRelay);
      MainTankStatus = 2;
    }
    else
    {
      TurnOff(StreetTankRelay);
      MainTankStatus = 4;
    }
  }

  // Достигнут максимальный уровень
  if (MainTankPercent >= 100)
  {
    TurnOff(StreetTankRelay);     //Выключаем насос уличной емкости
    MainTankStatus = 1;           //Домашняя емкость - состояние "НОРМА"

    TurnOff(MainTankRelay);       //Выключаем реле блокировки насосной станции
    MainPumpIsBlocked = false;    //Снимаем программную блокировку насосной станции
  }

  // Если уровень ниже уровня блокировки эл.клапана емкости с горячей водой - блокировка клапана
  if (MainTankPercent < MainTank_MinHotWater)
  {
    HotWaterTankIsBlocked = true;
  }
  else if (MainTankPercent > HotWater_Unblock)
  {
    HotWaterTankIsBlocked = false;
  }

  // Если уровень в домашней емкости ниже минимума
  if (MainTankPercent < MainTank_MinBlock)
  {
    TurnOn(MainTankRelay);       //Включаем реле блокировки насосной станции
    MainPumpIsBlocked = true;    //Устанавливаем программную блокировку насосной станции

    if (MainTankStatus != 2 )
    {
      MainTankStatus = 4;
    }
  }
  else
  {
    TurnOff(MainTankRelay);       //Выключаем реле блокировки насосной станции
    MainPumpIsBlocked = false;    //Снимаем программную блокировку насосной станции
    if ((MainTankStatus != 2) && (MainTankStatus != 4))
    {
      MainTankStatus = 1;
    }
  }
}

// Логика уровней/событий уличной емкости
void Logic_StreetTank()
{
  // Если уровень уличной емкости достиг критического минимума - запускаем режим наполнения из скважины */
  if (StreetTankPercent < StreetTank_MinBlock)
  {
    //TurnOn(ChinkRelay);       /* Включение реле скважинного насоса */
    StreetTankStatus = 4;
  }
  else
  {
    StreetTankStatus = 1;
  }

  if (StreetTankPercent >= 100 )
  {
    //TurnOff(ChinkRelay);      /* Выключение реле сважинного насоса */
    StreetTankStatus = 1;
  }

  CheckWaterQuality();
}

// Логика уровней/событий домашней емкости
void Logic_HotWaterTank()
{
  // Наполнение

  if (HotWaterTankIsBlocked == false)
  {
    if (HotWaterTankPercent < 2)
    {
      if (sauna_water_Temp < 40)
      {
        TurnOn(HotWaterTankRelay);      // Открываем электромагнитный клапан
        HotWaterTankStatus = 2;         // Состояние емкости "Наполнение"
      }

    }
    else if (HotWaterTankPercent < 1)
    {
      TurnOn(HotWaterTankRelay);      // Открываем электромагнитный клапан
      HotWaterTankStatus = 2;         // Состояние емкости "Наполнение"
    }
  }
  else
  {
    TurnOff(HotWaterTankRelay);     // Закрываем электромагнитный клапан
    HotWaterTankStatus = 4;         // Состояние емкости "Ошибка"
  }

  //  if ((HotWaterTankPercent < 2) || (sauna_water_Temp < 40))
  //  {
  //    if (HotWaterTankIsBlocked == false)
  //    {
  //      TurnOn(HotWaterTankRelay);      // Открываем электромагнитный клапан
  //      HotWaterTankStatus = 2;         // Состояние емкости "Наполнение"
  //    }
  //    else
  //    {
  //      TurnOff(HotWaterTankRelay);     // Закрываем электромагнитный клапан
  //      HotWaterTankStatus = 4;         // Состояние емкости "Ошибка"
  //    }
  //  }
  //
  //  if (HotWaterTankPercent <= 1)
  //  {
  //    if (HotWaterTankIsBlocked == false)
  //    {
  //      TurnOn(HotWaterTankRelay);      // Открываем электромагнитный клапан
  //      HotWaterTankStatus = 2;         // Состояние емкости "Наполнение"
  //    }
  //    else
  //    {
  //      TurnOff(HotWaterTankRelay);     // Закрываем электромагнитный клапан
  //      HotWaterTankStatus = 4;         // Состояние емкости "Ошибка"
  //    }
  //  }

  // Достигнут максимальный уровень
  if (HotWaterTankPercent == 3)
  {
    TurnOff(HotWaterTankRelay);       // Закрываем электромагнитный клапан
    HotWaterTankStatus = 1;           // Состояние емкости "Норма"
  }
}

// Проверка условий прозрачности воды в уличной емкости
void CheckWaterQuality()
{
  if (wq_raw < 90)
  {
    StreetTankStatus = 4;
  }
  else
  {
    if ((StreetTankStatus != 2) && (StreetTankStatus != 4))
    {
      StreetTankStatus = 1;
    }
  }
}
/* END ------------------- БЛОК ЛОГИКИ ЕМКОСТЕЙ ------------------------ END */

void LED12864_Brightness(int BL_brightness)
{
  analogWrite(BLK, BL_brightness);
}

// Температура в доме
//void HomeAirTemp()
//{
// home_air_Temp = clock.readTemperature();
//}

// Температура бака с горячей водой
// также применяем коэффициент сглаживания показаний (как и при работе с ультразвуковыми датчиками) - для устранения "шума"
void SaunaStonesTemperature()
{
  int oldsensorValue = sauna_stones_Temp;

  if (progress == 1 )
  {
    sauna_stones_Temp = sauna_stones_temp_Raw;
    sauna_stones_Temp = (oldsensorValue * (averageFactor - 1) + sauna_stones_Temp) / averageFactor;
  }
}

void i2cReadSlave()
{
  const int argCount = 10;             // Количество аргументов массива получаемых со Slave-контроллера !!!Обязательно проверить размерность на соответствие с кодом Slave'а
  int respVals[argCount];              // Создаем переменную (массив - размерность = argCount) для получения данных со Slave-контроллера
  Wire.requestFrom(8, argCount);       // Запрашиваем "argCount" байт из Slave-контроллера по i2c адрес #8
  uint8_t respIoIndex = 0;

  while (Wire.available())
  {
    for (byte r = 0; r < argCount; r++)
      if (Wire.available()) {
        respVals[respIoIndex] = (uint8_t)Wire.read();
        respIoIndex++;
      }
      else {
        // log or handle error: "missing read"; if you are not going to do so use r index instead of respIoIndex and delete respoIoIndex from this for loop
        break;
      }

    temp_cm1            = respVals[0];    // Значение уровня в см. для домашней емкости
    temp_cm2            = respVals[1];    // Значение уровня в см. для уличной емкости
    HotWaterTankPercent = respVals[2];    // Значение уровня в баке с горячей водой

    /* Значение температуры с термопары MAX6675 */
    dataA = respVals[3];
    dataB = respVals[4];

    sauna_stones_temp_Raw = (respVals[4] << 8) | respVals[3];
    //sauna_stones_temp_Raw  = dataA +  averageFactor + (dataB * 0.1);   // Собираем float из целой и дробной частей

    wq_raw = respVals[5];

    sauna_air_Temp    = respVals[6];
    sauna_water_Temp  = respVals[7];
    street_Temp       = respVals[8];
    //home_air_Temp     = respVals[9]; - можно использовать на что то другое - теперь температура на Мастере
    //Serial.println(street_Temp);
  }
}
