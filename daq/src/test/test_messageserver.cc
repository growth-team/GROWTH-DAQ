#include "MessageServer.hh"

int main(int argc, char* argv[]) {
  using namespace std;
  MessageServer messageServer(nullptr);
  cout << "Starting message server" << endl;
  messageServer.start();

  messageServer.join();
  cout << "Message server joined" << endl;
}
