#define F_CPU 9600000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>


#define FIRST_PLUS_MIN 187
#define FIRST_PLUS_MAX 196

#define FIRST_MINUS_MIN 120
#define FIRST_MINUS_MAX 137

#define SECOND_PLUS_MIN 167
#define SECOND_PLUS_MAX 174

#define SECOND_MINUS_MIN 0
#define SECOND_MINUS_MAX 10


#define COOLER0 PB0 // OC0A
#define COOLER1 PB1 // OC0B
#define KEYBOARD_IN PB2 //ADC1

#define PWM_ARR_SIZES 8
#define PWM_ARR_MAX (PWM_ARR_SIZES-1)

volatile uint8_t keyboard_value = 255;

uint8_t EEMEM first = PWM_ARR_MAX;
uint8_t EEMEM second = PWM_ARR_MAX;
uint8_t EEMEM crc = 0;

uint8_t first_value = 0;
uint8_t second_value = 0;
uint8_t crc_t = 0;

uint8_t pwm_values[PWM_ARR_SIZES] = {
	32, 64, 96, 128,
	160, 192, 224, 255
};

ISR(ADC_vect)
{
	keyboard_value = ADCH;
}

int main(void)
{
	DDRB |= (1 << COOLER0)|(1 << COOLER1); // Выходы
	PORTB &= ~((1 << COOLER0)|(1 << COOLER1)); // Низкий уровень
	// Таймер для ШИМ
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	
	
	TCNT0 = 0; // Сброс значение счётчика
	
	while (!eeprom_is_ready()){}
	//Восстановление значений ШИМ
	first_value =  eeprom_read_byte(&first);
	second_value =  eeprom_read_byte(&second);
	crc_t =  eeprom_read_byte(&crc);
	if(crc_t != first_value+second_value || (first_value > PWM_ARR_MAX || second_value > PWM_ARR_MAX) || crc_t == 0xFF){
		eeprom_write_byte(&first, PWM_ARR_MAX);
		eeprom_write_byte(&second, PWM_ARR_MAX);
		eeprom_write_byte(&crc, PWM_ARR_MAX*2);
		first_value = second_value =  PWM_ARR_MAX;
	}
	
	
	OCR0A = pwm_values[first_value];
	OCR0B = pwm_values[second_value];
	
	TCCR0B = (1 << CS00); // Без делителя
	ADMUX = 1 | (1 << ADLAR); // VCC REF, ADC2
	ADCSRA = ((1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADIE) | (1 << ADPS1)); // АЦП включен, запуск преобразования, режим автоизмерения, прерывание по окончанию преобразования, частота CLK/4
	ADCSRB = 0; //Режим автоизмерения: постоянно запущено
	sei();
	while(1)
	{
		cli();
		uint8_t val_key = keyboard_value;
		sei();
		
		if(val_key >= FIRST_PLUS_MIN && val_key <= FIRST_PLUS_MAX){ //Плюс первого канала
			if(first_value != PWM_ARR_MAX){
				OCR0A = pwm_values[++first_value];
				eeprom_write_byte(&first, first_value);
				eeprom_write_byte(&crc, first_value+second_value);
			}
			} else if(val_key >= FIRST_MINUS_MIN && val_key <= FIRST_MINUS_MAX) { //Минус первого канала
			if(first_value != 0){
				OCR0A = pwm_values[--first_value];
				eeprom_write_byte(&first, first_value);
				eeprom_write_byte(&crc, first_value+second_value);
			}
			} else if(val_key >= SECOND_PLUS_MIN && val_key <= SECOND_PLUS_MAX) { //Плюс первого канала
			if(second_value != PWM_ARR_MAX){
				OCR0B = pwm_values[++second_value];
				eeprom_write_byte(&second, second_value);
				eeprom_write_byte(&crc, first_value+second_value);
			}
			} else if(val_key >= SECOND_MINUS_MIN && val_key <= SECOND_MINUS_MAX) { //Минус первого канала
			if(second_value != 0){
				OCR0B = pwm_values[--second_value];
				eeprom_write_byte(&second, second_value);
				eeprom_write_byte(&crc, first_value+second_value);
			}
		}
		val_key = 255;
		_delay_ms(100);
	}
}