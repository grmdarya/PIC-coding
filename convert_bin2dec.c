# Ввести и преобразовать двоичное чило в десятичное.
# Запись производится во внешнюю энергонезависимую память. Если число переводится – срабатывает звонок.
# При помощи кнопки 4 устанавливаем курсор. 
# Данные битов задаются нажатием кнопки 5.
# При помощи кнопки 6, происходит преобразование двоичного число в десятичное.
# Сохранение данных, производится при помощи кнопки 7.
# Микроконтроллер общается с перифирией по средством I2C.

#include    <xc.h>
#include    <pic.h>
#include <string.h>
#include <math.h>
__CONFIG(0x3972);
#define byte unsigned char
#define Freq 20 

#define  testbit(var, bit)   ((var) & (1 <<(bit))) //проверка уровня напряжения на SCL и SDA
#define  setbit(var, bit)    ((var) |= (1 << (bit))) //устанавливает в бите с номером bit var-а единицу
#define  clrbit(var, bit)    ((var) &= ~(1 << (bit))) //устанавливает в bit-ом бите var-а ноль (~ - отрицание)

#define SCL 3    // 3 бит в конфигурации 
#define SDA 4   // 4 бит в конфигурации
#define cache_size_I2C 0x40  // размер буфера  I2C

byte Ch_ACK; //флаг определяющий уровень напряжения на SDA
byte tmp_buffer_I2C, Slave_ADR_RW_I2C, tmp_I2C;
unsigned int Adr_I2C;

//LCD
void Send_Command_LCD(byte tmp); //программа инифиализации
void Send_B_LCD(byte tmp);   // отправить в LCD
void Set_Coord_LCD(byte i, byte j);   //установка курсора в нужное место назначения
void Show_String_LCD(const char * mySTRING);  //вывод строки(символов) на экран

byte Current_ind = 0;

static const char str_BLANK[] = "                ";
static const char str_save[] = "      Save      ";
static const char load_old_data[] = "Load from M?  <-YES  OR  NO->";


char str_line0[] = { 0x30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char str_line1[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

char in_p = 0, in_c = 0, line = 0;
byte button = 0;



//Проверка нажатия кнопок: кнопки нажаты = в малдшых 4 разрядах еденицы (1111 - 7,6,5,4 кнопки)
byte Check_buttons(void) {
	byte tmp, tmp_PORTB = PORTB, tmp_TRISB = TRISB;

	TRISB &= 0x0F; //установить на 4-7 кнопки 1
	PORTB |= 0xF0; //установить на 4-7 кнопки 0

	tmp = PORTB ^ 0xFF;

	PORTB = tmp_PORTB; TRISB = tmp_TRISB; //

	return(tmp >> 4);
}

void Delay(unsigned int tmp) { //общая задержка 

	while (tmp--);
	return;
}

void Delay10(int tmp) {     //задержка для сохранения и вывода на экран
	while (tmp--) {
		Delay(3126);
	}
	return;
}
//Функция для подачи входного синхросигнала, считывает данных со входов

void Pulse_LCD(unsigned int x) {
	RB2 = 1;  //подключение двунаправленного порта ввода/вывода с подтягивающим резистором
	Delay(x);  //задержка
	RB2 = 0;   //сброс 
	Delay(x);
}
// Функция инициализации ЖКИ
void Init_LCD(void) {
	Delay(200 * Freq);
	TRISB = 0;   //переключение порта B на вывод
	PORTB = 0x30;
	Pulse_LCD(20 * Freq);
	Pulse_LCD(20 * Freq);
	Pulse_LCD(20 * Freq);

	PORTB = 0x20;
	Pulse_LCD(20 * Freq);
	//Режим отображения 2-х строк с матрицей 5 на 8 точек и работу с 4-х разрядной шиной данных

	Send_Command_LCD(0x28);
	Send_Command_LCD(0x0C);
	//включает отображение на экране ЖКИ-модуля, без отображения курсоров
	Send_Command_LCD(0x06);
	//устанавливает режим автоматического перемещения курсора слева-направо после вывода каждого символа
	Send_Command_LCD(0x02);
}
/*
* подпрограмма для пересылки старшего и младшего байтов команды в ЖКИ
* необходима для ф-ций Send_Command и Send_Byte
*/
void Send_B_LCD(byte tmp) {
	//пока нажаты кнопки - задержка
	while (Check_buttons()) Delay(500 * Freq);
	// если кнопки отжаты- пересылка старшего байта команды/данных
	PORTB = (PORTB & 0x0F) + (tmp & 0xF0); 240 + 15
		Pulse_LCD(2 * Freq);
	// пересылка младшего байта команды/данных
	PORTB = (PORTB & 0x0F) + (tmp << 4); 15
		Pulse_LCD(2 * Freq);
}
//Передача в ЖКИ данных
void Send_Byte_LCD(byte tmp) {
	// посылаем байт с данными (RS = 1)
	Send_B_LCD(tmp);
	Current_ind++; // переменная показывает, в какое место ЖКИ осуществляется вывод

	if (Current_ind == 16) Set_Coord_LCD(1, 0); //если последнее место в строке, переходим на следующую строку
	if (Current_ind == 32) Set_Coord_LCD(0, 0); //если последнее место во 2 строке, возврат на 1 строку, 1 позиция
}
//Передача в ЖКИ команды
void Send_Command_LCD(byte tmp) {
	RB1 = 0; // перевод RS в 0 для передачи команды
	Send_B_LCD(tmp);
	RB1 = 1; // возврат в начальное состояние
	Delay(250 * Freq);
}
// Очистка экрана
void Clr_LCD(void) {
	Set_Coord_LCD(0, 0);
	Show_String_LCD(str_BLANK);
	Set_Coord_LCD(0, 0);
}
//Позиционирование курсора на нужную строку
void Set_Coord_LCD(byte i, byte j) {
	// 1 строка
	if (i == 0) {
		Current_ind = j;
		Send_Command_LCD(0x80 + j);
		// 2 строка
	}
	else {
		Current_ind = 16 + j;
		Send_Command_LCD(0xC0 + j);
	};
	return;
}
// Вывод на дисплей содержимого строковых констант
void Show_String_LCD(const char * mySTRING) {
	while (*mySTRING) {
		Send_Byte_LCD(*(mySTRING++));
	};
}


//I2С - 2 проводника, подтянутых к +5В через резисторы с сопротивлением в несколько КОм
//SCL - передача импульсов синхронизации
//SDA - передача данных между устройствами

void LOW_SCL_I2C(void) {//установка SCL на низкое напряжение
	//устанавливаем в 3 бит(SCL) регистра PORTC 0
	clrbit(PORTC, SCL); // устанавливаем в 3 бит(SCL) регистра PORTC 0
	Delay(5);
}

void HIGH_SCL_I2C(void) {//установка SCL на высокое напряжение
	//устанавливаем в 3 бит(SCL) регистра PORTC 1
	setbit(PORTC, SCL); Delay(5);
}

void LOW_SDA_I2C(void) {//установка SDA на низкое напряжение
	//устанавливаем в 4 бит(SDA) регистра PORTC 0
	clrbit(PORTC, SDA);
	clrbit(TRISC, SDA);
	Delay(5);
}

void HIGH_SDA_I2C(void) {// на высокое напряжение
	//устанавливаем в 4 бит(SDA) регистра PORTC 1
	setbit(TRISC, SDA);
	Delay(5);
}

void CLOCK_PULSE_I2C(void) { // синхроимпульс на SCL
	HIGH_SCL_I2C();
	LOW_SCL_I2C();
}
/*
* отзыв устройства сигналом АСК
* выдает нулевой бит на SDA во время действия девятого синхроимпульса
* посылается после каждого переданного байта(кроме завершения чтения)
*/
void ACK_I2C(void) {
	LOW_SDA_I2C();
	CLOCK_PULSE_I2C();
}
/*
* сигнал nack(no ack - отсутствие подтверждения)
* выдаёт спец комбинацию сигналов на sda scl для завершения чтения
*/
void NACK_I2C(void) {
	HIGH_SDA_I2C();
	CLOCK_PULSE_I2C();
}

// стартовая комбинация - сигнал START, подается для начал организации связи по I2C
void START_I2C(void) {
	HIGH_SDA_I2C(); HIGH_SCL_I2C();
	//перевод линии SDA из состояния высокого напряжения в низкое, на SCL высокое напряжение
	LOW_SDA_I2C(); LOW_SCL_I2C();
}
// сигнал стоп, конец связи по I2C
void STOP_I2C(void) {
	LOW_SDA_I2C(); LOW_SCL_I2C();
	//перевод линии SDA из нулевого состояния в состояние высокого напряжения, на SCL высокий уровень напряжения
	HIGH_SCL_I2C(); HIGH_SDA_I2C(); LOW_SCL_I2C();
}

//проверка напряжения на sda
void Check_ACK_I2C(void) {
	HIGH_SCL_I2C(); // устанавливаем высокое напряжение на SCL
	if (testbit(PORTC, SDA))
		Ch_ACK = 1; // высокое
	else Ch_ACK = 0; // низкое
	LOW_SCL_I2C();
}
//реализация передачи байта данных с помощью изменения уровня напряжения на SDA
void OUT_BYTE_I2C(byte t) {
	byte tmp;
	tmp = 8;
	while (tmp--) {
		if (t & 0x80)HIGH_SDA_I2C(); else LOW_SDA_I2C();
		CLOCK_PULSE_I2C(); t += t;
	}
	HIGH_SDA_I2C(); Check_ACK_I2C();
}
//Отправка ЗАПРОСНОГО БАЙТА
//ЗБ = адресное поля(4 бита - тип устройства, 3 бита - номер конкретной схемы) и бита направления передачи (0-запись, 1-чтение)

void Send_Slave_Addr_I2C(void) {
	int tmp;
Rep:
	OUT_BYTE_I2C(Slave_ADR_RW_I2C);//Получаем ответ от ведомого с опр. адресом
	NACK_I2C();
	if (testbit(PORTB, SDA)) {
		STOP_I2C(); START_I2C(); goto Rep;
	};
	tmp_buffer_I2C = cache_size_I2C - (Adr_I2C & 63);
}

//запись в буфер байта, который после завершения передачи(после сигнала STOP) из этого буфера будет записан в память (размер буфера = 64 байта)

void Write_I2C(byte tmp) {
	OUT_BYTE_I2C(tmp);
	SSPIF = 0;
	SSPBUF = tmp;
	while (SSPIF == 0);
	while (ACKSTAT);
	SSPIF = 0;
}

void Start_I2C(void) {
	START_I2C();            //Посылаем комбинацию START на шину I2C
	Send_Slave_Addr_I2C();  //Посылаем на шину запросный байт, содержащий тип, номер устройства и бит операции

	if ((Slave_ADR_RW_I2C & 1) == 0) { //Если 8 бит запросного байта 0,то запись в память
		if (!(Slave_ADR_RW_I2C & 16))
			Write_I2C((byte)(Adr_I2C >> 8));
		Write_I2C((byte)Adr_I2C);
	}
}

void Stop_I2C(void) {//Остановка операции
	STOP_I2C();     //Выставляем линии SDA/SCL в нужное положение
	tmp_buffer_I2C = cache_size_I2C - (Adr_I2C & 63);//вычисляем и записываем свободное пространство в буфере
	return;
}

void Init_Write_I2C(unsigned int uAdr_I2C)
{
	STOP_I2C();
	Adr_I2C = uAdr_I2C;
	tmp_buffer_I2C = cache_size_I2C - (Adr_I2C & 63);
	Slave_ADR_RW_I2C &= 0xFE; //режим записи
	START_I2C();

	return;
}
//инициализация перед записью данных в память
void Init_WRITE_I2C(unsigned int Adr_begin) {
	Adr_I2C = Adr_begin;
rep:
	START_I2C(); Slave_ADR_RW_I2C &= 0xFE;
	OUT_BYTE_I2C(Slave_ADR_RW_I2C);
	if (Ch_ACK) {
		STOP_I2C();
		goto rep;
	}
	OUT_BYTE_I2C(Adr_begin >> 8);
	if (Ch_ACK) {
		STOP_I2C(); goto rep;
	}
	OUT_BYTE_I2C(Adr_begin);
	if (Ch_ACK) {
		STOP_I2C(); goto rep;
	}
}

void OUT_BYTE_PAGE_I2C(byte tmp) { //
	OUT_BYTE_I2C(tmp);
	Adr_I2C++;
	if ((cache_size_I2C - 1) & Adr_I2C)
		return;
	STOP_I2C();
	Init_WRITE_I2C(Adr_I2C);
	return;
}

//инициализация чтения из памяти
void Init_READ_I2C(unsigned int Adr_begin) {
	Init_WRITE_I2C(Adr_begin);//имитируем запрос на запись
	START_I2C();//выставляем стартовую конфигурацию
	Slave_ADR_RW_I2C |= 1;//адрес
	OUT_BYTE_I2C(Slave_ADR_RW_I2C);
}

byte IN_BYTE_I2C(void) {
	byte t, tmp = 8;
	t = 0; HIGH_SDA_I2C();
	while (tmp--) {
		t += t;
		HIGH_SCL_I2C();
		if (testbit(PORTC, SDA))t++; LOW_SCL_I2C();
	}
	return(t);
}

byte IN_BYTE_ACK_I2C(void) {
	byte t; t = IN_BYTE_I2C(); ACK_I2C();
	return(t);
}

byte IN_BYTE_NACK_STOP_I2C(void) {//завершение операции чтения из памяти: МК формирует NACK
	byte t;
	t = IN_BYTE_I2C();
	NACK_I2C();
	STOP_I2C();
	return(t);
}

//Инициализация EEPROM
void init_I2C() {
	RC4 = 0;
	// установка SDA на низк уровень. SDA установл по высокому значению на TRIS
	TRISC3 = 0;
	TRISC4 = 0;
}

void init_eeprom() {
	//выбираем номер переферийного устройства - EEPROM
	Slave_ADR_RW_I2C = 0xA0;
	TRISC = 0x9B;
	init_I2C();
}

//функция записи данных в EEPROM
void save_data() {
	Init_WRITE_I2C(0);//запись с адреса 0

	for (int i = 0; i < 16; i++) {//запись строчку 0
		OUT_BYTE_PAGE_I2C(str_line0[i]);
	}

	for (int i = 0; i < 16; i++) {//запись строчку 1
		OUT_BYTE_PAGE_I2C(str_line1[i]);
	}

	OUT_BYTE_PAGE_I2C(in_c);//последний введённый символ
	OUT_BYTE_PAGE_I2C(in_p);//последняя позиция
	OUT_BYTE_PAGE_I2C(line);//номер строчки
	STOP_I2C();//Завершение операции записи
}
// загрузка данных из памяти
void load_data() {
	Init_READ_I2C(0);//Чтение с адреса 0

	for (int i = 0; i < 16; i++) {//Чтение строчки 0
		str_line0[i] = IN_BYTE_ACK_I2C();
	}

	for (int i = 0; i < 16; i++) {//Чтение строчки 1
		str_line1[i] = IN_BYTE_ACK_I2C();
	}

	in_c = IN_BYTE_ACK_I2C();//Чтение последнего символа
	in_p = IN_BYTE_ACK_I2C();//Чтение последней позиции
	line = IN_BYTE_ACK_I2C();//Чтение строчки, с которой в последний раз работали

	IN_BYTE_NACK_STOP_I2C();//Завершение операции чтения
}

// функция звонка
void Beep(void) {
	byte tmp_TRISB = TRISB, tmp_PORTB = PORTB, i;
	TRISB3 = 0;
	i = 12 * Freq;
	while (i--) {
		RB3 = 1; Delay(8 * Freq);
		RB3 = 0; Delay(8 * Freq);
	}
	PORTB = tmp_PORTB;
	TRISB = tmp_TRISB;
}

// считывание кнопок, выполнение функции по кнопкам, вывод на дисплей
void work() {

	Delay(3126);//10mc
	button = 0;
	button = Check_buttons();
	// если нажата кнопка
	if (button != 0x0) {
		//посимвольный ввод
		if (button == 0x1) {//увеличиваем число
			if (in_c == 1) {
				in_c = 0;
			}
			else {
				in_c++;
			}
		}

		if (button == 0x2) {//переход с следующему символу
			if (in_p != 16) {//если символов 16, конец ввода
				in_c = 0;
				in_p++;
			}
		}

		str_line0[in_p] = in_c + 0x30;
		mass[in_p] = in_c;
		str_line0[in_p + 1] = 0;

		if (button == 0x4)
		{
			int sum = 0;

			for (int i = sizeof(mass) / sizeof(*mass) - 1; i > -1; i--)
			{
				if (mass[i] == 1)   // если i-й бит в строке не нулевой
				{
					sum += pow(2, i);// суммировать 2 в i-ой степени
				}
			}

			int temp;
			int n = 0;
			int buf = sum;

			while (buf > 0)
			{
				arr[n] = buf % 10;
				buf = buf / 10;
				n++;
			}

			for (int i = 0; i <= n / 2; i++)
			{
				temp = arr[i];
				arr[i] = arr[n - i - 1];
				arr[n - i - 1] = temp;
			}

			for (int i = 0; i < n; i++)
			{
				str_line1[i] = arr[in_p] + 0x30;
			}

			Beep();
		}

		if (button == 0x8) {
			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_save);
			Show_String_LCD(str_BLANK); //вывод строки
			Set_Coord_LCD(0, 0);

			save_data();

			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_BLANK);
			Show_String_LCD(str_BLANK);
			Set_Coord_LCD(0, 0);

			Delay10(100);//задержка 1 s
		}

		//каждое выполнение функции вывод на дисплей
		Set_Coord_LCD(0, 0);
		Show_String_LCD(str_line0);
		Set_Coord_LCD(1, 0);
		Show_String_LCD(str_line1);
	}
}

void main(void) {
	TRISD = 0;
	TRISB = 0;
	PORTD = 0;
	Delay10(30);
	//инициализация дисплея
	Init_LCD();
	//очистка дисплей
	Set_Coord_LCD(0, 0);
	Show_String_LCD(str_BLANK);
	Show_String_LCD(str_BLANK);
	Set_Coord_LCD(0, 0);

	//инициализируем EEPROM
	init_eeprom();

	//загрузить сохранение?
	Show_String_LCD(load_old_data);
	byte tmp = 0;
	while (1) {
		tmp = Check_buttons();
		if (tmp == 0x1) {
			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_BLANK);
			Show_String_LCD(str_BLANK);
			Set_Coord_LCD(0, 0);

			load_data();
			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_line0);
			Set_Coord_LCD(1, 0);
			Show_String_LCD(str_line1)
				break;
		}
		if (tmp == 0x8) {
			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_BLANK);
			Show_String_LCD(str_BLANK);
			Set_Coord_LCD(0, 0);
			Show_String_LCD(str_line0);

			break;
		}
	}

	while (1) {
		work();
	}
}
