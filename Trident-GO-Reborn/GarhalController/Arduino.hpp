#ifndef PC_ARDUINO_ARDUINO_H_
#define PC_ARDUINO_ARDUINO_H_

#pragma comment(lib, "Setupapi.lib")

#include <Windows.h>
#include <SetupAPI.h>
#include <devguid.h>

#include <string>
#include <vector>

// Commands
#define CMD_JUMP   1  // No arguments
#define CMD_AIM    2  // x, y -> how much to move mouse to in X and Y axis
#define CMD_MDOWN  3  // n - number of NOT dormant enemies (hp, weapon, position)
#define CMD_MUP    4
#define CMD_CLEFT  5
#define CMD_CRIGHT 6
#define CMD_W      7
#define CMD_S      8

class Arduino {
public:
	static void MouseMove(float x, float y, float z);
	static void Connect(LPCSTR com_port);

	static bool SendCommand(char cmd_index, const std::vector<float>& params = {});

	static bool GetDevice(LPCSTR friendly_name, LPSTR com_port);

	static bool SendData(const char* data, SIZE_T data_size);
private:
	Arduino() = default;
	~Arduino();

	static HANDLE arduino_handle_;
};

#endif  // PC_ARDUINO_ARDUINO_H_