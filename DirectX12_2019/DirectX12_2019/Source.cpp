#include <iostream>
#include "App.h"
#include <windows.h>
//int main(){
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	App& app = App::Instance();
	app.Initialize();
	app.Run();
	app.Terminate();
	return 0;
}