/** Top-level file of the GROWTH Gamma-ray/particle event acquisition program.
 */
#include "MainThread.hh"
#include "MessageServer.hh"

int main(int argc, char* argv[]) {
	using namespace std;

	// Process arguments
	if (argc < 4) {
		cout << "Provide UART device name (e.g. /dev/tty.usb-aaa-bbb), YAML configuration file, and exposure." << endl;
		cout << endl;
		cout << "If zero or negative exposure is provided, the program pauses after boot." << endl;
		cout << "An observation is started when it is receives the 'resume' command via ZeroMQ IPC socket." << endl;
		::exit(-1);
	}
	std::string deviceName(argv[1]);
	std::string configurationFile(argv[2]);
	double exposureInSec = atoi(argv[3]);

	int dummyArgc = 0;
	char* dummyArgv[] = { (char*) "" };
#ifdef DRAW_CANVAS
	app = new TApplication("app", &dummyArgc, dummyArgv);
#endif

	// Instantiate
	MainThread* mainThread = new MainThread(deviceName, configurationFile, exposureInSec);
	MessageServer* messageServer = new MessageServer(mainThread);

#ifdef DRAW_CANVAS
	mainThread->start();
	CxxUtilities::Condition c;
	c.wait(3000);
	app->Run();
	c.wait(3000);
#else
	// Run
	if (exposureInSec > 0) {
		mainThread->start();
	}
	if (exposureInSec <= 0) {
		messageServer->start();
		messageServer->join();
	}
	mainThread->join();
#endif

	// Delete
	delete mainThread;
	delete messageServer;

	return 0;
}
