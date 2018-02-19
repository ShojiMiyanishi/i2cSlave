/*
 * asyncSerial.cpp
 *
 *  Created on: 2017/05/29
 *      Author: omiya
 */
#include "AsyncSerial.h"
#include "SerialBase.h"
#include <string>
#include <cstdarg>
#include "mbed.h"
extern DigitalOut *debugPin;
extern DigitalOut* debugPin2;
//namespace mbed {
AsyncSerial::AsyncSerial(PinName tx, PinName rx, int size  )
	: SerialBase(tx, rx, MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE)
{
	_size=size;
	buffer=new uint8_t[_size];
	rxBuffer=new uint8_t[_size];
	_pinTx=tx;
	_pinRx=rx;
	init();
}
AsyncSerial::AsyncSerial(PinName tx, PinName rx)
	: SerialBase(tx, rx,MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE)
{
	_size=300;
	buffer=new uint8_t[_size];
	rxBuffer=new uint8_t[_size];
	_pinTx=tx;
	_pinRx=rx;
	init();
}
void AsyncSerial::init(void){
	SerialBase::set_dma_usage_tx(DMA_USAGE_ALWAYS);
	SerialBase::set_dma_usage_rx(DMA_USAGE_ALWAYS);
	rxChar=0;
	//_size = sizeof(buffer);
	func.attach(this,&AsyncSerial::callbackWrite);
	rxFunc.attach(this,&AsyncSerial::callbackRead);
	writePos=readPos=rxReadPos=rxWritePos=0;
	working=false;
	_debug=false;
	_terminator=false;
	_callback=0;
	_job=idle;
	/*
	 * 1byte毎にコールバックさせる。
	 */
	start_read(rxBuffer,1,8,rxFunc,0xffff,SERIAL_RESERVED_CHAR_MATCH);
	_rxCallback=0;
}
void AsyncSerial::startRx(void){
	start_read(rxBuffer,1,8,rxFunc,0xffff,SERIAL_RESERVED_CHAR_MATCH);
}
AsyncSerial::~AsyncSerial(void){
	abort_read();
	delete(buffer);
	delete(rxBuffer);
}
bool AsyncSerial::changePin(PinName tx,PinName rx){
	bool ret=false;
	if(working)return ret;
	if(tx!=_pinTx || rx!=_pinRx){
		if(getTxBufferChar()==0){
			//attach(0,RxIrq);
			abort_read();

			flash();
			_pinTx=tx;
			_pinRx=rx;
			serial_init(&_serial, tx, rx);
			//attach(rxFunc,RxIrq);
			startRx();
			ret=true;
		}
	}
	*debugPin2=!*debugPin2;
	return ret;
}
void AsyncSerial::changePinCallbackCall(void){
	_callback.call((int)&_message);
}
bool AsyncSerial::changePin(int id,PinName tx,PinName rx,Callback <void(int)>callback){
	static bool flag=false;
	bool ret=false;
	if(flag)return ret;
	flag=true;
	_message.id=id;
	_callback=callback;
	if(tx!=_pinTx || rx!=_pinRx){
		if(getTxBufferChar()==0){
			//attach(0,RxIrq);
			//abort_read();

			flash();
			_pinRx=rx;
			_pinTx=tx;
			serial_init(&_serial, tx, rx);
			//attach(rxFunc,RxIrq);
			//startRx();
			_timeout.attach_us(this,&AsyncSerial::changePinCallbackCall,1);
			ret=true;
		}
	}else{
		_timeout.attach_us(this,&AsyncSerial::changePinCallbackCall,1);
		ret=true;
	}
	flag=false;
	*debugPin2=!*debugPin2;
	return ret;
}
bool AsyncSerial::getc(char &c) {
	bool ret=false;
    lock();
    _terminator=false;
    if(readable()>0){
    	rxChar--;
    	c=rxBuffer[rxReadPos];
    	rxReadPos++;
    	if(rxReadPos>=_size)rxReadPos=0;
    	ret = true;
    }
    unlock();
    return ret;
}
int AsyncSerial::gets(char *c,int len) {
	int ret=-1;
    if(readable()>0){
		int copylen;
		copylen= len>readable()?readable():len-1;
    	memcpy(c,&rxBuffer[rxReadPos],copylen);
    	c[copylen+1]=0;
    	ret=copylen;
    }
    return ret;
}

int AsyncSerial::putc(int c) {
    lock();
    int ret = _base_putc(c);
    unlock();
    return ret;
}

int AsyncSerial::puts(const char *str) {

    int len = strlen(str);
    {
		//書き込み位置が読み出し位置をオーバーした時の処理
		//読み出し位置を書き込み開始位置に設定し、オーバーライトされた部分をとばす。
		lock();
		if(1)
		{
			if( writePos> readPos){
				int tempPos= writePos+len-_size;
				//書き込み位置がゼロに戻った時
				if(tempPos>0){
					if(tempPos> readPos) readPos= writePos;
				}
			}else{
				if( writePos+len> readPos){
					 readPos= writePos;
				}
			}
		}
		if( writePos+len<_size){
			memcpy(& buffer[ writePos ],str,len);
			 writePos+=len;
		}else{
			//2分割コピー
			int pos=_size - writePos;
			memcpy(& buffer[ writePos ],str,pos);
			memcpy(& buffer[0],&str[pos],len-pos);
			 writePos=len-pos;
		}
		unlock();
    }
	if(! working){
		if( readPos!= writePos){
			startWrite();
		}
	}

    return 0;
}

void AsyncSerial::callbackWrite(int event)
{
	working=false;
	/*
	 *
	 */
	if( readPos!= writePos){
		startWrite();
	}
	if(_job==changePinJob){
		if( _callback ){
			_message.state=changePinJob;
			_callback.call((int)&_message);
			_callback=0;
		}
	}
	_job=idle;
}
void AsyncSerial::callbackRead(int event)
{
	if(rxBuffer[rxWritePos]=='\n'){
		_terminator=true;
	}
	rxWritePos++;
	if(rxWritePos>=_size){
		rxWritePos=0;
	}
	if( rxReadPos== rxWritePos){// overrunの処理：最古のデータをすてる。
		rxReadPos++;

		if(rxReadPos>=_size){
			rxReadPos=0;
		}
	}else{
		rxChar++;
	}
	start_read(&rxBuffer[rxWritePos],1,8,rxFunc,0xffff,SERIAL_RESERVED_CHAR_MATCH);
	if(_rxCallback){
		_rxCallback(readable());
		_rxCallback=0;
	}
}

void AsyncSerial::startWrite(){
	if(! working ){
		if( readPos!= writePos){
			int ret;
			if( readPos< writePos){
				ret = write((uint8_t*) & buffer[ readPos], writePos- readPos, func, 0xffffffff);
				if(ret==0){
					//p1=1;
					 working=true;
					 readPos= writePos;
				}
			}else{
				ret=write((uint8_t*) &  buffer[ readPos],_size - readPos, func,0xffffffff );
				if(ret==0){
					//p1=1;
					 working=true;
					 readPos=0;
				}
			}
		}
	}
}
// Experimental support for printf in AsyncSerial. No Stream inheritance
// means we can't call printf() directly, so we use sprintf() instead.
// We only call malloc() for the sprintf() buffer if the buffer
// length is above a certain threshold, otherwise we use just the stack.
int AsyncSerial::printf(const char *format, ...) {
#define STRING_STACK_LIMIT 256
    va_list arg;
    va_start(arg, format);
    // ARMCC microlib does not properly handle a _size of 0.
    // As a workaround supply a dummy buffer with a _size of 1.
    char dummy_buf[1];
    int len = vsnprintf(dummy_buf, sizeof(dummy_buf), format, arg);
    //if (len < STRING_STACK_LIMIT)
    {
        char *temp = new char[len + 1];
        vsprintf(temp, format, arg);
		//書き込み位置が読み出し位置をオーバーした時の処理
		//読み出し位置を書き込み開始位置に設定し、オーバーライトされた部分をとばす。
		lock();
		if(1)
		{
			if( writePos> readPos){
				int tempPos= writePos+len-_size;
				//書き込み位置がゼロに戻った時
				if(tempPos>0){
					if(tempPos> readPos) readPos= writePos;
				}
			}else{
				if( writePos+len> readPos){
					 readPos= writePos;
				}
			}
		}
		if( writePos+len<_size){
			memcpy(& buffer[ writePos ],temp,len);
			 writePos+=len;
		}else{
			//2分割コピー
			int pos=_size - writePos;
			memcpy(& buffer[ writePos ],temp,pos);
			memcpy(& buffer[0],&temp[pos],len-pos);
			 writePos=len-pos;
		}
		unlock();
        delete[] temp;
    }
	if(! working){
		if( readPos!= writePos){
			startWrite();
		}
	}
    va_end(arg);
    return len;
}

/** Acquire exclusive access to this serial port
 */
void AsyncSerial::lock() {
    // No lock used - external synchronization required
}

/** Release exclusive access to this serial port
 */
void AsyncSerial::unlock() {
    // No lock used - external synchronization required
}
int AsyncSerial::getTxBufferChar(void){
	if(readPos<=writePos){
		return writePos-readPos;
	}else{
		return _size-readPos+writePos;
	}
}

//}//end mbed


