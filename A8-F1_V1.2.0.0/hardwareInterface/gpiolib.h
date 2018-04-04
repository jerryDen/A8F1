#ifdef 		__GPIOLIB_H__
#define 	__GPIOLIB_H__



#define PH(num)  



struct Gpio_Ops{
	int (*setOutValue)(struct Gpio_Ops * ops, int value);
	int (*getInputValue)(struct Gpio_Ops * ops);

}Gpio_Ops,*pGpio_Ops;
pGpio_Ops createGpioServer(int gpioPin);
void destroyGpioServer(pGpio_Ops *server); 

#endif

























