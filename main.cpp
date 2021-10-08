#include "stdafx.h"
#include "BSFMApp.h"

INT APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShowCmd)
{
	BSFMApp app;

	if (!app.initialize(hInst, nShowCmd))
		return -1;

	return app.run();
}