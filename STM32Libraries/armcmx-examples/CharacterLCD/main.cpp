#include <stm32f4xx.h>
#include <stm32f4xx_it.h>

#include "armcmx.h"
#include "stm32f4_discovery.h"

#include "LiquidCrystal.h"

LiquidCrystal lcd(PD0, PD1, PE8, PE9, PE10, PE11);
 
int main(void) {
  
  armcmx_init();
  
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Hello there!");
}
