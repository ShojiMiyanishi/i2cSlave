/*
 * dma.h
 *
 *  Created on: 2018/02/19
 *      Author: omiya
 */

#ifndef DMA_H_
#define DMA_H_

#include "cmsis.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMA_1 = (int)DMA1_BASE,
    DMA_2 = (int)DMA2_BASE
} DMAName;

#ifdef __cplusplus
}
#endif


class Dma{

/*
 * UART4RX:DMA_2.CH5
 * UART5RX:DMA_2.CH2
 */
public:
	enum SourcePortName {
	    UART4RX,
		UART5RX
	};
	Dma(SourcePortName port,int size){
		if(port!=UART4RX && port!=UART5RX)return;
		buffer=new uint8_t[size];
		hDMA=new DMA_HandleTypeDef;
		_size=size;
		hDMA->DmaBaseAddress=DMA_2;
		if(port==UART4RX){
			hDMA->Instance=DMA2_Channel5;
			hDMA->ChannelIndex=5;
		}else
		if(port==UART5RX){
			hDMA->Instance=DMA2_Channel2;
			hDMA->ChannelIndex=2;
		}
	}
private:
	int _size;
	uint8_t * buffer;
	DMA_HandleTypeDef *hDMA;
protected:

};


#endif /* DMA_H_ */
