#include <iostream>
#include <string.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#pragma warning(disable:4996)

libusb_device_handle* devHandle = NULL;

// vendor ID
#define	VENDOR_ID			0x191A

// device ID
#define	DEVICE_ID			0x8003

// command version
#define	COMMAND_VERSION		0x00

// command ID
#define	COMMAND_ID			0x00

// endpoint address for transmission from host to USB control stacked signal light
#define	ENDPOINT_ADDRESS	1

// timeout time for sending command
#define	SEND_TIMEOUT		1000

// protocol data area size
#define	SEND_BUFFER_SIZE	8

// LED unit color
#define LED_COLOR_RED		0		// red
#define LED_COLOR_YELLOW	1		// yellow
#define LED_COLOR_GREEN		2		// green
#define LED_COLOR_BLUE		3		// blue
#define LED_COLOR_WHITE		4		// white

// LED pattern
#define LED_OFF				0x0		// light off
#define LED_ON				0x1		// lighted
#define LED_PATTERN1		0x2		// LED pattern 1
#define LED_PATTERN2		0x3		// LED pattern 2
#define LED_PATTERN3		0x4		// LED pattern 3
#define LED_PATTERN4		0x5		// LED pattern 4
#define LED_KEEP			0xF		// keep current settings

// buzzer pattern
#define BUZZER_OFF			0x0		// stop
#define BUZZER_ON			0x1		// buzzing (continuous)
#define BUZZER_PATTERN1		0x2		// buzzer pattern 1
#define BUZZER_PATTERN2		0x3		// buzzer pattern 2
#define BUZZER_PATTERN3		0x4		// buzzer pattern 3
#define BUZZER_PATTERN4		0x5		// buzzer pattern 4
#define BUZZER_KEEP			0xF		// keep current settings

// buzzer pitch
#define	BUZZER_PITCH_OFF	0x0		// stop
#define	BUZZER_PITCH1		0x1		// A6
#define	BUZZER_PITCH2		0x2		// B♭6
#define	BUZZER_PITCH3		0x3		// B6
#define	BUZZER_PITCH4		0x4		// C7
#define	BUZZER_PITCH5		0x5		// D♭7
#define	BUZZER_PITCH6		0x6		// D7
#define	BUZZER_PITCH7		0x7		// E♭7
#define	BUZZER_PITCH8		0x8		// E7
#define	BUZZER_PITCH9		0x9		// F7
#define	BUZZER_PITCH10		0xA		// G♭7
#define	BUZZER_PITCH11		0xB		// G7
#define	BUZZER_PITCH12		0xC		// A♭7
#define	BUZZER_PITCH13		0xD		// A7
#define	BUZZER_PITCH_DFLT_A	0xE		// default value for note A: D7
#define	BUZZER_PITCH_DFLT_B	0xF		// default value for sound B: (stop)

int UsbOpen();
void UsbClose();
int SendCommand(char* sendData, int sendLength);
int SetLight(unsigned char color, unsigned char state);
int SetTower(unsigned char red, unsigned char yellow, unsigned char green, unsigned char blue, unsigned char white);
int SetBuz(unsigned char buz_state, unsigned char limit);
int SetBuzEx(unsigned char buz_state, unsigned char limit, unsigned char pitch1, unsigned char pitch2);
int Reset();

/// <summary>
/// main function
/// </summary>
int main(int argc, char* argv[])
{
	int ret;
	
	// USB制御積層信号灯へUSB通信で接続する
	ret = UsbOpen();
    if (ret == -1)
    {
        return -1;
    }

    // Get the command identifier specified by the command line argument
    char* commandId = NULL;
    if (argc > 1) {
        commandId = argv[1];
    }

    switch (*commandId)
    {
        case '1':
        {
            // Turn on and pattern the USB controlled stacked signal lights by specifying the LED color and LED pattern
            if (argc >= 4)
                SetLight(atoi(argv[2]), atoi(argv[3]));
            break;
        }

        case '2':
        {
            // Pattern lighting of USB controlled stacked signal lights by specifying multiple LED colors and LED patterns
            if (argc >= 7)
                SetTower(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
            break;
        }

        case '3':
        {
            // Specify buzzer pattern to buzz USB controlled stacked signal lights
            if (argc >= 4)
                SetBuz(atoi(argv[2]), atoi(argv[3]));
            break;
        }

        case '4':
        {
            // Specify buzzer scale and pattern to buzz USB controlled laminated signal lights
            if (argc >= 6)
                SetBuzEx(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
            break;
        }

        case '5':
        {
            // Turn off all LED units and stop the buzzer
            Reset();
            break;
        }
    }
	
    // ソケットをクローズ
    UsbClose();
	
	return 0;
}

/// <summary>
/// Connect to a USB controlled stacked signal light via USB communication
/// </summary>
/// <returns>Success: 0, Failure: non-0</returns>
int UsbOpen()
{
	int ret;
	
	// Initialization
	ret = libusb_init(NULL);
	if (ret < 0) {
        std::cout << "libusb init failed" << std::endl;
		return -1;
	}
	
	// open the device
	devHandle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, DEVICE_ID);
	if (devHandle == NULL) {
        std::cout << "libusb open failed" << std::endl;
		return -1;
	}

	// Detach kernel driver
	libusb_detach_kernel_driver(devHandle, 0);
	ret = libusb_claim_interface(devHandle, 0);
	if (ret!= LIBUSB_SUCCESS) {
        std::cout << "libusb claim failed" << std::endl;
		return ret;
	}
	
	return 0;
}

/// <summary>
/// Terminate USB communication with USB controlled stacked signal light
/// </summary>
void UsbClose()
{
	if (devHandle != NULL) {
		libusb_close(devHandle);
	}
}

/// <summary>
/// Send command
/// </summary>
/// <param name="sendData">Data to be sent</param>
/// <param name="sendLength">Send Data Size</param>
/// <returns>success: 0, failure: non-zero</returns>
int SendCommand(unsigned char* sendData, int sendLength)
{
	int ret;
	int length = 0;

	if (devHandle == NULL) {
        std::cout << "device handle is not null" << std::endl;
		return -1;
	}

	// Send
	ret = libusb_interrupt_transfer(devHandle, ENDPOINT_ADDRESS, sendData, sendLength, &length, SEND_TIMEOUT);
	if (ret != 0) {
        std::cout << "failed to send" << std::endl;
		return -1;
	}

	return 0;
}

/// <summary>
/// Turn on and pattern the USB controlled stacked signal lights by specifying the LED color and LED pattern.
/// The buzzer and LED units other than the specified LED color will maintain their current state.
/// </summary>
/// <param name="color">LED color to control (Red: 0, Yellow: 1, Green: 2, Blue: 3, White: 4)</param>
/// <param name="state">LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <returns>success: 0, failure: non-zero</returns>
int SetLight(unsigned char color, unsigned char state)
{
	int ret;
    unsigned char sendData[SEND_BUFFER_SIZE];
    std::memset(sendData, 0, sizeof(sendData));

	// command version (0x00: fixed)
    sendData[0] = COMMAND_VERSION;

    // Command ID (0x00: fixed)
    sendData[1] = COMMAND_ID;

	// buzzer control (status quo)
    sendData[2] = BUZZER_KEEP;

    // buzzer scale
    sendData[3] = 0;

    // LED control
    sendData[4]  = (LED_KEEP << 4) | LED_KEEP;
    sendData[5]  = (LED_KEEP << 4) | LED_KEEP;
    sendData[6]  = LED_KEEP << 4;
	switch(color) {
	case LED_COLOR_RED:		// red
		sendData[4] = state << 4;
		sendData[4] |= LED_KEEP;
		break;
		
	case LED_COLOR_YELLOW:	// yellow
		sendData[4] = LED_KEEP << 4;
		sendData[4] |= state;
		break;
		
	case LED_COLOR_GREEN:	// green
		sendData[5] = state << 4;
		sendData[5] |= LED_KEEP;
		break;
		
	case LED_COLOR_BLUE:	// blue
		sendData[5] = LED_KEEP << 4;
		sendData[5] |= state;
		break;
		
	case LED_COLOR_WHITE:	// white
		sendData[6] = state << 4;
		sendData[6] |= LED_KEEP;
		break;
		
	default:
        std::cout << "out of range color" << std::endl;
		return -1;
	}

    // Empty ((0x00: Fixed))
    sendData[7] = 0;

    // send command
    ret = SendCommand(sendData, sizeof(sendData));
    if (ret != 0) {
        std::cout << "failed to send data" << std::endl;
        return -1;
    }

    return 0;
}

/// <summary>
/// Pattern lighting of USB controlled stacked signal lights by specifying multiple LED colors and LED patterns
/// </summary>
/// <param name="red">Red LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <param name="yellow">Yellow LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <param name="green">Green LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <param name="blue">Blue LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <param name="white">White LED pattern (off: 0, on: 0x1, LED pattern 1: 0x2, LED pattern 2: 0x3, LED pattern 3: 0x4, LED pattern 4: 0x5, keep current setting: 0x6 to 0xF)</param>
/// <returns>success: 0, failure: non-zero</returns>
int SetTower(unsigned char red, unsigned char yellow, unsigned char green, unsigned char blue, unsigned char white)
{
	int ret;
    unsigned char sendData[SEND_BUFFER_SIZE];
    std::memset(sendData, 0, sizeof(sendData));

	// command version (0x00: fixed)
    sendData[0] = COMMAND_VERSION;

    // Command ID (0x00: fixed)
    sendData[1] = COMMAND_ID;

	// Buzzer control (keep current)
    sendData[2] = BUZZER_KEEP;

    // buzzer scale
    sendData[3] = 0;

    // LED control
    sendData[4] |= red << 4;
    sendData[4] |= yellow;
    sendData[5] |= green << 4;
    sendData[5] |= blue;
    sendData[6] |= white << 4;

    // Empty ((0x00: Fixed))
    sendData[7] = 0;

    // send command
    ret = SendCommand(sendData, sizeof(sendData));
    if (ret != 0) {
        std::cout << "failed to send data" << std::endl;
        return -1;
    }

    return 0;
}

/// <summary>
/// Specify buzzer pattern to buzz USB controlled stacked signal lights.
/// The LED unit will maintain its current state. /// LED unit maintains its current state; scale operates at default values.
/// </summary>
/// <param name="buz_state">Buzzer pattern</param>
/// <param name="limit">Continuous operation: 0, frequency operation: 1-15</param>
/// <returns>success: 0, failure: non-zero</returns
int SetBuz(unsigned char buz_state, unsigned char limit)
{
	int ret;
    unsigned char sendData[SEND_BUFFER_SIZE];
    std::memset(sendData, 0, sizeof(sendData));

	// command version (0x00: fixed)
    sendData[0] = COMMAND_VERSION;

    // Command ID (0x00: fixed)
    sendData[1] = COMMAND_ID;

    // Buzzer control
    sendData[2] |= limit << 4;
    sendData[2] |= buz_state;

    // Buzzer scale
    sendData[3] |= BUZZER_PITCH_DFLT_A << 4;
    sendData[3] |= BUZZER_PITCH_DFLT_B;

    // LED control (status quo)
    sendData[4] |= LED_KEEP << 4;
    sendData[4] |= LED_KEEP;
    sendData[5] |= LED_KEEP << 4;
    sendData[5] |= LED_KEEP;
    sendData[6] |= LED_KEEP << 4;

    // Empty ((0x00: Fixed))
    sendData[7] = 0;

    // send command
    ret = SendCommand(sendData, sizeof(sendData));
    if (ret != 0) {
        std::cout << "failed to send data" << std::endl;
        return -1;
    }

    return 0;
}

/// <summary
/// Specify buzzer scale and pattern to buzz USB controlled laminated signal lights
/// </summary>
/// <param name="buz_state">Buzzer pattern</param>
/// <param name="limit">Continuous operation: 0, frequency operation: 1 to 15</param>
/// <param name="pitch1">Buzzer scale for note A (Stop: 0x0, A6: 0x1, B♭6: 0x2, B6: 0x3, C7: 0x4, D♭7: 0x5, D7: 0x6, E♭7: 0x7, E7: 0x8, F7: 0x9, G♭7: 0xA, G7: 0xB, A♭7: 0xC, A7 : 0xD, default value for note A: D7: 0xE, default value for note B: (stop): 0xF)</param>
/// <param name="pitch2">Buzzer scale for note B (Stop: 0x0, A6: 0x1, B♭6: 0x2, B6: 0x3, C7: 0x4, D♭7: 0x5, D7: 0x6, E♭7: 0x7, E7: 0x8, F7: 0x9, G♭7: 0xA, G7: 0xB, A♭7: 0xC, A7 : 0xD, default value for note A: D7: 0xE, default value for note B: (stop): 0xF)</param>
/// <returns>success: 0, failure: non-zero</returns>.
int SetBuzEx(unsigned char buz_state, unsigned char limit, unsigned char pitch1, unsigned char pitch2)
{
	int ret;
    unsigned char sendData[SEND_BUFFER_SIZE];
    std::memset(sendData, 0, sizeof(sendData));

	// command version (0x00: fixed)
    sendData[0] = COMMAND_VERSION;

    // Command ID (0x00: fixed)
    sendData[1] = COMMAND_ID;

    // Buzzer control
    sendData[2] |= limit << 4;
    sendData[2] |= buz_state;

    // Buzzer scale
    sendData[3] |= pitch1 << 4;
    sendData[3] |= pitch2;

    // LED control (status quo)
    sendData[4] |= LED_KEEP << 4;
    sendData[4] |= LED_KEEP;
    sendData[5] |= LED_KEEP << 4;
    sendData[5] |= LED_KEEP;
    sendData[6] |= LED_KEEP << 4;

    // Empty ((0x00: Fixed))
    sendData[7] = 0;

    // send command
    ret = SendCommand(sendData, sizeof(sendData));
    if (ret != 0) {
        std::cout << "failed to send data" << std::endl;
        return -1;
    }

    return 0;
}

/// <summary>
/// Turn off all LED units and stop the buzzer
/// </summary>
/// <returns>success: 0, failure: non-zero</returns
int Reset()
{
	int ret;
    unsigned char sendData[SEND_BUFFER_SIZE];
    std::memset(sendData, 0, sizeof(sendData));

	// command version (0x00: fixed)
    sendData[0] = COMMAND_VERSION;

    // Command ID (0x00: fixed)
    sendData[1] = COMMAND_ID;

    // Buzzer control
    sendData[2] = BUZZER_OFF;

    // buzzer pitch
    sendData[3] = BUZZER_PITCH_OFF;

    // LED control
    sendData[4] = LED_OFF;
    sendData[5] = LED_OFF;
    sendData[6] = LED_OFF;

    // Empty ((0x00: Fixed))
    sendData[7] = 0;

    // send command
    ret = SendCommand(sendData, sizeof(sendData));
    if (ret != 0) {
        std::cout << "failed to send data" << std::endl;
        return -1;
    }

    return 0;
}
