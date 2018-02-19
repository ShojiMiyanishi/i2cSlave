/*
 * asyncSerial.h
 *
 *  Created on: 2017/05/29
 *      Author: omiya
 */

#ifndef ASYNCSERIAL_H_
#define ASYNCSERIAL_H_

#include "drivers/SerialBase.h"
#include "Timeout.h"

#ifndef DEVICE_SERIAL_ASYNCH
#error  define DEVICE_SERIAL_ASYNCH
#endif
namespace mbed {

class AsyncSerial: public SerialBase , private NonCopyable<AsyncSerial>{
public:
	enum state_t{
		idle=0,
		txJob,
		otherJob,
		changePinJob,
	};
	typedef struct {
		int id;
		state_t state;
	}message_t;
    /** Create a RawSerial port, connected to the specified transmit and receive pins, with the specified baud.
     *
     *  @param tx Transmit pin
     *  @param rx Receive pin
     *  @param baud The baud rate of the serial port (optional, defaults to MBED_CONF_PLATFORM_DEFAULT_SERIAL_BAUD_RATE)
     *
     *  @note
     *    Either tx or rx may be specified as NC if unused
     */
	AsyncSerial(PinName tx, PinName rx,			int size			);

	AsyncSerial(PinName tx, PinName rx);

	~AsyncSerial(void);

	bool changePin(PinName tx,PinName rx);
	bool changePin(int id,PinName tx,PinName rx , Callback<void(int)>callback);
    /** Write a char to the serial port
     *
     * @param c The char to write
     *
     * @returns The written char or -1 if an error occured
     */
    int putc(int c);

    /** Read a char from the serial port
     *
     * @returns The char read from the serial port
     */
    bool getc(char& c);
    int gets(char* c,int len);
    void flash(void){
    	rxReadPos=rxWritePos;
    }

    /** Write a string to the serial port
     *
     * @param str The string to write
     *
     * @returns 0 if the write succeeds, EOF for error
     */
    int puts(const char *str);

    int printf(const char *format, ...);
    int writeable(void){
    	if(readPos<=writePos){
    		return _size-writePos+readPos;
    	}else{
    		return readPos-writePos;
    	}
    };
    int readable(void){
    	if(rxReadPos>rxWritePos){
    		return _size+rxWritePos-rxReadPos;
    	}else{
    		return rxWritePos-rxReadPos;
    	}
//    	return rxChar;
    };
    char getPos(int i){
    	if(i+rxReadPos< _size){
    		return rxBuffer[i+rxReadPos];
    	}else{
    		return rxBuffer[(i+rxReadPos)% _size];
    	}

    };
    bool detachRxCallback(){
    	if(_rxCallback){
    		_rxCallback=0;
    		return true;
    	}else{
    		return false;
    	}
    }
    bool attachRxCallback(Callback<void(int count)>callback){
    	if(_rxCallback){
    		_rxCallback=callback;
    		return true;
    	}else{
    		return false;
    	}
    }
    bool checkReturnCode(Callback<void(int)>callback,char* checkString,int timeout){
    	if(_rxCallback){
    		_rxCallback=callback;
    		return true;
    	}else{
    		return false;
    	}
    }
#if 1
    int getTxBufferChar(void);
    void startWrite();
#else
    int getTxBufferChar(void){
    	if(readPos<writePos){
    		return writePos-readPos;
    	}else{
    		return _size-readPos+writePos;
    	}
    }
#endif
    /*
     *
    SerialBase &getSerialBase(void){
    	return _serialBase;
    };
     */

private:
    void init(void);
    void callbackWrite(int event);
    void callbackRead(int event);
	int _size;
	message_t _message;
	state_t _job;
	//SerialBase* _serialBase;
	Callback <void(int)>_rxCallback;
	Timeout _timeout;
	void changePinCallbackCall(void);
protected:
    //uint8_t buffer[500];
    uint8_t *buffer,*rxBuffer;
	event_callback_t func,rxFunc;
	Callback<void(int)>_callback;
	uint8_t _pinTx,_pinRx;
public:
    volatile int readPos,rxReadPos;
    volatile int writePos,rxWritePos;
    bool _terminator;
    bool debug(bool flag){_debug=flag;return _debug;};
    void startRx(void);
    int getBufferSize(void){return _size;};
    void stopRx(void){
    	attach(0,RxIrq);
    }
protected:
    volatile int rxChar;
	volatile bool working;
	//volatile bool mutex;
	volatile bool _debug;
    /** Acquire exclusive access to this serial port
     */
    void lock(void);

    /** Release exclusive access to this serial port
     */
    void unlock(void);
};
}
#endif /* ASYNCSERIAL_H_ */
